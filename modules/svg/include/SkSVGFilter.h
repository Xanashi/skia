/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGFilter_DEFINED
#define SkSVGFilter_DEFINED

#include "modules/svg/include/SkSVGHiddenContainer.h"
#include "modules/svg/include/SkSVGTypes.h"

class SK_API SkSVGFilter final : public SkSVGHiddenContainer {
public:
    static sk_sp<SkSVGFilter> Make() { return sk_sp<SkSVGFilter>(new SkSVGFilter()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGFilter>(new SkSVGFilter(*this));
    }

    sk_sp<SkImageFilter> buildFilterDAG(const SkSVGRenderContext&) const;

    SVG_ATTR(X, SkSVGLength, SkSVGLength(-10, SkSVGLength::Unit::kPercentage))
    SVG_ATTR(Y, SkSVGLength, SkSVGLength(-10, SkSVGLength::Unit::kPercentage))
    SVG_ATTR(Width, SkSVGLength, SkSVGLength(120, SkSVGLength::Unit::kPercentage))
    SVG_ATTR(Height, SkSVGLength, SkSVGLength(120, SkSVGLength::Unit::kPercentage))
    SVG_ATTR(FilterUnits,
             SkSVGObjectBoundingBoxUnits,
             SkSVGObjectBoundingBoxUnits(SkSVGObjectBoundingBoxUnits::Type::kObjectBoundingBox))
    SVG_ATTR(PrimitiveUnits,
             SkSVGObjectBoundingBoxUnits,
             SkSVGObjectBoundingBoxUnits(SkSVGObjectBoundingBoxUnits::Type::kUserSpaceOnUse))

private:
    SkSVGFilter() : INHERITED(SkSVGTag::kFilter) {}
    SkSVGFilter(const SkSVGFilter& other) : INHERITED(other) 
        , fX(other.fX), fY(other.fY), fWidth(other.fWidth), fHeight(other.fHeight)
        , fFilterUnits(other.fFilterUnits), fPrimitiveUnits(other.fPrimitiveUnits) {}

    bool parseAndSetAttribute(const char*, const char*) override;

    /** Propagates any inherited presentation attributes in the given context. */
    void applyProperties(SkSVGRenderContext*) const;

    using INHERITED = SkSVGHiddenContainer;
};

#endif  // SkSVGFilter_DEFINED
