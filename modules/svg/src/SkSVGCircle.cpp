/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkCanvas.h"
#include "modules/svg/include/SkSVGCircle.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGValue.h"

SkSVGCircle::SkSVGCircle() : INHERITED(SkSVGTag::kCircle) {}

bool SkSVGCircle::parseAndSetAttribute(const char* n, const char* v) {
    return INHERITED::parseAndSetAttribute(n, v) ||
           this->setCx(SkSVGAttributeParser::parse<SkSVGLength>("cx", n, v)) ||
           this->setCy(SkSVGAttributeParser::parse<SkSVGLength>("cy", n, v)) ||
           this->setR(SkSVGAttributeParser::parse<SkSVGLength>("r", n, v));
}

std::tuple<SkPoint, SkScalar> SkSVGCircle::resolve(const SkSVGLengthContext& lctx) const {
    const auto cx = lctx.resolve(fCx, SkSVGLengthContext::LengthType::kHorizontal);
    const auto cy = lctx.resolve(fCy, SkSVGLengthContext::LengthType::kVertical);
    const auto  r = lctx.resolve(fR , SkSVGLengthContext::LengthType::kOther);

    return std::make_tuple(SkPoint::Make(cx, cy), r);
}

const SkPath* SkSVGCircle::onResolvePath(const SkSVGRenderContext& ctx) const {
    if (fPath.isEmpty()) {
        std::tie(fPos, fRad) = this->resolve(ctx.lengthContext());
        fPath = SkPath::Circle(fPos.x(), fPos.y(), fRad);
    }

    return &fPath;
}

void SkSVGCircle::onDraw(SkCanvas* canvas, const SkSVGLengthContext& lctx,
                         const SkPaint& paint, SkPathFillType) const {
    if (fRad > 0) {
        canvas->drawCircle(fPos.x(), fPos.y(), fRad, paint);
    }
}

SkPath SkSVGCircle::onAsPath(const SkSVGRenderContext& ctx) const {
    this->onResolvePath(ctx);
    this->mapToParent(&fPath);
    return fPath;
}

SkRect SkSVGCircle::onObjectBoundingBox(const SkSVGRenderContext& ctx) const {
    this->onResolvePath(ctx);
    return SkRect::MakeXYWH(fPos.fX - fRad, fPos.fY - fRad, 2 * fRad, 2 * fRad);
}

}
