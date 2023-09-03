/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkBlendMode.h"
#include "include/effects/SkImageFilters.h"
#include "modules/svg/include/SkSVGAttributeParser.h"
#include "modules/svg/include/SkSVGFeBlend.h"
#include "modules/svg/include/SkSVGFilterContext.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGValue.h"

bool SkSVGFeBlend::parseAndSetAttribute(const char* name, const char* value) {
    return INHERITED::parseAndSetAttribute(name, value) ||
           this->setIn2(SkSVGAttributeParser::parse<SkSVGFeInputType>("in2", name, value)) ||
           this->setMode(SkSVGAttributeParser::parse<SkSVGBlendMode>("mode", name, value));
}

sk_sp<SkImageFilter> SkSVGFeBlend::onMakeImageFilter(const SkSVGRenderContext& ctx,
                                                     const SkSVGFilterContext& fctx) const {
    const SkRect cropRect = this->resolveFilterSubregion(ctx, fctx);
    const SkBlendMode blendMode = this->getMode().blendMode();
    const SkSVGColorspace colorspace = this->resolveColorspace(ctx, fctx);
    const sk_sp<SkImageFilter> background = fctx.resolveInput(ctx, fIn2, colorspace);
    const sk_sp<SkImageFilter> foreground = fctx.resolveInput(ctx, this->getIn(), colorspace);
    return SkImageFilters::Blend(blendMode, background, foreground, cropRect);
}
