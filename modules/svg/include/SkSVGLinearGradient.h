/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGLinearGradient_DEFINED
#define SkSVGLinearGradient_DEFINED

#include "modules/svg/include/SkSVGGradient.h"
#include "modules/svg/include/SkSVGTypes.h"

class SK_API SkSVGLinearGradient final : public SkSVGGradient {
public:
    static sk_sp<SkSVGLinearGradient> Make() {
        return sk_sp<SkSVGLinearGradient>(new SkSVGLinearGradient());
    }


    SVG_OPTIONAL_ATTR(X1, SkSVGLength)
    SVG_OPTIONAL_ATTR(Y1, SkSVGLength)
    SVG_OPTIONAL_ATTR(X2, SkSVGLength)
    SVG_OPTIONAL_ATTR(Y2, SkSVGLength)

protected:
    bool parseAndSetAttribute(const char*, const char*) override;

    bool applyAttributes(sk_sp<SkSVGGradient>*) const override;

    sk_sp<SkShader> onMakeShader(const SkSVGRenderContext&,
                                 const SkColor4f*, const SkScalar*, int count,
                                 SkTileMode, const SkSVGObjectBoundingBoxUnits,
                                 const SkMatrix&) const override;

private:
    SkSVGLinearGradient();

    using INHERITED = SkSVGGradient;
};

#endif // SkSVGLinearGradient_DEFINED
