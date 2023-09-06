/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGSymbol_DEFINED
#define SkSVGSymbol_DEFINED

#include "modules/svg/include/SkSVGContainer.h"
#include "include/pathops/SkPathOps.h"

class SK_API SkSVGSymbol : public SkSVGContainer {
public:
    static sk_sp<SkSVGSymbol> Make() { return sk_sp<SkSVGSymbol>(new SkSVGSymbol()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGSymbol>(new SkSVGSymbol(*this));
    }

    SVG_ATTR(X                  , SkSVGLength, SkSVGLength(0))
    SVG_ATTR(Y                  , SkSVGLength, SkSVGLength(0))
    SVG_ATTR(Width              , SkSVGLength, SkSVGLength(100, SkSVGLength::Unit::kPercentage))
    SVG_ATTR(Height             , SkSVGLength, SkSVGLength(100, SkSVGLength::Unit::kPercentage))
    SVG_ATTR(PreserveAspectRatio, SkSVGPreserveAspectRatio, SkSVGPreserveAspectRatio())
    
    SVG_OPTIONAL_ATTR(RefX      , SkSVGLength)
    SVG_OPTIONAL_ATTR(RefY      , SkSVGLength)
    SVG_OPTIONAL_ATTR(ViewBox   , SkSVGViewBoxType)

    bool parseAndSetAttribute(const char* n, const char* v) override;
    void onSetAttribute(SkSVGAttribute attr, const SkSVGValue& v) override;

    void renderSymbol(const SkSVGRenderContext& ctx) const;

private:
    SkSVGSymbol() : INHERITED(SkSVGTag::kSymbol) {}
    SkSVGSymbol(const SkSVGSymbol& other) : INHERITED(other)
        , fX(other.fX) , fY(other.fY), fWidth(other.fWidth), fHeight(other.fHeight)
        , fPreserveAspectRatio(other.fPreserveAspectRatio)
        , fRefX(other.fRefX) , fRefY(other.fRefY), fViewBox(other.fViewBox) {}

    // Only render symbols when referenced
    void onRender(const SkSVGRenderContext& ctx) const override { }

    using INHERITED = SkSVGContainer;
};

#endif // SkSVGSymbol_DEFINED
