/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGFeBlend_DEFINED
#define SkSVGFeBlend_DEFINED

#include "modules/svg/include/SkSVGFe.h"
#include "modules/svg/include/SkSVGTypes.h"

class SK_API SkSVGFeBlend : public SkSVGFe {
public:
    static sk_sp<SkSVGFeBlend> Make() { return sk_sp<SkSVGFeBlend>(new SkSVGFeBlend()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGFeBlend>(new SkSVGFeBlend(*this));
    }

    SVG_ATTR(Mode, SkSVGBlendMode, SkSVGBlendMode())
    SVG_ATTR(In2 , SkSVGFeInputType, SkSVGFeInputType())

protected:
    sk_sp<SkImageFilter> onMakeImageFilter(const SkSVGRenderContext&,
                                           const SkSVGFilterContext&) const override;

    std::vector<SkSVGFeInputType> getInputs() const override {
        return {this->getIn(), this->getIn2()};
    }

    bool parseAndSetAttribute(const char*, const char*) override;

private:
    SkSVGFeBlend() : INHERITED(SkSVGTag::kFeBlend) {}
    SkSVGFeBlend(const SkSVGFeBlend& other) : INHERITED(other)
        , fMode(other.fMode), fIn2(other.fIn2) {}

    using INHERITED = SkSVGFe;
};

#endif  // SkSVGFeBlend_DEFINED
