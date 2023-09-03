/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGFeOffset_DEFINED
#define SkSVGFeOffset_DEFINED

#include "modules/svg/include/SkSVGFe.h"
#include "modules/svg/include/SkSVGTypes.h"

class SK_API SkSVGFeOffset : public SkSVGFe {
public:
    static sk_sp<SkSVGFeOffset> Make() { return sk_sp<SkSVGFeOffset>(new SkSVGFeOffset()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGFeOffset>(new SkSVGFeOffset(*this));
    }

    SVG_ATTR(Dx, SkSVGNumberType, SkSVGNumberType(0))
    SVG_ATTR(Dy, SkSVGNumberType, SkSVGNumberType(0))

protected:
    sk_sp<SkImageFilter> onMakeImageFilter(const SkSVGRenderContext&,
                                           const SkSVGFilterContext&) const override;

    std::vector<SkSVGFeInputType> getInputs() const override { return {this->getIn()}; }

    SkSVGColorspace resolveColorspace(const SkSVGRenderContext&,
                                      const SkSVGFilterContext&) const override {
        return SkSVGColorspace::kSRGB;
    }

    bool parseAndSetAttribute(const char*, const char*) override;

private:
    SkSVGFeOffset() : INHERITED(SkSVGTag::kFeOffset) {}
    SkSVGFeOffset(const SkSVGFeOffset& other) : INHERITED(other)
        , fDx(other.fDx), fDy(other.fDy) {}

    using INHERITED = SkSVGFe;
};

#endif  // SkSVGFeOffset_DEFINED
