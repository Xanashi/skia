/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/svg/include/SkSVGAttribute.h"

SkSVGPresentationAttributes SkSVGPresentationAttributes::MakeInitial() {
    SkSVGPresentationAttributes result;

    result.fId.set(SkSVGStringType());
    result.fClass.set(SkSVGStringType());
    result.fStyle.set(SkSVGStringType());

    result.fFill.set(SkSVGPaint(SkSVGColor(SK_ColorBLACK)));
    result.fFillOpacity.set(SkSVGNumberType(1));
    result.fFillRule.set(SkSVGFillRule(SkSVGFillRule::Type::kNonZero));
    result.fClipRule.set(SkSVGFillRule(SkSVGFillRule::Type::kNonZero));
    result.fCornerRadius.set(SkSVGLength(0));

    result.fStroke.set(SkSVGPaint(SkSVGPaint::Type::kNone));
    result.fStrokeDashArray.set(SkSVGDashArray(SkSVGDashArray::Type::kNone));
    result.fStrokeDashOffset.set(SkSVGLength(0));
    result.fStrokeLineCap.set(SkSVGLineCap::kButt);
    result.fStrokeLineJoin.set(SkSVGLineJoin(SkSVGLineJoin::Type::kMiter));
    result.fStrokeMiterLimit.set(SkSVGNumberType(4));
    result.fStrokeOpacity.set(SkSVGNumberType(1));
    result.fStrokeWidth.set(SkSVGLength(1));

    result.fBlendMode.set(SkSVGBlendMode());
    result.fVisibility.set(SkSVGVisibility(SkSVGVisibility::Type::kVisible));

    result.fColor.set(SkSVGColorType(SK_ColorBLACK));
    result.fColorInterpolation.set(SkSVGColorspace::kSRGB);
    result.fColorInterpolationFilters.set(SkSVGColorspace::kLinearRGB);

    result.fFontFamily.init("Sans");
    result.fFontStyle.init(SkSVGFontStyle::Type::kNormal);
    result.fFontSize.init(SkSVGLength(24));
    result.fFontWeight.init(SkSVGFontWeight::Type::kNormal);
    result.fTextAnchor.init(SkSVGTextAnchor::Type::kStart);
    result.fTextTransform.init(SkSVGTextTransform::Type::kNone);

    result.fDisplay.init(SkSVGDisplay::kInline);

    result.fStopColor.set(SkSVGColor(SK_ColorBLACK));
    result.fStopOpacity.set(SkSVGNumberType(1));
    result.fFloodColor.set(SkSVGColor(SK_ColorBLACK));
    result.fFloodOpacity.set(SkSVGNumberType(1));
    result.fLightingColor.set(SkSVGColor(SK_ColorWHITE));

    result.fContextFill.set(SkSVGPaint(SkSVGPaint::Type::kNone));
    result.fContextStroke.set(SkSVGPaint(SkSVGPaint::Type::kNone));

    return result;
}
