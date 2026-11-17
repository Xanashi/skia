/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/effects/SkImageFilters.h"

#include "include/core/SkBitmap.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkColorSpace.h"
#include "include/core/SkFlattenable.h"
#include "include/core/SkImage.h"
#include "include/core/SkImageFilter.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkSamplingOptions.h"
#include "include/core/SkScalar.h"
#include "include/core/SkSize.h"
#include "include/core/SkTypes.h"
#include "include/private/SkTo.h"
#include "src/core/SkColorPriv.h"
#include "src/core/SkImageFilterTypes.h"
#include "src/core/SkImageFilter_Base.h"
#include "src/core/SkPicturePriv.h"
#include "src/core/SkReadBuffer.h"
#include "src/core/SkSpecialImage.h"
#include "src/core/SkWriteBuffer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

struct SkRect;

namespace {

// 1D running-sum box blur of a single 8-bit plane. Pixels outside the plane count as 0
// (transparent), and the sum is always divided by the full window, so edges fade out. This
// matches feGaussianBlur's edgeMode="none".
void box_pass(const uint8_t* src, uint8_t* dst, int n, int stride, int r) {
    if (r <= 0) {
        for (int i = 0; i < n; ++i) {
            dst[i * stride] = src[i * stride];
        }
        return;
    }
    const int window = 2 * r + 1;
    // 20-bit fixed-point reciprocal of the window size.
    const uint32_t recip = ((1u << 20) + window - 1) / window;
    int sum = 0;
    for (int i = 0; i <= r && i < n; ++i) {
        sum += src[i * stride];
    }
    for (int i = 0; i < n; ++i) {
        // sum / window, rounded.
        dst[i * stride] = (uint8_t)(((uint32_t)sum * recip + (1u << 19)) >> 20);
        const int add = i + r + 1;
        const int sub = i - r;
        if (add < n) {
            sum += src[add * stride];
        }
        if (sub >= 0) {
            sum -= src[sub * stride];
        }
    }
}

// Three box passes of radius r have a total variance of r^2 + r, so
// r = (-1 + sqrt(1 + 4 * sigma^2)) / 2 reproduces the target sigma.
int box_radius(float sigma) {
    if (sigma <= 0.f) {
        return 0;
    }
    return std::max(1, (int)lroundf((std::sqrt(1.f + 4.f * sigma * sigma) - 1.f) * 0.5f));
}

// Approximates a Gaussian blur with three box passes per axis. 'tmp' must hold w * h bytes.
void blur_direct(uint8_t* a, uint8_t* tmp, int w, int h, float sx, float sy) {
    const int rx = box_radius(sx);
    const int ry = box_radius(sy);
    if (rx > 0) {
        // Horizontal passes, alternating between 'a' and 'tmp'. The third pass ends in 'tmp'.
        for (int p = 0; p < 3; ++p) {
            uint8_t* s = (p & 1) ? tmp : a;
            uint8_t* d = (p & 1) ? a : tmp;
            for (int y = 0; y < h; ++y) {
                box_pass(s + (size_t)y * w, d + (size_t)y * w, w, 1, rx);
            }
        }
        std::memcpy(a, tmp, (size_t)w * h);
    }
    if (ry > 0) {
        // Vertical passes.
        for (int p = 0; p < 3; ++p) {
            uint8_t* s = (p & 1) ? tmp : a;
            uint8_t* d = (p & 1) ? a : tmp;
            for (int x = 0; x < w; ++x) {
                box_pass(s + x, d + x, h, w, ry);
            }
        }
        std::memcpy(a, tmp, (size_t)w * h);
    }
}

// Area-average downsample by (kx, ky). Each source pixel is read exactly once.
void downsample_box(const uint8_t* src, int w, int h,
                    uint8_t* dst, int dw, int dh, int kx, int ky) {
    for (int y = 0; y < dh; ++y) {
        const int y0 = y * ky;
        const int y1 = std::min(y0 + ky, h);
        for (int x = 0; x < dw; ++x) {
            const int x0 = x * kx;
            const int x1 = std::min(x0 + kx, w);
            unsigned sum = 0;
            const unsigned count = (unsigned)(x1 - x0) * (unsigned)(y1 - y0);
            for (int yy = y0; yy < y1; ++yy) {
                const uint8_t* row = src + (size_t)yy * w;
                for (int xx = x0; xx < x1; ++xx) {
                    sum += row[xx];
                }
            }
            dst[(size_t)y * dw + x] = count ? (uint8_t)((sum + count / 2) / count) : 0;
        }
    }
}

// Bilinear upsample of 'small' (dw x dh) into 'dst' (w x h), with pixel centers aligned.
void upsample_bilinear(const uint8_t* small, int dw, int dh,
                       uint8_t* dst, int w, int h, int kx, int ky) {
    for (int y = 0; y < h; ++y) {
        const float fy = (y + 0.5f) / ky - 0.5f;
        const int iy = (int)std::floor(fy);
        const float wy = fy - iy;
        const int y0 = std::clamp(iy, 0, dh - 1);
        const int y1 = std::clamp(iy + 1, 0, dh - 1);
        const uint8_t* r0 = small + (size_t)y0 * dw;
        const uint8_t* r1 = small + (size_t)y1 * dw;
        uint8_t* dstRow = dst + (size_t)y * w;
        for (int x = 0; x < w; ++x) {
            const float fx = (x + 0.5f) / kx - 0.5f;
            const int ix = (int)std::floor(fx);
            const float wx = fx - ix;
            const int x0 = std::clamp(ix, 0, dw - 1);
            const int x1 = std::clamp(ix + 1, 0, dw - 1);
            const float top = r0[x0] * (1.f - wx) + r0[x1] * wx;
            const float bottom = r1[x0] * (1.f - wx) + r1[x1] * wx;
            dstRow[x] = (uint8_t)(top * (1.f - wy) + bottom * wy + 0.5f);
        }
    }
}

// Blurs the alpha plane. Large sigmas are blurred on a downsampled plane and upsampled
// bilinearly again, which only touches the full-resolution plane twice instead of six times.
// The difference is not visible in shadows. Small sigmas are blurred at full resolution, which
// is already cheap. 'tmp' must hold w * h bytes (only the full-resolution path uses it).
void blur_alpha_plane(uint8_t* a, uint8_t* tmp, int w, int h, float sx, float sy) {
    // The downsample factor aims for a sigma of about 2 in the small plane, and keeps the small
    // plane at least about 8 pixels wide and high.
    auto factor = [](float sigma, int dim) {
        int k = std::max(1, (int)lroundf(sigma * 0.5f));
        return std::min(k, std::max(1, dim / 8));
    };
    const int kx = factor(sx, w);
    const int ky = factor(sy, h);

    if (kx <= 1 && ky <= 1) {
        blur_direct(a, tmp, w, h, sx, sy);
        return;
    }

    const int dw = (w + kx - 1) / kx;
    const int dh = (h + ky - 1) / ky;
    std::vector<uint8_t> small((size_t)dw * dh);
    std::vector<uint8_t> smallTmp((size_t)dw * dh);

    downsample_box(a, w, h, small.data(), dw, dh, kx, ky);
    // The area average and the bilinear upsample add a little blur of their own, so the spread
    // is slightly wider than Skia's Gaussian blur.
    blur_direct(small.data(), smallTmp.data(), dw, dh, sx / kx, sy / ky);
    upsample_bilinear(small.data(), dw, dh, a, w, h, kx, ky);
}

// Maps 0..255 to a 0..256 scale that is exact at both ends. Unlike SkAlpha255To256 (alpha + 1),
// this maps 0 to 0, so zero coverage adds nothing.
unsigned alpha_to_256(unsigned alpha) {
    return alpha + (alpha >> 7);
}

// Composites the colored, offset shadow under the source, or writes the shadow alone if
// 'shadowOnly' is set. 'flood' is the premultiplied shadow color. 'blurA' and 'src' have the
// same bounds as 'dst', and (dx, dy) is the shadow offset in pixels.
void composite_shadow(const SkPixmap& src, const uint8_t* blurA, const SkPixmap& dst,
                      SkPMColor flood, int dx, int dy, bool shadowOnly) {
    const int w = dst.width();
    const int h = dst.height();
    for (int y = 0; y < h; ++y) {
        const uint32_t* srcRow = shadowOnly ? nullptr : src.addr32(0, y);
        uint32_t* dstRow = dst.writable_addr32(0, y);
        for (int x = 0; x < w; ++x) {
            // The shadow at (x, y) is the blurred alpha at (x - dx, y - dy).
            const int bx = x - dx;
            const int by = y - dy;
            const bool inside = bx >= 0 && bx < w && by >= 0 && by < h;
            const unsigned coverage = inside ? blurA[(size_t)by * w + bx] : 0;
            const uint32_t s = srcRow ? srcRow[x] : 0;
            if (!coverage) {
                dstRow[x] = s;
                continue;
            }
            // Source over the premultiplied shadow, scaled by its coverage.
            const uint32_t shadow = SkAlphaMulQ(flood, alpha_to_256(coverage));
            dstRow[x] = s + SkAlphaMulQ(shadow, alpha_to_256(255 - SkGetPackedA32(s)));
        }
    }
}

// Renders the drop shadow directly on the CPU. Returns std::nullopt if the input isn't a raster
// N32 premul image (e.g. a GPU texture or F16), in which case the caller falls back to the
// composed filter graph. An empty FilterResult means the output is empty.
std::optional<skif::FilterResult> apply_native_drop_shadow(
        const skif::Context& ctx,
        const skif::FilterResult& input,
        float sx, float sy, int dx, int dy,
        SkPMColor flood, bool shadowOnly,
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

    // The working area covers maxOutput (where the source is drawn on top, unless shadow-only)
    // and the area the shadow reads from: maxOutput moved by -offset and outset by the blur's
    // support of about 3 sigma.
    const int supportX = (int)std::ceil(3.f * sx);
    const int supportY = (int)std::ceil(3.f * sy);
    skif::LayerSpace<SkIRect> work = maxOutput;
    skif::LayerSpace<SkIRect> shadowRequired = maxOutput;
    shadowRequired.offset(skif::LayerSpace<skif::IVector>({-dx, -dy}));
    shadowRequired.outset(skif::LayerSpace<SkISize>({supportX, supportY}));
    work.join(shadowRequired);
    const int workW = work.width();
    const int workH = work.height();
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
    const int ox = srcOrigin.x() - work.left();
    const int oy = srcOrigin.y() - work.top();
    SkIRect land = SkIRect::MakeXYWH(ox, oy, srcImg->width(), srcImg->height());
    if (land.intersect(SkIRect::MakeWH(workW, workH))) {
        SkPixmap bufPixmap, subset;
        if (srcBuf.peekPixels(&bufPixmap) && bufPixmap.extractSubset(&subset, land)) {
            (void)srcAsImage->readPixels(subset, land.left() - ox, land.top() - oy);
        }
    }

    // Only the alpha plane is blurred, which is about a quarter of the memory traffic of an
    // RGBA blur.
    std::vector<uint8_t> alphaPlane((size_t)workW * workH);
    std::vector<uint8_t> tmp((size_t)workW * workH);
    SkPixmap srcPixmap;
    srcBuf.peekPixels(&srcPixmap);
    for (int y = 0; y < workH; ++y) {
        const uint32_t* row = srcPixmap.addr32(0, y);
        uint8_t* alphaRow = alphaPlane.data() + (size_t)y * workW;
        for (int x = 0; x < workW; ++x) {
            alphaRow[x] = (uint8_t)SkGetPackedA32(row[x]);
        }
    }
    blur_alpha_plane(alphaPlane.data(), tmp.data(), workW, workH, sx, sy);

    SkPixmap dstPixmap;
    dstBuf.peekPixels(&dstPixmap);
    composite_shadow(srcPixmap, alphaPlane.data(), dstPixmap, flood, dx, dy, shadowOnly);

    SkBitmap out;
    if (!dstBuf.extractSubset(&out, SkIRect::MakeXYWH(maxOutput.left() - work.left(),
                                                      maxOutput.top() - work.top(),
                                                      maxOutput.width(), maxOutput.height()))) {
        return std::nullopt;
    }
    out.setImmutable();

    auto special = SkSpecialImages::MakeFromRaster(SkIRect::MakeWH(out.width(), out.height()),
                                                   out, SkSurfaceProps{});
    return skif::FilterResult(std::move(special), maxOutput.topLeft());
}

// The composed filter graph. SkNativeDropShadowImageFilter falls back to it when the native path
// can't be used, and legacy SKPs are still deserialized into it.
static sk_sp<SkImageFilter> make_drop_shadow_graph(SkVector offset,
                                                   SkSize sigma,
                                                   SkColor4f color,
                                                   sk_sp<SkColorSpace> colorSpace,
                                                   bool shadowOnly,
                                                   sk_sp<SkImageFilter> input,
                                                   const std::optional<SkRect>& crop) {
    // A drop shadow blurs the input, filters it to be the solid color + blurred
    // alpha, and then offsets it. If it's not shadow-only, the input is then
    // src-over blended on top. Finally it's cropped to the optional 'crop'.
    sk_sp<SkImageFilter> filter = input;
    filter = SkImageFilters::Blur(sigma.fWidth, sigma.fHeight, std::move(filter));
    filter = SkImageFilters::ColorFilter(
            SkColorFilters::Blend(color, std::move(colorSpace), SkBlendMode::kSrcIn),
            std::move(filter));
    // TODO: Offset should take SkSamplingOptions too, but kLinear filtering is needed to hide
    // nearest-neighbor sampling artifacts from fractional offsets applied post-blur.
    filter = SkImageFilters::MatrixTransform(SkMatrix::Translate(offset.fX, offset.fY),
                                             SkFilterMode::kLinear,
                                             std::move(filter));
    if (!shadowOnly) {
#if defined(SK_LEGACY_BLEND_FOR_DROP_SHADOWS)
        filter = SkImageFilters::Blend(
                SkBlendMode::kSrcOver, std::move(filter), std::move(input));
#else
        // Merge is visually equivalent to Blend(kSrcOver) but draws each child independently,
        // whereas Blend() fills the union of the child bounds with a single shader evaluation.
        // Since we know the original and the offset blur will have somewhat disjoint bounds, a
        // Blend() shader would force evaluating tile edge conditions for each, while merge lets us
        // avoid that.
        filter = SkImageFilters::Merge(std::move(filter), std::move(input));
#endif
    }
    if (crop) {
        filter = SkImageFilters::Crop(*crop, std::move(filter));
    }
    return filter;
}

// A drop shadow filter that renders raster N32 input directly (see apply_native_drop_shadow)
// and evaluates the composed filter graph for everything else.
class SkNativeDropShadowImageFilter final : public SkImageFilter_Base {
public:
    SkNativeDropShadowImageFilter(SkPoint offset, SkSize sigma, SkColor4f color,
                                  sk_sp<SkColorSpace> colorSpace, bool shadowOnly,
                                  sk_sp<SkImageFilter> input)
            : SkImageFilter_Base(&input, 1)
            , fOffset(offset)
            , fSigma(sigma)
            , fColor(color)
            , fColorSpace(std::move(colorSpace))
            , fShadowOnly(shadowOnly) {}

    SkRect computeFastBounds(const SkRect& src) const override;

protected:
    void flatten(SkWriteBuffer&) const override;

private:
    friend void ::SkRegisterLegacyDropShadowImageFilterFlattenable();
    SK_FLATTENABLE_HOOKS(SkNativeDropShadowImageFilter)

    skif::FilterResult onFilterImage(const skif::Context&) const override;

    skif::LayerSpace<SkIRect> onGetInputLayerBounds(
            const skif::Mapping& mapping,
            const skif::LayerSpace<SkIRect>& desiredOutput,
            std::optional<skif::LayerSpace<SkIRect>> contentBounds) const override;

    std::optional<skif::LayerSpace<SkIRect>> onGetOutputLayerBounds(
            const skif::Mapping& mapping,
            std::optional<skif::LayerSpace<SkIRect>> contentBounds) const override;

    // The blur sigma in layer space, mapped the same way SkMorphologyImageFilter maps its radii.
    SkV2 deviceSigma(const skif::Mapping& mapping) const {
        skif::LayerSpace<SkSize> s = mapping.paramToLayer(fSigma);
        return {std::max(0.f, s.width()), std::max(0.f, s.height())};
    }

    // The offset in layer space, rounded to whole pixels. The magnitude is mapped as a size and
    // the sign is restored afterwards, which is only correct for scale/translate matrices (as
    // used for thumbnails). Rotated matrices would need the offset mapped as a vector.
    skif::LayerSpace<skif::IVector> deviceOffset(const skif::Mapping& mapping) const {
        SkPoint offset = SkPoint(fOffset);
        skif::LayerSpace<SkSize> magnitude = mapping.paramToLayer(
                skif::ParameterSpace<SkSize>(SkSize{std::abs(offset.fX), std::abs(offset.fY)}));
        int dx = (int)lroundf(magnitude.width());
        int dy = (int)lroundf(magnitude.height());
        return skif::LayerSpace<skif::IVector>({offset.fX < 0 ? -dx : dx,
                                                offset.fY < 0 ? -dy : dy});
    }

    skif::LayerSpace<SkISize> blurSupport(SkV2 sigma) const {
        return skif::LayerSpace<SkISize>({(int)std::ceil(3.f * sigma.x),
                                          (int)std::ceil(3.f * sigma.y)});
    }

    skif::LayerSpace<SkIRect> requiredInput(const skif::Mapping& mapping,
                                            skif::LayerSpace<SkIRect> desiredOutput) const {
        SkV2 sigma = this->deviceSigma(mapping);
        skif::LayerSpace<skif::IVector> offset = this->deviceOffset(mapping);
        // The shadow needs the input moved opposite to the offset and outset by the blur support.
        skif::LayerSpace<SkIRect> shadow = desiredOutput;
        shadow.offset(skif::LayerSpace<skif::IVector>({-offset.x(), -offset.y()}));
        shadow.outset(this->blurSupport(sigma));
        if (fShadowOnly) {
            return shadow;
        }
        // The source is drawn on top of the shadow.
        shadow.join(desiredOutput);
        return shadow;
    }

    skif::LayerSpace<SkIRect> outputBounds(const skif::Mapping& mapping,
                                           skif::LayerSpace<SkIRect> contentBounds) const {
        SkV2 sigma = this->deviceSigma(mapping);
        skif::LayerSpace<skif::IVector> offset = this->deviceOffset(mapping);
        // The blur spreads the content, and the result is then offset.
        skif::LayerSpace<SkIRect> shadow = contentBounds;
        shadow.outset(this->blurSupport(sigma));
        shadow.offset(offset);
        if (fShadowOnly) {
            return shadow;
        }
        shadow.join(contentBounds);
        return shadow;
    }

    SkPMColor floodColor() const {
        // The color is used as is, without converting it from fColorSpace to the destination
        // color space. This is exact for sRGB, which is what SVG colors are.
        return SkPreMultiplyColor(fColor.toSkColor());
    }

    skif::ParameterSpace<SkPoint> fOffset;
    skif::ParameterSpace<SkSize> fSigma;
    SkColor4f fColor;
    sk_sp<SkColorSpace> fColorSpace;
    bool fShadowOnly;
};

sk_sp<SkImageFilter> make_drop_shadow(SkPoint offset, SkSize sigma, SkColor4f color,
                                      sk_sp<SkColorSpace> colorSpace, bool shadowOnly,
                                      sk_sp<SkImageFilter> input,
                                      const SkImageFilters::CropRect& cropRect) {
    sk_sp<SkImageFilter> filter(new SkNativeDropShadowImageFilter(
            offset, sigma, color, std::move(colorSpace), shadowOnly, std::move(input)));
    if (cropRect) {
        filter = SkImageFilters::Crop(*cropRect, std::move(filter));
    }
    return filter;
}

sk_sp<SkFlattenable> legacy_drop_shadow_create_proc(SkReadBuffer& buffer) {
    if (!buffer.isVersionLT(SkPicturePriv::Version::kDropShadowImageFilterComposition)) {
        // SKPs created with this version or newer just serialize the image filter composition that
        // is equivalent to a drop-shadow, instead of a single dedicated flattenable for the effect.
        return nullptr;
    }

    auto [child, cropRect] = SkImageFilter_Base::Unflatten(buffer);

    SkScalar dx = buffer.readScalar();
    SkScalar dy = buffer.readScalar();
    SkScalar sigmaX = buffer.readScalar();
    SkScalar sigmaY = buffer.readScalar();
    SkColor4f color = SkColor4f::FromColor(buffer.readColor());

    // For backwards compatibility, the shadow mode had been saved as an enum cast to a 32LE int,
    // where shadow-and-foreground was 0 and shadow-only was 1. Other than the number of bits, this
    // is equivalent to the bool that SkDropShadowImageFilter now uses.
    bool shadowOnly = SkToBool(buffer.read32LE(1));
    return make_drop_shadow_graph({dx, dy}, {sigmaX, sigmaY}, color, /*colorSpace=*/nullptr,
                                  shadowOnly, std::move(child), cropRect);
}

} // anonymous namespace

sk_sp<SkImageFilter> SkImageFilters::DropShadow(
        SkScalar dx, SkScalar dy, SkScalar sigmaX, SkScalar sigmaY, SkColor4f color,
        sk_sp<SkColorSpace> colorSpace,
        sk_sp<SkImageFilter> input, const CropRect& cropRect) {
    return make_drop_shadow({dx, dy}, {sigmaX, sigmaY}, color, std::move(colorSpace),
                            /*shadowOnly=*/false, std::move(input), cropRect);
}

sk_sp<SkImageFilter> SkImageFilters::DropShadowOnly(
        SkScalar dx, SkScalar dy, SkScalar sigmaX, SkScalar sigmaY, SkColor4f color,
        sk_sp<SkColorSpace> colorSpace, sk_sp<SkImageFilter> input, const CropRect& cropRect) {
    return make_drop_shadow({dx, dy}, {sigmaX, sigmaY}, color, std::move(colorSpace),
                            /*shadowOnly=*/true, std::move(input), cropRect);
}

// TODO (michaelludwig) - Remove after grace period for SKPs to stop using old create proc
void SkRegisterLegacyDropShadowImageFilterFlattenable() {
    SK_REGISTER_FLATTENABLE(SkNativeDropShadowImageFilter);
    SkFlattenable::Register("SkDropShadowImageFilter", legacy_drop_shadow_create_proc);
    SkFlattenable::Register("SkDropShadowImageFilterImpl", legacy_drop_shadow_create_proc);
}

sk_sp<SkFlattenable> SkNativeDropShadowImageFilter::CreateProc(SkReadBuffer& buffer) {
    SK_IMAGEFILTER_UNFLATTEN_COMMON(common, 1);
    SkScalar dx = buffer.readScalar();
    SkScalar dy = buffer.readScalar();
    SkScalar sigmaX = buffer.readScalar();
    SkScalar sigmaY = buffer.readScalar();
    SkColor4f color;
    buffer.readColor4f(&color);
    bool shadowOnly = buffer.readBool();
    // The color space isn't serialized, so it's read back as nullptr (sRGB).
    if (shadowOnly) {
        return SkImageFilters::DropShadowOnly(dx, dy, sigmaX, sigmaY, color,
                                              /*colorSpace=*/nullptr, common.getInput(0),
                                              common.cropRect());
    }
    return SkImageFilters::DropShadow(dx, dy, sigmaX, sigmaY, color, /*colorSpace=*/nullptr,
                                      common.getInput(0), common.cropRect());
}

void SkNativeDropShadowImageFilter::flatten(SkWriteBuffer& buffer) const {
    this->SkImageFilter_Base::flatten(buffer);
    SkPoint offset = SkPoint(fOffset);
    SkSize sigma = SkSize(fSigma);
    buffer.writeScalar(offset.fX);
    buffer.writeScalar(offset.fY);
    buffer.writeScalar(sigma.fWidth);
    buffer.writeScalar(sigma.fHeight);
    buffer.writeColor4f(fColor);
    buffer.writeBool(fShadowOnly);
}

///////////////////////////////////////////////////////////////////////////////

skif::FilterResult SkNativeDropShadowImageFilter::onFilterImage(const skif::Context& ctx) const {
    const skif::Mapping& mapping = ctx.mapping();

    skif::LayerSpace<SkIRect> requiredInput = this->requiredInput(mapping, ctx.desiredOutput());
    skif::FilterResult childOutput =
            this->getChildOutput(0, ctx.withNewDesiredOutput(requiredInput));

    skif::LayerSpace<SkIRect> maxOutput = this->outputBounds(mapping, childOutput.layerBounds());
    if (!maxOutput.intersect(ctx.desiredOutput())) {
        return {};
    }

    SkV2 sigma = this->deviceSigma(mapping);
    skif::LayerSpace<skif::IVector> offset = this->deviceOffset(mapping);

    if (auto handled = apply_native_drop_shadow(ctx, childOutput, sigma.x, sigma.y,
                                                offset.x(), offset.y(), this->floodColor(),
                                                fShadowOnly, maxOutput)) {
        return *handled;
    }

    // Evaluate the composed graph for input the native path can't handle (non-raster, F16, etc.).
    sk_sp<SkImageFilter> graph = make_drop_shadow_graph(
            SkPoint(fOffset), SkSize(fSigma), fColor, fColorSpace, fShadowOnly,
            sk_ref_sp(this->getInput(0)), std::nullopt);
    return as_IFB(graph.get())->filterImage(ctx);
}

skif::LayerSpace<SkIRect> SkNativeDropShadowImageFilter::onGetInputLayerBounds(
        const skif::Mapping& mapping,
        const skif::LayerSpace<SkIRect>& desiredOutput,
        std::optional<skif::LayerSpace<SkIRect>> contentBounds) const {
    skif::LayerSpace<SkIRect> requiredInput = this->requiredInput(mapping, desiredOutput);
    return this->getChildInputLayerBounds(0, mapping, requiredInput, contentBounds);
}

std::optional<skif::LayerSpace<SkIRect>> SkNativeDropShadowImageFilter::onGetOutputLayerBounds(
        const skif::Mapping& mapping,
        std::optional<skif::LayerSpace<SkIRect>> contentBounds) const {
    auto childOutput = this->getChildOutputLayerBounds(0, mapping, contentBounds);
    if (childOutput) {
        return this->outputBounds(mapping, *childOutput);
    }
    return skif::LayerSpace<SkIRect>::Unbounded();
}

SkRect SkNativeDropShadowImageFilter::computeFastBounds(const SkRect& src) const {
    SkRect bounds = this->getInput(0) ? this->getInput(0)->computeFastBounds(src) : src;
    SkPoint offset = SkPoint(fOffset);
    SkSize sigma = SkSize(fSigma);
    SkRect shadow = bounds;
    shadow.outset(3.f * sigma.fWidth, 3.f * sigma.fHeight);
    shadow.offset(offset.fX, offset.fY);
    if (fShadowOnly) {
        return shadow;
    }
    bounds.join(shadow);
    return bounds;
}
