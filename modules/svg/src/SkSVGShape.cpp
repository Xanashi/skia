/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGShape.h"
#include "modules/svg/include/SkSVGMarker.h"
#include "include/core/SkPath.h"
#include "include/core/SkPathMeasure.h"
#include "include/core/SkCanvas.h"

SkSVGShape::SkSVGShape(SkSVGTag t) : INHERITED(t) {}

inline const SkSVGMarker* AsMarker(SkSVGRenderContext::BorrowedNode node) {
    return node.get() ? reinterpret_cast<const SkSVGMarker*>(node.get()) : nullptr;
}

bool SkSVGShape::parseAndSetAttribute(const char* n, const char* v) {
    return INHERITED::parseAndSetAttribute(n, v) ||
           this->resolveShorthandMarker(n, v) ||
           this->setMarkerStart(SkSVGAttributeParser::parse<SkSVGFuncIRI>("marker-start", n, v)) ||
           this->setMarkerMid(SkSVGAttributeParser::parse<SkSVGFuncIRI>("marker-mid", n, v)) ||
           this->setMarkerEnd(SkSVGAttributeParser::parse<SkSVGFuncIRI>("marker-end", n, v)) ||
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

    if (fillPaint.isValid() && this->tag() != SkSVGTag::kLine) {
        this->onDraw(ctx.canvas(), ctx.lengthContext(), *fillPaint, fillType);
    }

    if (strokePaint.isValid()) {
        this->onDraw(ctx.canvas(), ctx.lengthContext(), *strokePaint, fillType);
    }

    if (path && this->onSupportsMarkers()) {
        this->renderMarkers(ctx, path);
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

void SkSVGShape::renderMarkers(const SkSVGRenderContext& ctx, const SkPath* markerPath) const {    
    if (markerPath->countPoints() <= 0) {
        return;
    }

    auto pathPoints = getPathPoints(markerPath);
    auto pointCount = (int)pathPoints.size();

    if (pointCount < 2) {
        return;
    }

    if (fMarkerStart.type() == SkSVGFuncIRI::Type::kIRI) {
        auto refPoint1 = pathPoints[0].pt;
        auto index = 1;
        while (index < pointCount && pathPoints[index].pt == refPoint1) {
            ++index;
        }
        if (index < pointCount) {
            auto refPoint2 = pathPoints[index].pt;
            const auto marker = AsMarker(ctx.findNodeById(fMarkerStart.iri()));
            if (marker) {
                marker->renderMarker(ctx, refPoint1, refPoint1, refPoint2, true);
            }
        }
    }

    if (fMarkerMid.type() == SkSVGFuncIRI::Type::kIRI) {
        const auto marker = AsMarker(ctx.findNodeById(fMarkerMid.iri()));
        if (marker) {
            auto bezierIndex = -1;
            for (auto i = 1; i <= pointCount - 2; i++) {
                // For Bezier curves, the marker shall only been shown at the last point
                if (pathPoints[i].isBezier)
                    bezierIndex = (bezierIndex + 1) % pathPoints[i].pointsInCurve;
                else
                    bezierIndex = -1;

                if (bezierIndex == -1 || bezierIndex == pathPoints[i].pointsInCurve - 1) {
                    marker->renderMarker(ctx, pathPoints[i].pt, pathPoints[i - 1].pt,
                                              pathPoints[i].pt, pathPoints[i + 1].pt);
                }
            }

            // If the last path point is the same as the first, we have a closed subpath
            // and in that case marker-mid also affects the first/last vertex of the path
            if (pathPoints[0].pt == pathPoints[pointCount - 1].pt) {
                marker->renderMarker(ctx, pathPoints[pointCount - 1].pt, pathPoints[pointCount - 2].pt,
                                          pathPoints[pointCount - 1].pt, pathPoints[0].pt);
            }
        }
    }

    if (fMarkerEnd.type() == SkSVGFuncIRI::Type::kIRI) {
        auto index = pointCount - 1;
        auto refPoint1 = pathPoints[index].pt;
        --index;
        while (index > 0 && pathPoints[index].pt == refPoint1) {
            --index;
        }
        auto refPoint2 = pathPoints[index].pt;
        const auto marker = AsMarker(ctx.findNodeById(fMarkerEnd.iri()));
        if (marker) {
            marker->renderMarker(ctx, refPoint1, refPoint2, pathPoints[pointCount - 1].pt, false);
        }
    }
}

std::vector<PathPoint> SkSVGShape::getPathPoints(const SkPath* markerPath) const {
    SkPoint points[4];
    SkPath::Verb verb;
    SkPath::Iter iter(*markerPath, false);
    std::vector<PathPoint> markerPoints;

    while ((verb = iter.next(points)) != SkPath::kDone_Verb) {
        switch (verb) {
            case SkPath::kMove_Verb: {
                markerPoints.push_back({points[0], false});
                break;
            }
            case SkPath::kLine_Verb: {
                markerPoints.push_back({points[1], false});
                break;
            }
            case SkPath::kCubic_Verb: {
                markerPoints.push_back({points[1], true, 3});
                markerPoints.push_back({points[2], true, 3});
                markerPoints.push_back({points[3], true, 3});
                break;
            }
            case SkPath::kQuad_Verb: {
                markerPoints.push_back({points[1], true, 2});
                markerPoints.push_back({points[2], true, 2});
                break;
            }
            case SkPath::kConic_Verb: {
                SkPoint quads[5];
                SkPath::ConvertConicToQuads(points[0], points[1], points[2], iter.conicWeight(), quads, 1);

                markerPoints.push_back({quads[1], true, 4});
                markerPoints.push_back({quads[2], true, 4});
                markerPoints.push_back({quads[3], true, 4});
                markerPoints.push_back({quads[4], true, 4});
                break;
            }
            case SkPath::kClose_Verb:
            case SkPath::kDone_Verb: {
                break;
            }
        }
    }

    return markerPoints;
}

bool SkSVGShape::resolveShorthandMarker(const char* n, const char* v) {
    auto iri = SkSVGAttributeParser::parse<SkSVGFuncIRI>("marker", n, v);
    if (iri.isValid()) {
        if (fMarkerStart.type() != SkSVGFuncIRI::Type::kIRI) {
            this->setMarkerStart(iri);
        }
        if (fMarkerMid.type() != SkSVGFuncIRI::Type::kIRI) {
            this->setMarkerMid(iri);
        }
        if (fMarkerEnd.type() != SkSVGFuncIRI::Type::kIRI) {
            this->setMarkerEnd(iri);
        }
        return true;
    }
    return false;
}
