/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkColorSpace.h"
#include "include/effects/SkGradientShader.h"
#include "modules/svg/include/SkSVGRadialGradient.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGValue.h"
#include "modules/svg/include/SkSVGTools.h"

SkSVGRadialGradient::SkSVGRadialGradient() : INHERITED(SkSVGTag::kRadialGradient) {}

bool SkSVGRadialGradient::parseAndSetAttribute(const char* name, const char* value) {
    return INHERITED::parseAndSetAttribute(name, value) ||
           this->setCx(SkSVGAttributeParser::parse<SkSVGLength>("cx", name, value)) ||
           this->setCy(SkSVGAttributeParser::parse<SkSVGLength>("cy", name, value)) ||
           this->setR(SkSVGAttributeParser::parse<SkSVGLength>("r", name, value))   ||
           this->setFx(SkSVGAttributeParser::parse<SkSVGLength>("fx", name, value)) ||
           this->setFy(SkSVGAttributeParser::parse<SkSVGLength>("fy", name, value));
}

bool SkSVGRadialGradient::applyAttributes(sk_sp<SkSVGGradient>* gradient) const {
    bool result = INHERITED::applyAttributes(gradient);

    if ((*gradient)->tag() != SkSVGTag::kRadialGradient) {
        return true;
    }

    auto grad = sk_pointer_cast<SkSVGRadialGradient>(*gradient); 
    if (!grad.get()) { return true; }

    return result |
           inherit_if_needed(this->fCx, grad->fCx) |
           inherit_if_needed(this->fCy, grad->fCy) |
           inherit_if_needed(this->fR , grad->fR)  |
           inherit_if_needed(this->fFx, grad->fFx) |
           inherit_if_needed(this->fFy, grad->fFy);
}

sk_sp<SkShader> SkSVGRadialGradient::onMakeShader(const SkSVGRenderContext& ctx,
                                                  const SkColor4f* colors, const SkScalar* pos,
                                                  int count, SkTileMode tm,
                                                  const SkSVGObjectBoundingBoxUnits units,
                                                  const SkMatrix& m) const {
    const SkSVGLengthContext lctx =
            units.type() == SkSVGObjectBoundingBoxUnits::Type::kObjectBoundingBox
                    ? SkSVGLengthContext({1, 1})
                    : ctx.lengthContext();

    auto fcx = GETVAL(fCx, SkSVGLength(50, SkSVGLength::Unit::kPercentage));
    auto fcy = GETVAL(fCy, SkSVGLength(50, SkSVGLength::Unit::kPercentage));
    auto fr  = GETVAL(fR , SkSVGLength(50, SkSVGLength::Unit::kPercentage));

    const auto      r = lctx.resolve(fr , SkSVGLengthContext::LengthType::kOther);
    const auto center = SkPoint::Make(
                        lctx.resolve(fcx, SkSVGLengthContext::LengthType::kHorizontal),
                        lctx.resolve(fcy, SkSVGLengthContext::LengthType::kVertical));
    const auto  focal = SkPoint::Make(
        fFx.isValid() ? lctx.resolve(*fFx, SkSVGLengthContext::LengthType::kHorizontal)
                      : center.x(),
        fFy.isValid() ? lctx.resolve(*fFy, SkSVGLengthContext::LengthType::kVertical)
                      : center.y());

    if (r == 0) {
        const auto last_color = count > 0 ? colors[count - 1] : SkColors::kBlack;
        return SkShaders::Color(last_color, nullptr);
    }

    return center == focal
        ? SkGradientShader::MakeRadial(center, r, colors, nullptr, pos, count, tm, 0, &m)
        : SkGradientShader::MakeTwoPointConical(focal, 0, center, r, colors, nullptr, pos,
                                                count, tm, 0, &m);
}
