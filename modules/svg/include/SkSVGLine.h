/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGLine_DEFINED
#define SkSVGLine_DEFINED

#include "modules/svg/include/SkSVGShape.h"
#include "modules/svg/include/SkSVGTypes.h"

struct SkPoint;

class SK_API SkSVGLine final : public SkSVGShape {
public:
    static sk_sp<SkSVGLine> Make() { return sk_sp<SkSVGLine>(new SkSVGLine()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGLine>(new SkSVGLine(*this));
    }

    SVG_ATTR(X1, SkSVGLength, SkSVGLength(0))
    SVG_ATTR(Y1, SkSVGLength, SkSVGLength(0))
    SVG_ATTR(X2, SkSVGLength, SkSVGLength(0))
    SVG_ATTR(Y2, SkSVGLength, SkSVGLength(0))

protected:
    bool parseAndSetAttribute(const char*, const char*) override;

    const SkPath* onResolvePath(const SkSVGRenderContext&) const override;

    void onDraw(SkCanvas*, const SkSVGLengthContext&, const SkPaint&,
                SkPathFillType) const override;

    SkPath onAsPath(const SkSVGRenderContext&) const override;

private:
    SkSVGLine();
    SkSVGLine(const SkSVGLine& other) : INHERITED(other)
        , fX1(other.fX1), fY1(other.fY1), fX2(other.fX2), fY2(other.fY2)
        , fP0(other.fP0), fP1(other.fP1), fPath(other.fPath) {}

    // resolve and return the two endpoints
    std::tuple<SkPoint, SkPoint> resolve(const SkSVGLengthContext&) const;

    using INHERITED = SkSVGShape;
  
    mutable SkPoint fP0, fP1;
    mutable SkPath fPath;
};

#endif // SkSVGLine_DEFINED
