/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGShape_DEFINED
#define SkSVGShape_DEFINED

#include "include/core/SkPath.h"
#include "modules/svg/include/SkSVGTransformableNode.h"
#include "modules/svg/include/SkSVGAnimate.h"

class SkSVGLengthContext;
class SkPaint;

class SK_API SkSVGShape : public SkSVGTransformableNode {
public:
    void appendChild(sk_sp<SkSVGNode>) override;

    SVG_ATTR(PathLength , SkSVGLength , SkSVGLength(0))

protected:
    SkSVGShape(SkSVGTag);
    SkSVGShape(const SkSVGShape& other) : INHERITED(other), fPathLength(other.fPathLength) {}

    bool parseAndSetAttribute(const char*, const char*) override;

    void onRender(const SkSVGRenderContext&) const final;

    virtual void          onApplyAnimationNode(const SkSVGAnimate*) {}

    virtual const SkPath* onResolvePath(const SkSVGRenderContext&) const = 0; 
    virtual void          onDraw(SkCanvas*, const SkSVGLengthContext&, const SkPaint&,
                                 SkPathFillType) const = 0;

private:
    using INHERITED = SkSVGTransformableNode;

    SkScalar getPathLengthRatio(const SkPath*) const;
};

#endif // SkSVGShape_DEFINED
