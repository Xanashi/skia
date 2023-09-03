/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGShape.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathMeasure.h"

SkSVGShape::SkSVGShape(SkSVGTag t) : INHERITED(t) {}


bool SkSVGShape::parseAndSetAttribute(const char* n, const char* v) {
    return INHERITED::parseAndSetAttribute(n, v) ||
           this->setPathLength(SkSVGAttributeParser::parse<SkSVGLength>("pathLength", n, v));
}

void SkSVGShape::appendChild(sk_sp<SkSVGNode> node) {
    auto tag = node->tag();
    if (tag == SkSVGTag::kAnimate || 
        tag == SkSVGTag::kAnimateTransform ||
        tag == SkSVGTag::kAnimateMotion) {
        onApplyAnimationNode(static_cast<const SkSVGAnimate*>(node.get()));
    } else {
        SkDebugf("cannot append child nodes to an SVG shape.\n");
    }
}

void SkSVGShape::onRender(const SkSVGRenderContext& ctx) const {
    // Perform geometry resolution in onResolvePath and cache it for the 2 below onDraw calls
    const SkPath* path = this->onResolvePath(ctx);

    if (path) {
        ctx.setPathLengthRatio(this->getPathLengthRatio(path));        
    }

    const auto fillType  = ctx.presentationContext().fInherited.fFillRule->asFillType();
    const auto fillPaint = ctx.fillPaint(),
             strokePaint = ctx.strokePaint();

    if (fillPaint.isValid()) {
        this->onDraw(ctx.canvas(), ctx.lengthContext(), *fillPaint, fillType);
    }

    if (strokePaint.isValid()) {
        this->onDraw(ctx.canvas(), ctx.lengthContext(), *strokePaint, fillType);
    }
}

SkScalar SkSVGShape::getPathLengthRatio(const SkPath* path) const {
    if (fPathLength.value() > 0) {
        SkScalar computedPathLength = SkPathMeasure(*path, false).getLength();
        if (computedPathLength > 0) {            
            return computedPathLength / fPathLength.value();
        }
    }
    return 1;
}

}
