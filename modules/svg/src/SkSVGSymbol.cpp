/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/svg/include/SkSVGSymbol.h"

#include "include/core/SkCanvas.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGValue.h"

bool SkSVGSymbol::parseAndSetAttribute(const char* n, const char* v) {
    return INHERITED::parseAndSetAttribute(n, v) ||
        this->setX(SkSVGAttributeParser::parse<SkSVGLength>("x", n, v)) ||
        this->setY(SkSVGAttributeParser::parse<SkSVGLength>("y", n, v)) ||
        this->setRefX(SkSVGAttributeParser::parse<SkSVGLength>("refX", n, v)) ||
        this->setRefY(SkSVGAttributeParser::parse<SkSVGLength>("refY", n, v)) ||
        this->setWidth(SkSVGAttributeParser::parse<SkSVGLength>("width", n, v)) ||
        this->setHeight(SkSVGAttributeParser::parse<SkSVGLength>("height", n, v));
}

void SkSVGSymbol::onSetAttribute(SkSVGAttribute attr, const SkSVGValue& v) {
    switch (attr) {
        case SkSVGAttribute::kViewBox:
            if (const auto* vb = v.as<SkSVGViewBoxValue>()) {
                this->setViewBox(*vb);
            }
            break;
        case SkSVGAttribute::kPreserveAspectRatio:
            if (const auto* par = v.as<SkSVGPreserveAspectRatioValue>()) {
                this->setPreserveAspectRatio(*par);
            }
            break;
        default:
            this->INHERITED::onSetAttribute(attr, v);
    }
}

void SkSVGSymbol::renderSymbol(const SkSVGRenderContext& ctx) const {
    SkSVGRenderContext lctx(ctx, this);

    auto viewPortRect = ctx.lengthContext().resolveRect(fX, fY, fWidth, fHeight);
    auto contentMatrix = SkMatrix::Translate(viewPortRect.x(), viewPortRect.y());
    auto viewPort = SkSize::Make(viewPortRect.width(), viewPortRect.height());

    if (fViewBox.isValid()) {
        const SkRect& viewBox = *fViewBox;
        viewPort = SkSize::Make(viewBox.width(), viewBox.height());
        contentMatrix.preConcat(ComputeViewboxMatrix(viewBox, viewPortRect, fPreserveAspectRatio));
    } 

    if (!contentMatrix.isIdentity()) {
        lctx.saveOnce();
        lctx.canvas()->concat(contentMatrix);
    }

    if (viewPort != lctx.lengthContext().viewPort()) {
        lctx.writableLengthContext()->setViewPort(viewPort);
    }

    if (INHERITED::onPrepareToRender(&lctx)) {
        INHERITED::onRender(lctx);
    }
}