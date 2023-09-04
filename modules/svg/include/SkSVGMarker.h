/*
 * Copyright 2021 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGMarker_DEFINED
#define SkSVGMarker_DEFINED

#include "modules/svg/include/SkSVGHiddenContainer.h"
#include "modules/svg/include/SkSVGTypes.h"
#include "modules/svg/include/SkSVGValue.h"
#include "include/core/SkPoint.h"

class SK_API SkSVGMarker final : public SkSVGHiddenContainer {
public:
    static sk_sp<SkSVGMarker> Make() { return sk_sp<SkSVGMarker>(new SkSVGMarker()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGMarker>(new SkSVGMarker(*this));
    }

    SVG_ATTR(RefX               , SkSVGLength, SkSVGLength(0))
    SVG_ATTR(RefY               , SkSVGLength, SkSVGLength(0))
    SVG_ATTR(MarkerWidth        , SkSVGLength, SkSVGLength(3, SkSVGLength::Unit::kPX))
    SVG_ATTR(MarkerHeight       , SkSVGLength, SkSVGLength(3, SkSVGLength::Unit::kPX))
    SVG_ATTR(MarkerUnits        , SkSVGMarkerUnits , SkSVGMarkerUnits())
    SVG_ATTR(Orient             , SkSVGMarkerOrient, SkSVGMarkerOrient())
    SVG_ATTR(PreserveAspectRatio, SkSVGPreserveAspectRatio, SkSVGPreserveAspectRatio())
    SVG_ATTR(ViewBox            , SkSVGViewBoxType, SkSVGViewBoxType())

    bool parseAndSetAttribute(const char*, const char*) override;

    void renderMarker(const SkSVGRenderContext&, SkPoint, SkPoint, SkPoint, bool) const;
    void renderMarker(const SkSVGRenderContext&, SkPoint, SkPoint, SkPoint, SkPoint) const;

protected:
    void onSetAttribute(SkSVGAttribute, const SkSVGValue&) override;

private:
    friend class SkSVGRenderContext;

    SkSVGMarker() : INHERITED(SkSVGTag::kMarker) {}
    SkSVGMarker(const SkSVGMarker& other) : INHERITED(other)
        , fRefX(other.fRefX) , fRefY(other.fRefY) , fMarkerWidth(other.fMarkerWidth)
        , fMarkerHeight(other.fMarkerHeight) , fMarkerUnits(other.fMarkerUnits) , fOrient(other.fOrient)
        , fPreserveAspectRatio(other.fPreserveAspectRatio), fViewBox(other.fViewBox) {}

    void internalRenderMarker(float, const SkSVGRenderContext&, SkPoint) const;
    void renderMarkerContent(const SkSVGRenderContext&) const;

    using INHERITED = SkSVGHiddenContainer;
};

#endif // SkSVGMarker_DEFINED
