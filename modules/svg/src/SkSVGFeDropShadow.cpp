/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/effects/SkImageFilters.h"
#include "modules/svg/include/SkSVGAttributeParser.h"
#include "modules/svg/include/SkSVGFeDropShadow.h"
#include "modules/svg/include/SkSVGFilterContext.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGValue.h"

bool SkSVGFeDropShadow::parseAndSetAttribute(const char* name, const char* value) {
    return INHERITED::parseAndSetAttribute(name, value) ||
           this->setDx(SkSVGAttributeParser::parse<SkSVGNumberType>("dx", name, value)) ||
           this->setDy(SkSVGAttributeParser::parse<SkSVGNumberType>("dy", name, value)) ||
           this->setStdDeviation(SkSVGAttributeParser::parse<StdDeviation>(
                   "stdDeviation", name, value));
}

SkColor SkSVGFeDropShadow::resolveFloodColor(const SkSVGRenderContext& ctx) const {
    const auto floodColor = this->getFloodColor();
    const auto floodOpacity = this->getFloodOpacity();
    // Uninherited presentation attributes should have a concrete value by now.
    if (!floodColor.isValue() || !floodOpacity.isValue()) {
        SkDebugf("unhandled: flood-color or flood-opacity has no value\n");
        return SK_ColorBLACK;
    }

    const SkColor color = ctx.resolveSvgColor(*floodColor);
    return SkColorSetA(color, SkScalarRoundToInt(*floodOpacity * 255));
}

sk_sp<SkImageFilter> SkSVGFeDropShadow::onMakeImageFilter(const SkSVGRenderContext& ctx,
                                                          const SkSVGFilterContext& fctx) const {
    const auto d = SkV2{this->getDx(), this->getDy()} *
                   ctx.transformForCurrentOBB(fctx.primitiveUnits()).scale;
    const auto sigma = SkV2{fStdDeviation.fX, fStdDeviation.fY}
                     * ctx.transformForCurrentOBB(fctx.primitiveUnits()).scale;

    return SkImageFilters::DropShadow(
        d.x, d.y,
        sigma.x, sigma.y,
        resolveFloodColor(ctx),
        fctx.resolveInput(ctx, this->getIn(), this->resolveColorspace(ctx, fctx)),
        this->resolveFilterSubregion(ctx, fctx));
}
