/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkCanvas.h"
#include "modules/svg/include/SkSVGLine.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGValue.h"

SkSVGLine::SkSVGLine() : INHERITED(SkSVGTag::kLine) {}

bool SkSVGLine::parseAndSetAttribute(const char* n, const char* v) {
    return INHERITED::parseAndSetAttribute(n, v) ||
           this->setX1(SkSVGAttributeParser::parse<SkSVGLength>("x1", n, v)) ||
           this->setY1(SkSVGAttributeParser::parse<SkSVGLength>("y1", n, v)) ||
           this->setX2(SkSVGAttributeParser::parse<SkSVGLength>("x2", n, v)) ||
           this->setY2(SkSVGAttributeParser::parse<SkSVGLength>("y2", n, v));
}

std::tuple<SkPoint, SkPoint> SkSVGLine::resolve(const SkSVGLengthContext& lctx) const {
    return std::make_tuple(
        SkPoint::Make(lctx.resolve(fX1, SkSVGLengthContext::LengthType::kHorizontal),
                      lctx.resolve(fY1, SkSVGLengthContext::LengthType::kVertical)),
        SkPoint::Make(lctx.resolve(fX2, SkSVGLengthContext::LengthType::kHorizontal),
                      lctx.resolve(fY2, SkSVGLengthContext::LengthType::kVertical)));
}

const SkPath* SkSVGLine::onResolvePath(const SkSVGRenderContext& ctx) const {
    if (fPath.isEmpty()) {
        std::tie(fP0, fP1) = this->resolve(ctx.lengthContext());
        fPath = SkPath::Line(fP0, fP1);
    }
    return &fPath;
}

void SkSVGLine::onDraw(SkCanvas* canvas, const SkSVGLengthContext& lctx,
                       const SkPaint& paint, SkPathFillType) const {
    canvas->drawLine(fP0, fP1, paint);
}

SkPath SkSVGLine::onAsPath(const SkSVGRenderContext& ctx) const {
    this->onResolvePath(ctx);
    this->mapToParent(&fPath);
    return fPath;
}
