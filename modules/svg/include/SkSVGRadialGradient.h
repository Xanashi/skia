/*
 * Copyright 2017 Google LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGRadialGradient_DEFINED
#define SkSVGRadialGradient_DEFINED

#include "include/core/SkColor.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkScalar.h"
#include "include/private/SkAPI.h"
#include "modules/svg/include/SkSVGGradient.h"
#include "modules/svg/include/SkSVGNode.h"
#include "modules/svg/include/SkSVGTypes.h"

class SkMatrix;
class SkSVGRenderContext;
class SkShader;
enum class SkTileMode;

class SK_API SkSVGRadialGradient final : public SkSVGGradient {
public:
    static sk_sp<SkSVGRadialGradient> Make() {
        return sk_sp<SkSVGRadialGradient>(new SkSVGRadialGradient());
    }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGRadialGradient>(new SkSVGRadialGradient(*this));
    }

    SVG_OPTIONAL_ATTR(Cx, SkSVGLength)
    SVG_OPTIONAL_ATTR(Cy, SkSVGLength)
    SVG_OPTIONAL_ATTR(R,  SkSVGLength)
    SVG_OPTIONAL_ATTR(Fx, SkSVGLength)
    SVG_OPTIONAL_ATTR(Fy, SkSVGLength)

protected:
    bool parseAndSetAttribute(const char*, const char*) override;

    bool applyAttributes(sk_sp<SkSVGGradient>*) const override;

    sk_sp<SkShader> onMakeShader(const SkSVGRenderContext&,
                                 const SkColor4f*, const SkScalar*, int count,
                                 SkTileMode, const SkSVGObjectBoundingBoxUnits units,
                                 const SkMatrix&) const override;
private:
    SkSVGRadialGradient();
    SkSVGRadialGradient(const SkSVGRadialGradient& other) : INHERITED(other)
        , fCx(other.fCx), fCy(other.fCy), fR(other.fR), fFx(other.fFx), fFy(other.fFy) {}

    using INHERITED = SkSVGGradient;
};

#endif // SkSVGRadialGradient_DEFINED
