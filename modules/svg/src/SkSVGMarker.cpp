/*
 * Copyright 2021 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/svg/include/SkSVGMarker.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "include/core/SkCanvas.h"

bool SkSVGMarker::parseAndSetAttribute(const char* n, const char* v) {
    return INHERITED::parseAndSetAttribute(n, v) ||
           this->setRefX(SkSVGAttributeParser::parse<SkSVGLength>("refX", n, v)) ||
           this->setRefY(SkSVGAttributeParser::parse<SkSVGLength>("refY", n, v)) ||
           this->setMarkerWidth(SkSVGAttributeParser::parse<SkSVGLength>("markerWidth", n, v)) ||
           this->setMarkerHeight(SkSVGAttributeParser::parse<SkSVGLength>("markerHeight", n, v)) ||
           this->setMarkerUnits(SkSVGAttributeParser::parse<SkSVGMarkerUnits>("markerUnits", n, v)) ||
           this->setOrient(SkSVGAttributeParser::parse<SkSVGMarkerOrient>("orient", n, v));
}

void SkSVGMarker::onSetAttribute(SkSVGAttribute attr, const SkSVGValue& v) {
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

void SkSVGMarker::renderMarker(const SkSVGRenderContext& ctx, SkPoint refPoint, SkPoint markerPoint1, 
                  SkPoint markerPoint2, bool isStartMarker) const{
    float angle1 = 0.0;
    if (fOrient.type() == SkSVGMarkerOrient::Type::kAuto || 
        fOrient.type() == SkSVGMarkerOrient::Type::kAutoStartReverse) {
        float xDiff = markerPoint2.x() - markerPoint1.x();
        float yDiff = markerPoint2.y() - markerPoint1.y();
        angle1 = SkRadiansToDegrees(SkScalarATan2(yDiff, xDiff));

        if (isStartMarker && fOrient.type() == SkSVGMarkerOrient::Type::kAutoStartReverse) {
            angle1 += 180;
        }
    }

    this->internalRenderMarker(angle1, ctx, refPoint);
}

void SkSVGMarker::renderMarker(const SkSVGRenderContext& ctx, SkPoint refPoint, SkPoint markerPoint1,
                  SkPoint markerPoint2, SkPoint markerPoint3) const {
    float xDiff = markerPoint2.x() - markerPoint1.x();
    float yDiff = markerPoint2.y() - markerPoint1.y();
    float angle1 = SkRadiansToDegrees(SkScalarATan2(yDiff, xDiff));

    xDiff = markerPoint3.x() - markerPoint2.x();
    yDiff = markerPoint3.y() - markerPoint2.y();
    float angle2 = SkRadiansToDegrees(SkScalarATan2(yDiff, xDiff));

    this->internalRenderMarker((angle1 + angle2) / 2, ctx, refPoint);
}

void SkSVGMarker::internalRenderMarker(float angle, const SkSVGRenderContext& ctx, SkPoint markerPoint) const {
    SkSVGLength x(markerPoint.x());
    SkSVGLength y(markerPoint.y());

    auto viewPortRect = ctx.lengthContext().resolveRect(x, y, fMarkerWidth, fMarkerHeight);
    auto markerTransform = SkMatrix::Translate(viewPortRect.x(), viewPortRect.y());

    if (fOrient.type() == SkSVGMarkerOrient::Type::kAuto ||
        fOrient.type() == SkSVGMarkerOrient::Type::kAutoStartReverse)
        markerTransform.preRotate(angle);
    else
        markerTransform.preRotate(fOrient.angle());

    //markerTransform.preConcat(ComputeViewboxMatrix(fViewBox, viewPortRect, fPreserveAspectRatio));

    switch (fMarkerUnits.type()) {
        case SkSVGMarkerUnits::Type::kStrokeWidth: {
            auto strokeWidth = ctx.presentationContext().fInherited.fStrokeWidth->value();
            markerTransform.preScale(strokeWidth, strokeWidth);

            auto viewBoxWidth = fViewBox.width();
            auto viewBoxHeight = fViewBox.height();

            auto scaleFactorWidth = viewBoxWidth <= 0 ? 1 : fMarkerWidth.value() / viewBoxWidth;
            auto scaleFactorHeight = viewBoxHeight <= 0 ? 1 : fMarkerHeight.value() / viewBoxHeight;

            auto viewBoxToMarkerUnitsScaleX = fmin(scaleFactorWidth, scaleFactorHeight);
            auto viewBoxToMarkerUnitsScaleY = fmin(scaleFactorWidth, scaleFactorHeight);

            markerTransform.preTranslate(-fRefX.value() * viewBoxToMarkerUnitsScaleX,
                                         -fRefY.value() * viewBoxToMarkerUnitsScaleY);
            markerTransform.preScale(viewBoxToMarkerUnitsScaleX, viewBoxToMarkerUnitsScaleY);
            break;
        }
        case SkSVGMarkerUnits::Type::kUserSpaceOnUse:
            markerTransform.preTranslate(-fRefX.value(), -fRefY.value());
            break;
    }

    markerTransform.preConcat(this->getTransform());

    auto markerClone = sk_pointer_cast<SkSVGMarker>(this->makeShallowClone());
    markerClone->setTransform(markerTransform);

    if (!markerClone->getFill().isValue()) {
        markerClone->setAttribute("fill", "black");
    }
    if (!markerClone->getStroke().isValue()) {
        markerClone->setAttribute("stroke", "none");
    }
            
    markerClone->renderMarkerContent(ctx);
}

void SkSVGMarker::renderMarkerContent(const SkSVGRenderContext& ctx) const {
    // Clear the presentation context so we only inherit attributes from the marker
    SkSVGPresentationContext pctx;
    SkSVGRenderContext localContext(ctx, pctx, this);
    localContext.setContextColors(*ctx.presentationContext().fInherited.fFill,
                                  *ctx.presentationContext().fInherited.fStroke);

    if (SkSVGContainer::onPrepareToRender(&localContext)) {
        SkSVGContainer::onRender(localContext);
    }
}
