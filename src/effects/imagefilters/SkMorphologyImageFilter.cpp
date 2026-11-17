/*
 * Copyright 2012 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/effects/SkImageFilters.h"

#include "include/core/SkBitmap.h"
#include "include/core/SkFlattenable.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageFilter.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkM44.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkRect.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkScalar.h"
#include "include/core/SkShader.h"
#include "include/core/SkSize.h"
#include "include/core/SkSpan.h"
#include "include/core/SkTypes.h"
#include "include/effects/SkRuntimeEffect.h"
#include "src/core/SkImageFilterTypes.h"
#include "src/core/SkImageFilter_Base.h"
#include "src/core/SkKnownRuntimeEffects.h"
#include "src/core/SkReadBuffer.h"
#include "src/core/SkSpecialImage.h"
#include "src/core/SkVx.h"
#include "src/core/SkWriteBuffer.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace {

enum class MorphType {
    kErode,
    kDilate,
    kLastType = kDilate
};

enum class MorphDirection { kX, kY };

class SkMorphologyImageFilter final : public SkImageFilter_Base {
public:
    SkMorphologyImageFilter(MorphType type, SkSize radii, sk_sp<SkImageFilter> input)
            : SkImageFilter_Base(&input, 1)
            , fType(type)
            , fRadii(radii) {}

    SkRect computeFastBounds(const SkRect& src) const override;


protected:
    void flatten(SkWriteBuffer&) const override;

private:
    friend void ::SkRegisterMorphologyImageFilterFlattenables();
    SK_FLATTENABLE_HOOKS(SkMorphologyImageFilter)

    skif::FilterResult onFilterImage(const skif::Context&) const override;

    skif::LayerSpace<SkIRect> onGetInputLayerBounds(
            const skif::Mapping& mapping,
            const skif::LayerSpace<SkIRect>& desiredOutput,
            std::optional<skif::LayerSpace<SkIRect>> contentBounds) const override;

    std::optional<skif::LayerSpace<SkIRect>> onGetOutputLayerBounds(
            const skif::Mapping& mapping,
            std::optional<skif::LayerSpace<SkIRect>> contentBounds) const override;

    skif::LayerSpace<SkISize> radii(const skif::Mapping& mapping) const {
        skif::LayerSpace<SkISize> radii = mapping.paramToLayer(fRadii).round();
        SkASSERT(radii.width() >= 0 && radii.height() >= 0);

        // We limit the radius to something small, to avoid slow draw calls: crbug.com/1123035
        static constexpr int kMaxRadii = 256;
        return skif::LayerSpace<SkISize>({std::min(radii.width(), kMaxRadii),
                                          std::min(radii.height(), kMaxRadii)});
    }

    skif::LayerSpace<SkIRect> requiredInput(const skif::Mapping& mapping,
                                            skif::LayerSpace<SkIRect> bounds) const {
        // The input for a morphology filter is always the kernel outset, regardless of morph type.
        bounds.outset(this->radii(mapping));
        return bounds;
    }

    skif::LayerSpace<SkIRect> kernelOutputBounds(const skif::Mapping& mapping,
                                                 skif::LayerSpace<SkIRect> bounds) const {
        skif::LayerSpace<SkISize> radii = this->radii(mapping);
        if (fType == MorphType::kDilate) {
            // Transparent pixels up to the kernel radius away will be overridden by kDilate's "max"
            // function and be set to the input's boundary pixel colors, thus expanding the output.
            bounds.outset(radii);
        } else {
            // Pixels closer than the kernel radius to the input image's edges are overridden by
            // kErode's "min" function and will be set to transparent black, contracting the output.
            bounds.inset(radii);
        }
        return bounds;
    }

    MorphType fType;
    skif::ParameterSpace<SkSize> fRadii;
};

sk_sp<SkImageFilter> make_morphology(MorphType type,
                                     SkSize radii,
                                     sk_sp<SkImageFilter> input,
                                     const SkImageFilters::CropRect& cropRect) {
    if (radii.width() < 0.f || radii.height() < 0.f) {
        return nullptr; // invalid
    }
    sk_sp<SkImageFilter> filter = std::move(input);
    if (radii.width() > 0.f || radii.height() > 0.f) {
        filter = sk_sp<SkImageFilter>(new SkMorphologyImageFilter(type, radii, std::move(filter)));
    }
    // otherwise both radii are 0, so the kernel is always the identity function, in which case
    // we just need to apply the 'cropRect' to the 'input'.

    if (cropRect) {
        filter = SkImageFilters::Crop(*cropRect, std::move(filter));
    }
    return filter;
}

// The linear morphology kernel does (2R+1) texture samples per pixel, which we want to keep less
// than the maximum fragment samples allowed in DX9SM2 (32), so we choose R=14 to have some head
// room. The other tradeoff is that for R > kMaxLinearRadius, the sparse morphology kernel only
// requires 2 samples to double the accumulated kernel size, but at the cost of another render
// target.
static constexpr int kMaxLinearRadius = 14; // KEEP IN SYNC WITH CONSTANT IN `sksl_rt_shader.sksl`
sk_sp<SkShader> make_linear_morphology(sk_sp<SkShader> input,
                                       MorphType type,
                                       MorphDirection direction,
                                       int radius) {
    SkASSERT(radius <= kMaxLinearRadius);

    const SkRuntimeEffect* linearMorphologyEffect =
            GetKnownRuntimeEffect(SkKnownRuntimeEffects::StableKey::kLinearMorphology);

    SkRuntimeShaderBuilder builder(sk_ref_sp(linearMorphologyEffect));
    builder.child("child") = std::move(input);
    builder.uniform("offset") = direction == MorphDirection::kX ? SkV2{1.f, 0.f} : SkV2{0.f, 1.f};
    builder.uniform("flip") = (type == MorphType::kDilate) ? 1.f : -1.f;
    builder.uniform("radius") = (int32_t)radius;

    return builder.makeShader();
}

// Assuming 'input' was created by a series of morphology passes, each texel holds the aggregate
// (min or max depending on type) of (i-R) to (i+R) for some R. If 'radius' <= R, then the returned
// shader produces a new aggregate at each texel, i, of (i-R-radius) to (i+R+radius) with only two
// texture samples, which can be used to double the kernel size of the morphology effect.
sk_sp<SkShader> make_sparse_morphology(sk_sp<SkShader> input,
                                       MorphType type,
                                       MorphDirection direction,
                                       int radius) {

    const SkRuntimeEffect* sparseMorphologyEffect =
            GetKnownRuntimeEffect(SkKnownRuntimeEffects::StableKey::kSparseMorphology);

    SkRuntimeShaderBuilder builder(sk_ref_sp(sparseMorphologyEffect));
    builder.child("child") = std::move(input);
    builder.uniform("offset") = direction == MorphDirection::kX ? SkV2{(float)radius, 0.f}
                                                                : SkV2{0.f, (float)radius};
    builder.uniform("flip") = (type == MorphType::kDilate) ? 1.f : -1.f;

    return builder.makeShader();
}

skif::FilterResult morphology_pass(const skif::Context& ctx, const skif::FilterResult& input,
                                   MorphType type, MorphDirection dir, int radius) {
    using ShaderFlags = skif::FilterResult::ShaderFlags;

    auto axisDelta = [dir](int step) {
        return skif::LayerSpace<SkISize>({
                dir == MorphDirection::kX ? step : 0,
                dir == MorphDirection::kY ? step : 0});
    };

    // The first iteration will sample a full kernel outset from the final output.
    skif::LayerSpace<SkIRect> sampleBounds = ctx.desiredOutput();
    sampleBounds.outset(axisDelta(radius));

    skif::FilterResult childOutput = input;
    int appliedRadius = 0;
    while (radius > appliedRadius) {
        if (!childOutput) {
            return {}; // Eroded or dilated transparent black is still transparent black
        }

        // The first iteration uses up to kMaxLinearRadius with a linear accumulation pass.
        // After that we double the radius each step until we can finish with the target radius.
        int stepRadius =
                appliedRadius == 0 ? std::min(kMaxLinearRadius, radius)
                                   : std::min(radius - appliedRadius, appliedRadius);

        skif::Context stepCtx = ctx;
        if (appliedRadius + stepRadius < radius) {
            // Intermediate steps need to output what will be sampled on the next iteration
            auto outputBounds = sampleBounds;
            outputBounds.inset(axisDelta(stepRadius));
            stepCtx = ctx.withNewDesiredOutput(outputBounds);
        } // else the last iteration should output what was originally requested

        skif::FilterResult::Builder builder{stepCtx};
        builder.add(childOutput, sampleBounds, ShaderFlags::kSampledRepeatedly);
        childOutput = builder.eval(
                [&](SkSpan<sk_sp<SkShader>> inputs) {
                    if (appliedRadius == 0) {
                        return make_linear_morphology(inputs[0], type, dir, stepRadius);
                    } else {
                        return make_sparse_morphology(inputs[0], type, dir, stepRadius);
                    }
                });

        sampleBounds = stepCtx.desiredOutput();
        appliedRadius += stepRadius;
        SkASSERT(appliedRadius <= radius); // Our last iteration should hit 'radius' exactly.
    }

    return childOutput;
}

// Per-channel max (dilate) and min (erode) of two premultiplied N32 pixels.
uint32_t pixel_max(uint32_t a, uint32_t b) {
    uint32_t result;
    skvx::max(skvx::byte4::Load(&a), skvx::byte4::Load(&b)).store(&result);
    return result;
}

uint32_t pixel_min(uint32_t a, uint32_t b) {
    uint32_t result;
    skvx::min(skvx::byte4::Load(&a), skvx::byte4::Load(&b)).store(&result);
    return result;
}

// 1D van Herk/Gil-Werman filter: the max or min over a window of 2 * r + 1 pixels, in a constant
// number of operations per pixel regardless of the radius. 'prefix' and 'suffix' are scratch
// buffers of at least n pixels. Pixels outside [0, n) count as transparent black. 'src' and 'dst'
// may be the same buffer.
template <uint32_t (*Op)(uint32_t, uint32_t)>
void van_herk_pass(const uint32_t* src, uint32_t* dst, int n, int r,
                   uint32_t* prefix, uint32_t* suffix) {
    if (r <= 0) {
        std::copy(src, src + n, dst);
        return;
    }
    const int window = 2 * r + 1;
    for (int i = 0; i < n; ++i) {
        prefix[i] = (i % window == 0) ? src[i] : Op(prefix[i - 1], src[i]);
    }
    for (int i = n - 1; i >= 0; --i) {
        suffix[i] = (i % window == window - 1 || i == n - 1) ? src[i] : Op(suffix[i + 1], src[i]);
    }
    for (int i = 0; i < n; ++i) {
        const int lo = i - r;
        const int hi = i + r;
        const uint32_t a = (lo >= 0) ? suffix[lo] : 0;
        const uint32_t b = (hi < n) ? prefix[hi] : 0;
        dst[i] = Op(a, b);
    }
}

// Applies the 1D filter to every row with radius rx, then to every column with radius ry.
template <uint32_t (*Op)(uint32_t, uint32_t)>
void morphology_2d(const SkPixmap& src, const SkPixmap& dst, int rx, int ry) {
    const int w = src.width();
    const int h = src.height();
    std::vector<uint32_t> prefix(std::max(w, h));
    std::vector<uint32_t> suffix(std::max(w, h));
    std::vector<uint32_t> column(h);
    std::vector<uint32_t> tmp((size_t)w * h);

    for (int y = 0; y < h; ++y) {
        van_herk_pass<Op>(src.addr32(0, y), tmp.data() + (size_t)y * w, w, rx,
                          prefix.data(), suffix.data());
    }

    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            column[y] = tmp[(size_t)y * w + x];
        }
        van_herk_pass<Op>(column.data(), column.data(), h, ry, prefix.data(), suffix.data());
        for (int y = 0; y < h; ++y) {
            *dst.writable_addr32(x, y) = column[y];
        }
    }
}

// Applies the morphology directly on the CPU. Returns std::nullopt if the input isn't a raster
// N32 premul image (e.g. a GPU texture or F16), in which case the caller falls back to the
// shader passes. An empty FilterResult means the output is empty.
std::optional<skif::FilterResult> apply_native_morphology(
        const skif::Context& ctx,
        const skif::FilterResult& input,
        MorphType type,
        skif::LayerSpace<SkISize> radii,
        skif::LayerSpace<SkIRect> maxOutput) {
    auto [srcImg, srcOrigin] = input.imageAndOffset(ctx);
    if (!srcImg) {
        return skif::FilterResult{};
    }

    sk_sp<SkImage> srcAsImage = srcImg->asImage();
    SkPixmap probe;
    if (!srcAsImage || !srcAsImage->peekPixels(&probe)) {
        return std::nullopt;
    }
    if (probe.colorType() != kN32_SkColorType || probe.alphaType() != kPremul_SkAlphaType) {
        return std::nullopt;
    }

    const int rx = radii.width();
    const int ry = radii.height();

    // The working area is maxOutput outset by the radii, so every output window lies inside it.
    // Pixels the child didn't cover stay transparent black, which matches decal tiling: dilate's
    // max ignores them and erode's min shrinks towards them.
    skif::LayerSpace<SkIRect> srcBounds = maxOutput;
    srcBounds.outset(radii);
    const int workW = srcBounds.width();
    const int workH = srcBounds.height();
    if (workW <= 0 || workH <= 0) {
        return skif::FilterResult{};
    }

    const SkImageInfo info = SkImageInfo::Make(workW, workH, kN32_SkColorType,
                                               kPremul_SkAlphaType, probe.refColorSpace());
    SkBitmap srcBuf, dstBuf;
    if (!srcBuf.tryAllocPixels(info) || !dstBuf.tryAllocPixels(info)) {
        return std::nullopt;
    }
    srcBuf.eraseColor(SK_ColorTRANSPARENT);

    // Copy the child's output into srcBuf at its position relative to the working area.
    const int dx = srcOrigin.x() - srcBounds.left();
    const int dy = srcOrigin.y() - srcBounds.top();
    SkIRect land = SkIRect::MakeXYWH(dx, dy, srcImg->width(), srcImg->height());
    if (land.intersect(SkIRect::MakeWH(workW, workH))) {
        SkPixmap bufPixmap, subset;
        if (srcBuf.peekPixels(&bufPixmap) && bufPixmap.extractSubset(&subset, land)) {
            (void)srcAsImage->readPixels(subset, land.left() - dx, land.top() - dy);
        }
    }

    SkPixmap srcPixmap, dstPixmap;
    srcBuf.peekPixels(&srcPixmap);
    dstBuf.peekPixels(&dstPixmap);
    if (type == MorphType::kDilate) {
        morphology_2d<pixel_max>(srcPixmap, dstPixmap, rx, ry);
    } else {
        morphology_2d<pixel_min>(srcPixmap, dstPixmap, rx, ry);
    }

    // The output for maxOutput is the interior of dstBuf, inset by the radii.
    SkBitmap out;
    if (!dstBuf.extractSubset(&out, SkIRect::MakeXYWH(rx, ry, maxOutput.width(),
                                                      maxOutput.height()))) {
        return std::nullopt;
    }
    out.setImmutable();

    auto special = SkSpecialImages::MakeFromRaster(SkIRect::MakeWH(out.width(), out.height()),
                                                   out, SkSurfaceProps{});
    return skif::FilterResult(std::move(special), maxOutput.topLeft());
}

} // end namespace

sk_sp<SkImageFilter> SkImageFilters::Dilate(SkScalar radiusX, SkScalar radiusY,
                                            sk_sp<SkImageFilter> input,
                                            const CropRect& cropRect) {
    return make_morphology(MorphType::kDilate, {radiusX, radiusY}, std::move(input), cropRect);
}

sk_sp<SkImageFilter> SkImageFilters::Erode(SkScalar radiusX, SkScalar radiusY,
                                           sk_sp<SkImageFilter> input,
                                           const CropRect& cropRect) {
    return make_morphology(MorphType::kErode, {radiusX, radiusY}, std::move(input), cropRect);
}

void SkRegisterMorphologyImageFilterFlattenables() {
    SK_REGISTER_FLATTENABLE(SkMorphologyImageFilter);
    // TODO (michaelludwig): Remove after grace period for SKPs to stop using old name
    SkFlattenable::Register("SkMorphologyImageFilterImpl", SkMorphologyImageFilter::CreateProc);
}

sk_sp<SkFlattenable> SkMorphologyImageFilter::CreateProc(SkReadBuffer& buffer) {
    SK_IMAGEFILTER_UNFLATTEN_COMMON(common, 1);

    SkScalar width = buffer.readScalar();
    SkScalar height = buffer.readScalar();
    MorphType filterType = buffer.read32LE(MorphType::kLastType);

    if (filterType == MorphType::kDilate) {
        return SkImageFilters::Dilate(width, height, common.getInput(0), common.cropRect());
    } else if (filterType == MorphType::kErode) {
        return SkImageFilters::Erode(width, height, common.getInput(0), common.cropRect());
    } else {
        return nullptr;
    }
}

void SkMorphologyImageFilter::flatten(SkWriteBuffer& buffer) const {
    this->SkImageFilter_Base::flatten(buffer);
    buffer.writeScalar(SkSize(fRadii).width());
    buffer.writeScalar(SkSize(fRadii).height());
    buffer.writeInt(static_cast<int>(fType));
}

///////////////////////////////////////////////////////////////////////////////

skif::FilterResult SkMorphologyImageFilter::onFilterImage(const skif::Context& ctx) const {
    skif::LayerSpace<SkIRect> requiredInput =
            this->requiredInput(ctx.mapping(), ctx.desiredOutput());
    skif::FilterResult childOutput =
            this->getChildOutput(0, ctx.withNewDesiredOutput(requiredInput));

    // If childOutput completely fulfilled requiredInput, maxOutput will match the context's
    // desired output, but if the output image is smaller, this will restrict the morphology output
    // to what is actual produceable.
    skif::LayerSpace<SkIRect> maxOutput =
        this->kernelOutputBounds(ctx.mapping(), childOutput.layerBounds());
    if (!maxOutput.intersect(ctx.desiredOutput())) {
        return {};
    }

    skif::LayerSpace<SkISize> radii = this->radii(ctx.mapping());
    if (auto handled = apply_native_morphology(ctx, childOutput, fType, radii, maxOutput)) {
        return *handled;
    }

    // The X pass has to preserve the extra rows to later be consumed by the Y pass.
    skif::LayerSpace<SkIRect> maxOutputX = maxOutput;
    maxOutputX.outset(skif::LayerSpace<SkISize>({0, radii.height()}));
    childOutput = morphology_pass(ctx.withNewDesiredOutput(maxOutputX), childOutput, fType,
                                  MorphDirection::kX, radii.width());
    childOutput = morphology_pass(ctx.withNewDesiredOutput(maxOutput), childOutput, fType,
                                  MorphDirection::kY, radii.height());
    return childOutput;
}

skif::LayerSpace<SkIRect> SkMorphologyImageFilter::onGetInputLayerBounds(
        const skif::Mapping& mapping,
        const skif::LayerSpace<SkIRect>& desiredOutput,
        std::optional<skif::LayerSpace<SkIRect>> contentBounds) const {
    skif::LayerSpace<SkIRect> requiredInput = this->requiredInput(mapping, desiredOutput);
    return this->getChildInputLayerBounds(0, mapping, requiredInput, contentBounds);
}

std::optional<skif::LayerSpace<SkIRect>> SkMorphologyImageFilter::onGetOutputLayerBounds(
        const skif::Mapping& mapping,
        std::optional<skif::LayerSpace<SkIRect>> contentBounds) const {
    auto childOutput = this->getChildOutputLayerBounds(0, mapping, contentBounds);
    if (childOutput) {
        return this->kernelOutputBounds(mapping, *childOutput);
    } else {
        return skif::LayerSpace<SkIRect>::Unbounded();
    }
}

SkRect SkMorphologyImageFilter::computeFastBounds(const SkRect& src) const {
    // See kernelOutputBounds() for rationale
    SkRect bounds = this->getInput(0) ? this->getInput(0)->computeFastBounds(src) : src;
    if (fType == MorphType::kDilate) {
        bounds.outset(SkSize(fRadii).width(), SkSize(fRadii).height());
    } else {
        bounds.inset(SkSize(fRadii).width(), SkSize(fRadii).height());
    }
    return bounds;
}
