/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGFeDropShadow_DEFINED
#define SkSVGFeDropShadow_DEFINED

#include "modules/svg/include/SkSVGFe.h"
#include "modules/svg/include/SkSVGTypes.h"

class SK_API SkSVGFeDropShadow : public SkSVGFe {
public:
    static sk_sp<SkSVGFeDropShadow> Make() {
        return sk_sp<SkSVGFeDropShadow>(new SkSVGFeDropShadow());
    }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGFeDropShadow>(new SkSVGFeDropShadow(*this));
    }

    SVG_ATTR(Dx, SkSVGNumberType, SkSVGNumberType(0))
    SVG_ATTR(Dy, SkSVGNumberType, SkSVGNumberType(0))
    SVG_ATTR(StdDeviation, StdDeviation, StdDeviation({0, 0}))

protected:
    sk_sp<SkImageFilter> onMakeImageFilter(const SkSVGRenderContext&,
                                           const SkSVGFilterContext&) const override;

    std::vector<SkSVGFeInputType> getInputs() const override { return {this->getIn()}; }

    bool parseAndSetAttribute(const char*, const char*) override;

private:
    SkSVGFeDropShadow() : INHERITED(SkSVGTag::kFeDropShadow) {}
    SkSVGFeDropShadow(const SkSVGFeDropShadow& other) : INHERITED(other)
        , fDx(other.fDx), fDy(other.fDy), fStdDeviation(other.fStdDeviation) {}

    SkColor resolveFloodColor(const SkSVGRenderContext&) const;

    using INHERITED = SkSVGFe;
};

#endif  // SkSVGFeDropShadow_DEFINED
