/*
 * Copyright 2017 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGPattern_DEFINED
#define SkSVGPattern_DEFINED

#include "include/core/SkRefCnt.h"
#include "include/private/SkAPI.h"
#include "modules/svg/include/SkSVGHiddenContainer.h"
#include "modules/svg/include/SkSVGNode.h"
#include "modules/svg/include/SkSVGTypes.h"

#include <optional>

class SkPaint;
class SkSVGRenderContext;

class SK_API SkSVGPattern final : public SkSVGHiddenContainer {
public:
    static sk_sp<SkSVGPattern> Make() {
        return sk_sp<SkSVGPattern>(new SkSVGPattern());
    }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGPattern>(new SkSVGPattern(*this));
    }

    SVG_ATTR(Href, SkSVGIRI, SkSVGIRI())
    SVG_OPTIONAL_ATTR(X                  , SkSVGLength)
    SVG_OPTIONAL_ATTR(Y                  , SkSVGLength)
    SVG_OPTIONAL_ATTR(Width              , SkSVGLength)
    SVG_OPTIONAL_ATTR(Height             , SkSVGLength)
    SVG_OPTIONAL_ATTR(PatternTransform   , SkSVGTransformType)
    SVG_OPTIONAL_ATTR(PatternContentUnits, SkSVGObjectBoundingBoxUnits)
    SVG_OPTIONAL_ATTR(PatternUnits       , SkSVGObjectBoundingBoxUnits)

    SVG_OPTIONAL_ATTR(PreserveAspectRatio, SkSVGPreserveAspectRatio)
    SVG_OPTIONAL_ATTR(ViewBox            , SkSVGViewBoxType)

protected:
    SkSVGPattern();
    SkSVGPattern(const SkSVGPattern& other) : INHERITED(other) 
        , fHref(other.fHref), fX(other.fX), fY(other.fY), fWidth(other.fWidth)
        , fHeight(other.fHeight), fPatternTransform(other.fPatternTransform) 
        , fPatternContentUnits(other.fPatternContentUnits), fPatternUnits(other.fPatternUnits)
        , fPreserveAspectRatio(other.fPreserveAspectRatio), fViewBox(other.fViewBox) {}

    bool parseAndSetAttribute(const char*, const char*) override;
    void onSetAttribute(SkSVGAttribute, const SkSVGValue&) override;

    bool onAsPaint(const SkSVGRenderContext&, SkPaint*) const override;

private:
    struct PatternAttributes {
        std::optional<SkSVGLength>  fX,
                                    fY,
                                    fWidth,
                                    fHeight;
        std::optional<SkSVGTransformType>          fPatternTransform;
        std::optional<SkSVGObjectBoundingBoxUnits> fPatternContentUnits;
        std::optional<SkSVGObjectBoundingBoxUnits> fPatternUnits;
        std::optional<SkSVGPreserveAspectRatio>    fPreserveAspectRatio;
        std::optional<SkSVGViewBoxType>            fViewBox;
    };

    const SkSVGPattern* resolveHref(const SkSVGRenderContext&, PatternAttributes*) const;
    const SkSVGPattern* hrefTarget(const SkSVGRenderContext&) const;

    using INHERITED = SkSVGHiddenContainer;
};

#endif // SkSVGPattern_DEFINED
