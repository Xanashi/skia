/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkColorSpace.h"
#include "include/effects/SkGradientShader.h"
#include "modules/svg/include/SkSVGLinearGradient.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGValue.h"
#include "modules/svg/include/SkSVGTools.h"

SkSVGLinearGradient::SkSVGLinearGradient() : INHERITED(SkSVGTag::kLinearGradient) {}

bool SkSVGLinearGradient::parseAndSetAttribute(const char* name, const char* value) {
    return INHERITED::parseAndSetAttribute(name, value) ||
           this->setX1(SkSVGAttributeParser::parse<SkSVGLength>("x1", name, value)) ||
           this->setY1(SkSVGAttributeParser::parse<SkSVGLength>("y1", name, value)) ||
           this->setX2(SkSVGAttributeParser::parse<SkSVGLength>("x2", name, value)) ||
           this->setY2(SkSVGAttributeParser::parse<SkSVGLength>("y2", name, value));
}

bool SkSVGLinearGradient::applyAttributes(sk_sp<SkSVGGradient>* gradient) const {
    bool result = INHERITED::applyAttributes(gradient);

    if ((*gradient)->tag() != SkSVGTag::kLinearGradient) {
        return true;
    }

    auto grad = sk_pointer_cast<SkSVGLinearGradient>(*gradient);
    if (!grad.get()) { return true; }

    return result |
           inherit_if_needed(this->fX1, grad->fX1) |
           inherit_if_needed(this->fY1, grad->fY1) |
           inherit_if_needed(this->fX2, grad->fX2) |
           inherit_if_needed(this->fY2, grad->fY2);
}

sk_sp<SkShader> SkSVGLinearGradient::onMakeShader(const SkSVGRenderContext& ctx,
                                                  const SkColor4f* colors, const SkScalar* pos,
                                                  int count, SkTileMode tm,
                                                  const SkSVGObjectBoundingBoxUnits units,
                                                  const SkMatrix& localMatrix) const {
    const SkSVGLengthContext lctx =
            units.type() == SkSVGObjectBoundingBoxUnits::Type::kObjectBoundingBox
            ? SkSVGLengthContext({1, 1})
            : ctx.lengthContext();

    auto fx1 = GETVAL(this->fX1, SkSVGLength(0, SkSVGLength::Unit::kPercentage));
    auto fy1 = GETVAL(this->fY1, SkSVGLength(0, SkSVGLength::Unit::kPercentage));
    auto fx2 = GETVAL(this->fX2, SkSVGLength(100, SkSVGLength::Unit::kPercentage));
    auto fy2 = GETVAL(this->fY2, SkSVGLength(0, SkSVGLength::Unit::kPercentage));

    const auto x1 = lctx.resolve(fx1, SkSVGLengthContext::LengthType::kHorizontal);
    const auto y1 = lctx.resolve(fy1, SkSVGLengthContext::LengthType::kVertical);
    const auto x2 = lctx.resolve(fx2, SkSVGLengthContext::LengthType::kHorizontal);
    const auto y2 = lctx.resolve(fy2, SkSVGLengthContext::LengthType::kVertical);

    const SkPoint pts[2] = { {x1, y1}, {x2, y2}};

    return SkGradientShader::MakeLinear(pts, colors, nullptr, pos, count, tm, 0, &localMatrix);
}
