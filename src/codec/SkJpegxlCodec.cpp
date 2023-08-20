/*
 * Copyright 2021 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/codec/SkJpegxlCodec.h"

#include "include/codec/SkCodec.h"
#include "include/codec/SkJpegxlDecoder.h"
#include "include/core/SkColorType.h"
#include "include/core/SkData.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkStream.h"
#include "include/core/SkTypes.h"
#include "include/private/SkEncodedInfo.h"
#include "include/private/base/SkTFitsIn.h"
#include "include/private/base/SkTemplates.h"
#include "include/private/base/SkTo.h"
#include "modules/skcms/skcms.h"
#include "src/codec/SkFrameHolder.h"
#include "src/codec/SkSwizzler.h"
#include "src/core/SkStreamPriv.h"

#include "jxl/codestream_header.h"
#include "jxl/decode.h"
#include "jxl/decode_cxx.h"
#include "jxl/types.h"
#include <jxl/resizable_parallel_runner.h>
#include <jxl/resizable_parallel_runner_cxx.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace {

class Frame : public SkFrame {
public:
    explicit Frame(int i, SkEncodedInfo::Alpha alpha) : INHERITED(i), fReportedAlpha(alpha) {}
    SkEncodedInfo::Alpha onReportedAlpha() const override { return fReportedAlpha; }

private:
    const SkEncodedInfo::Alpha fReportedAlpha;

    using INHERITED = SkFrame;
};

}  // namespace

bool SkJpegxlCodec::IsJpegxl(const void* buffer, size_t bytesRead) {
    JxlSignature result = JxlSignatureCheck(reinterpret_cast<const uint8_t*>(buffer), bytesRead);
    return (result == JXL_SIG_CODESTREAM) || (result == JXL_SIG_CONTAINER);
}

class SkJpegxlCodecPriv : public SkFrameHolder {
public:
    SkJpegxlCodecPriv() : 
        fParallelRunner(JxlResizableParallelRunnerMake(nullptr)),
        fDecoder(JxlDecoderMake(/* memory_manager= */ nullptr)) {}
    JxlResizableParallelRunnerPtr fParallelRunner; // Multi-threaded parallel runner
    JxlDecoderPtr fDecoder;  // unique_ptr with custom destructor
    JxlBasicInfo fInfo;
    bool fSeenAllFrames = false;
    std::vector<Frame> fFrames;
    int fLastProcessedFrame = SkCodec::kNoFrame;
    void* fDst;
    size_t fPixelShift;
    size_t fRowBytes;
    SkColorType fDstColorType;

protected:
    const SkFrame* onGetFrame(int i) const override {
        SkASSERT(i >= 0 && static_cast<size_t>(i) < fFrames.size());
        return static_cast<const SkFrame*>(&fFrames[i]);
    }
};

SkJpegxlCodec::SkJpegxlCodec(std::unique_ptr<SkJpegxlCodecPriv> codec,
                             SkEncodedInfo&& info,
                             std::unique_ptr<SkStream> stream,
                             sk_sp<SkData> data)
        : INHERITED(std::move(info), skcms_PixelFormat_RGBA_16161616LE, std::move(stream))
        , fCodec(std::move(codec))
        , fData(std::move(data)) {}

std::unique_ptr<SkCodec> SkJpegxlCodec::MakeFromStream(std::unique_ptr<SkStream> stream,
                                                       Result* result) {
    SkASSERT(result);
    if (!stream) {
        *result = SkCodec::kInvalidInput;
        return nullptr;
    }
    *result = kInternalError;
    // Either wrap or copy stream data.
    sk_sp<SkData> data = nullptr;
    if (stream->getMemoryBase()) {
        data = SkData::MakeWithoutCopy(stream->getMemoryBase(), stream->getLength());
    } else {
        data = SkCopyStreamToData(stream.get());
        // Data is copied; stream can be released now.
        stream.reset(nullptr);
    }

    auto priv = std::make_unique<SkJpegxlCodecPriv>();
    JxlDecoder* dec = priv->fDecoder.get();
    void* runner = priv->fParallelRunner.get();

    // Only query metadata this time.
    if (JxlDecoderSubscribeEvents(dec, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING) !=
        JXL_DEC_SUCCESS) {
        // Fresh instance must accept request for subscription.
        SkDEBUGFAIL("libjxl returned unexpected status");
        return nullptr;
    }

    if (JxlDecoderSetParallelRunner(dec, JxlResizableParallelRunner, runner) !=
        JXL_DEC_SUCCESS) {
        SkDEBUGFAIL("libjxl status: JxlDecoderSetParallelRunner failed");
        return nullptr;
    }
    
    if (JxlDecoderSetInput(dec, data->bytes(), data->size()) != JXL_DEC_SUCCESS) {
        // Fresh instance must accept first chunk of input.
        SkDEBUGFAIL("libjxl returned unexpected status");
        return nullptr;
    }

    JxlDecoderCloseInput(dec);

    int32_t width = 0, height = 0;
    SkEncodedInfo::Alpha alpha = SkEncodedInfo::kUnpremul_Alpha;
    SkEncodedInfo::Color color = SkEncodedInfo::kRGBA_Color;
    std::unique_ptr<SkEncodedInfo::ICCProfile> profile = nullptr;

    while (true) {
        JxlDecoderStatus status = JxlDecoderProcessInput(dec);

        if (status == JXL_DEC_ERROR) {
            *result = kInvalidInput;
            return nullptr;
        } 
        else if (status == JXL_DEC_NEED_MORE_INPUT) {
            *result = kIncompleteInput;
            return nullptr;
        }
        else if (status == JXL_DEC_BASIC_INFO) {
            JxlBasicInfo& info = priv->fInfo;
            status = JxlDecoderGetBasicInfo(dec, &info);
            if (status != JXL_DEC_SUCCESS) {
                // Current event is "JXL_DEC_BASIC_INFO" -> can't fail.
                SkDEBUGFAIL("libjxl returned unexpected status");
                return nullptr;
            }

            // Check that image dimensions are not too large.
            if (!SkTFitsIn<int32_t>(info.xsize) || !SkTFitsIn<int32_t>(info.ysize)) {
                *result = kInvalidInput;
                return nullptr;
            }

            width = SkTo<int32_t>(info.xsize);
            height = SkTo<int32_t>(info.ysize);

            JxlResizableParallelRunnerSetThreads(
                    runner, JxlResizableParallelRunnerSuggestThreads(info.xsize, info.ysize));

            bool hasAlpha = (info.alpha_bits != 0);
            bool isGray = (info.num_color_channels == 1);

            alpha = hasAlpha ? SkEncodedInfo::kUnpremul_Alpha : SkEncodedInfo::kOpaque_Alpha;
            if (hasAlpha) {
                color = isGray ? SkEncodedInfo::kGrayAlpha_Color : SkEncodedInfo::kRGBA_Color;
            } else {
                color = isGray ? SkEncodedInfo::kGray_Color : SkEncodedInfo::kRGB_Color;
            }
        }
        else if (status == JXL_DEC_COLOR_ENCODING) {
            size_t iccSize = 0;
            // TODO(eustas): format field is currently ignored by decoder.
            if (JxlDecoderGetICCProfileSize(dec, /* format = */ nullptr, 
                JXL_COLOR_PROFILE_TARGET_DATA, &iccSize) != JXL_DEC_SUCCESS) {
                // Likely incompatible colorspace.
                iccSize = 0;
            }
            
            if (iccSize) {
                auto icc = SkData::MakeUninitialized(iccSize);
                // TODO(eustas): format field is currently ignored by decoder.
                status = JxlDecoderGetColorAsICCProfile(
                        dec,
                        /* format = */ nullptr,
                        JXL_COLOR_PROFILE_TARGET_DATA,
                        reinterpret_cast<uint8_t*>(icc->writable_data()),
                        iccSize);
                if (status != JXL_DEC_SUCCESS) {
                    // Current event is JXL_DEC_COLOR_ENCODING -> can't fail.
                    SkDEBUGFAIL("libjxl returned unexpected status");
                    return nullptr;
                }
                profile = SkEncodedInfo::ICCProfile::Make(std::move(icc));
            }
        } 
        else if (status == JXL_DEC_SUCCESS) {
            // All decoding successfully finished.
            // It's not required to call JxlDecoderReleaseInput(dec) here since
            // the decoder will be destroyed.
            break;
        } 
        else {
            SkDEBUGFAIL("libjxl returned unexpected status");
            return nullptr;
        }
    }    

    int bitsPerChannel = 16;

    *result = kSuccess;
    SkEncodedInfo encodedInfo =
            SkEncodedInfo::Make(width, height, color, alpha, bitsPerChannel, std::move(profile));

    return std::unique_ptr<SkCodec>(new SkJpegxlCodec(
            std::move(priv), std::move(encodedInfo), std::move(stream), std::move(data)));
}

SkCodec::Result SkJpegxlCodec::onGetPixels(const SkImageInfo& dstInfo,
                                           void* dst,
                                           size_t rowBytes,
                                           const Options& options,
                                           int* rowsDecodedPtr) {
    // TODO(eustas): implement
    if (options.fSubset) {
        return kUnimplemented;
    }
    auto& codec = *fCodec.get();
    const int index = options.fFrameIndex;
    SkASSERT(0 == index || static_cast<size_t>(index) < codec.fFrames.size());
    auto* dec = codec.fDecoder.get();

    if ((codec.fLastProcessedFrame >= index) || (codec.fLastProcessedFrame = SkCodec::kNoFrame)) {
        codec.fLastProcessedFrame = SkCodec::kNoFrame;
        JxlDecoderRewind(dec);

        if (JxlDecoderSubscribeEvents(dec, /*JXL_DEC_FRAME | */JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS) {
            // Fresh decoder instance (after rewind) must accept subscription request.
            SkDEBUGFAIL("libjxl returned unexpected status");
            return kInternalError;
        }
        if (JxlDecoderSetInput(dec, fData->bytes(), fData->size()) != JXL_DEC_SUCCESS) {
            // Fresh decoder instance (after rewind) must accept first data chunk.
            SkDEBUGFAIL("libjxl returned unexpected status");
            return kInternalError;
        }

        JxlDecoderCloseInput(dec);
        SkASSERT(codec.fLastProcessedFrame + 1 == 0);
    }

    int nextFrame = codec.fLastProcessedFrame + 1;
    if (nextFrame < index) {
        JxlDecoderSkipFrames(dec, index - nextFrame);
    }

    while (true) {
        JxlDecoderStatus status = JxlDecoderProcessInput(dec);

        if (status == JXL_DEC_ERROR) {
            return kInternalError;
        } else if (status == JXL_DEC_NEED_MORE_INPUT) {
            return kInternalError;
        } else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            codec.fDst = dst;
            codec.fRowBytes = rowBytes;

            // Handle Grayscale
            uint32_t numChannels = fCodec->fDstColorType == kGray_8_SkColorType ? 1 : 4;
            JxlPixelFormat format = {numChannels, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, /* align = */ 0};

            if (JxlDecoderSetImageOutBuffer(dec, &format, codec.fDst, codec.fInfo.ysize * rowBytes) !=
                JXL_DEC_SUCCESS) {
                SkDEBUGFAIL("libjxl returned unexpected status");
                return kInternalError;
            }
        } 
        else if (status == JXL_DEC_FULL_IMAGE) {
            // Nothing to do. Do not yet return. If the image is an animation, more
            // full frames may be decoded. This example only keeps the last one.

            // TODO(eustas): actually, frame is not completely processed; for streaming / partial decoding
            //               we should also add a flag that "last processed frame" is still incomplete, and
            //               flip that flag when frame decoding is over.
            codec.fLastProcessedFrame = index;
        } 
        else if (status == JXL_DEC_SUCCESS) {
            // All decoding successfully finished.
            // It's not required to call JxlDecoderReleaseInput(dec) here since
            // the decoder will be destroyed.
            break;
        } 
        else {
            SkDEBUGFAIL("libjxl returned unexpected status");
            return kInternalError;
        }
    }

    if (fCodec->fDstColorType == kBGRA_8888_SkColorType) {
        SkOpts::RGBA_to_BGRA((uint32_t*)codec.fDst, (const uint32_t*)(codec.fDst), codec.fInfo.ysize * codec.fInfo.xsize);
    }

    // TODO(eustas): currently it is supposed that complete input is accessible;
    //               when streaming support is added JXL_DEC_NEED_MORE_INPUT would also
    //               become a legal outcome; amount of decoded scanlines should be calculated
    //               based on callback invocations / render-pipeline API.
    *rowsDecodedPtr = dstInfo.height();

    return kSuccess;
}

bool SkJpegxlCodec::onRewind() {
    JxlDecoderRewind(fCodec->fDecoder.get());
    return true;
}

bool SkJpegxlCodec::conversionSupported(const SkImageInfo& dstInfo, bool srcIsOpaque,
                                        bool needsColorXform) {
    fCodec->fDstColorType = dstInfo.colorType();
    switch (dstInfo.colorType()) {
        case kRGBA_8888_SkColorType:
            return true;  // memcpy
        case kBGRA_8888_SkColorType:
            return true;  // rgba->bgra
        case kRGBA_F16_SkColorType:
            SkASSERT(needsColorXform);  // TODO(eustas): not necessary for JXL.
            return true;  // memcpy
        case kGray_8_SkColorType:
            return true;  // memcpy

        // TODO(eustas): implement
        case kRGB_565_SkColorType:
            return false;
        case kAlpha_8_SkColorType:
            return false;

        default:
            return false;
    }
    return true;
}

//void SkJpegxlCodec::imageOutCallback(void* opaque, size_t x, size_t y,
//                                     size_t num_pixels, const void* pixels) {
//    SkJpegxlCodec* instance = reinterpret_cast<SkJpegxlCodec*>(opaque);
//    auto& codec = *instance->fCodec.get();
//    size_t offset = y * codec.fRowBytes + (x << codec.fPixelShift);
//    void* dst = SkTAddOffset<void>(codec.fDst, offset);
//    if (instance->colorXform()) {
//        instance->applyColorXform(dst, pixels, num_pixels);
//        return;
//    }
//    switch (codec.fDstColorType) {
//        case kRGBA_8888_SkColorType:
//            memcpy(dst, pixels, 4 * num_pixels);
//            return;
//        case kBGRA_8888_SkColorType:
//            SkOpts::RGBA_to_bgrA((uint32_t*) dst, (const uint32_t*)(pixels), num_pixels);
//            return;
//        case kRGBA_F16_SkColorType:
//            memcpy(dst, pixels, 8 * num_pixels);
//            return;
//        case kGray_8_SkColorType:
//            memcpy(dst, pixels, num_pixels);
//            return;
//        default:
//            SK_ABORT("Selected output format is not supported yet");
//            return;
//    }
//}

bool SkJpegxlCodec::scanFrames() {
    auto runner = JxlResizableParallelRunnerMake(nullptr);
    auto decoder = JxlDecoderMake(/* memory_manager = */ nullptr);
    JxlDecoder* dec = decoder.get();
    auto* frameHolder = fCodec.get();
    auto& frames = frameHolder->fFrames;
    const auto& info = fCodec->fInfo;
    frames.clear();

    auto alpha = (info.alpha_bits != 0) ? SkEncodedInfo::Alpha::kUnpremul_Alpha
                                        : SkEncodedInfo::Alpha::kOpaque_Alpha;

    auto status = JxlDecoderSubscribeEvents(dec, JXL_DEC_FRAME);
    if (status != JXL_DEC_SUCCESS) {
        // Fresh instance must accept request for subscription.
        SkDEBUGFAIL("libjxl returned unexpected status");
        return true;
    }

    status = JxlDecoderSetParallelRunner(dec, JxlResizableParallelRunner, runner.get());
    if (status != JXL_DEC_SUCCESS) {
        SkDEBUGFAIL("libjxl status: JxlDecoderSetParallelRunner failed");
        return true;
    }

    status = JxlDecoderSetInput(dec, fData->bytes(), fData->size());
    if (status != JXL_DEC_SUCCESS) {
        // Fresh instance must accept first input chunk.
        SkDEBUGFAIL("libjxl returned unexpected status");
        return true;
    }

    while (true) {
        status = JxlDecoderProcessInput(dec);
        switch (status) {
            case JXL_DEC_FRAME: {
                size_t frameId = frames.size();
                JxlFrameHeader frameHeader;
                if (JxlDecoderGetFrameHeader(dec, &frameHeader) != JXL_DEC_SUCCESS) {
                    return true;
                }
                frames.emplace_back(static_cast<int>(frameId), alpha);
                auto& frame = frames.back();
                // TODO(eustas): for better consistency we need to track total duration and report
                //               frame duration as delta to previous frame.
                int duration = (1000 * frameHeader.duration * info.animation.tps_denominator) /
                               info.animation.tps_numerator;
                frame.setDuration(duration);
                frameHolder->setAlphaAndRequiredFrame(&frame);
                break;
            }
            case JXL_DEC_SUCCESS: {
                return true;
            }
            default: {
                return false;
            }
        }
    }
}

int SkJpegxlCodec::onGetFrameCount() {
    if (!fCodec->fInfo.have_animation) {
        return 1;
    }

    if (!fCodec->fSeenAllFrames) {
        fCodec->fSeenAllFrames = scanFrames();
    }

    return fCodec->fFrames.size();
}

bool SkJpegxlCodec::onGetFrameInfo(int index, FrameInfo* frameInfo) const {
    if (index < 0) {
        return false;
    }
    if (static_cast<size_t>(index) >= fCodec->fFrames.size()) {
        return false;
    }
    fCodec->fFrames[index].fillIn(frameInfo, true);
    return true;
}

int SkJpegxlCodec::onGetRepetitionCount() {
    JxlBasicInfo& info = fCodec->fInfo;
    if (!info.have_animation) {
        return 0;
    }

    if (info.animation.num_loops == 0) {
        return kRepetitionCountInfinite;
    }

    if (SkTFitsIn<int>(info.animation.num_loops)) {
        return info.animation.num_loops - 1;
    }

    // Largest "non-infinite" value.
    return std::numeric_limits<int>::max();
}

const SkFrameHolder* SkJpegxlCodec::getFrameHolder() const {
    return fCodec.get();
}

// TODO(eustas): implement
// SkCodec::Result SkJpegxlCodec::onStartScanlineDecode(
//     const SkImageInfo& /*dstInfo*/, const Options& /*options*/) { return kUnimplemented; }

// TODO(eustas): implement
// SkCodec::Result SkJpegxlCodec::onStartIncrementalDecode(
//     const SkImageInfo& /*dstInfo*/, void*, size_t, const Options&) { return kUnimplemented; }

// TODO(eustas): implement
// SkCodec::Result SkJpegxlCodec::onIncrementalDecode(int*) { return kUnimplemented; }

// TODO(eustas): implement
// bool SkJpegxlCodec::onSkipScanlines(int /*countLines*/) { return false; }

// TODO(eustas): implement
// int SkJpegxlCodec::onGetScanlines(
//     void* /*dst*/, int /*countLines*/, size_t /*rowBytes*/) { return 0; }

// TODO(eustas): implement
// SkSampler* SkJpegxlCodec::getSampler(bool /*createIfNecessary*/) { return nullptr; }

namespace SkJpegxlDecoder {
bool IsJpegxl(const void* data, size_t len) {
    return SkJpegxlCodec::IsJpegxl(data, len);
}

std::unique_ptr<SkCodec> Decode(std::unique_ptr<SkStream> stream,
                                SkCodec::Result* outResult,
                                SkCodecs::DecodeContext) {
    SkCodec::Result resultStorage;
    if (!outResult) {
        outResult = &resultStorage;
    }
    return SkJpegxlCodec::MakeFromStream(std::move(stream), outResult);
}

std::unique_ptr<SkCodec> Decode(sk_sp<SkData> data,
                                SkCodec::Result* outResult,
                                SkCodecs::DecodeContext) {
    if (!data) {
        if (outResult) {
            *outResult = SkCodec::kInvalidInput;
        }
        return nullptr;
    }
    return Decode(SkMemoryStream::Make(std::move(data)), outResult, nullptr);
}
}  // namespace SkJpegDecoder
