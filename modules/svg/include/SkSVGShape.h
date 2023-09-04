/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGShape_DEFINED
#define SkSVGShape_DEFINED


#include "modules/svg/include/SkSVGTransformableNode.h"
#include "modules/svg/include/SkSVGAnimate.h"

class SkSVGLengthContext;
class SkPaint;
class SkPath;

struct PathPoint {
    SkPoint pt;
    bool isBezier;
    int pointsInCurve = 1;
};

class SK_API SkSVGShape : public SkSVGTransformableNode {
public:
    void appendChild(sk_sp<SkSVGNode>) override;

    SVG_ATTR(MarkerStart, SkSVGFuncIRI, SkSVGFuncIRI())
    SVG_ATTR(MarkerMid  , SkSVGFuncIRI, SkSVGFuncIRI())
    SVG_ATTR(MarkerEnd  , SkSVGFuncIRI, SkSVGFuncIRI())
    SVG_ATTR(PathLength , SkSVGLength , SkSVGLength(0))

protected:
    SkSVGShape(SkSVGTag);
    SkSVGShape(const SkSVGShape& other) : INHERITED(other)
        , fMarkerStart(other.fMarkerStart), fMarkerMid(other.fMarkerMid)
        , fMarkerEnd(other.fMarkerEnd), fPathLength(other.fPathLength) {}

    bool parseAndSetAttribute(const char*, const char*) override;

    void onRender(const SkSVGRenderContext&) const final;

    virtual void          onApplyAnimationNode(const SkSVGAnimate*) {}
    virtual bool          onSupportsMarkers() const { return false; } // override on shapes that support markers

    virtual const SkPath* onResolvePath(const SkSVGRenderContext&) const = 0; 
    virtual void          onDraw(SkCanvas*, const SkSVGLengthContext&, const SkPaint&,
                                 SkPathFillType) const = 0;

private:
    using INHERITED = SkSVGTransformableNode;

    SkScalar getPathLengthRatio(const SkPath*) const;
    void renderMarkers(const SkSVGRenderContext&, const SkPath*) const;
    std::vector<PathPoint> getPathPoints(const SkPath* markerPath) const;
    bool resolveShorthandMarker(const char*, const char*);
};

#endif // SkSVGShape_DEFINED
