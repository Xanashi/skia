/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGGradient_DEFINED
#define SkSVGGradient_DEFINED

#include "include/core/SkShader.h"
#include "modules/svg/include/SkSVGHiddenContainer.h"
#include "modules/svg/include/SkSVGTypes.h"

class SkMatrix;
class SkSVGRenderContext;
class SkSVGStop;

class SK_API SkSVGGradient : public SkSVGHiddenContainer {
public:
    SVG_ATTR(Href, SkSVGIRI, SkSVGIRI())
    SVG_OPTIONAL_ATTR(GradientTransform, SkSVGTransformType)
    SVG_OPTIONAL_ATTR(SpreadMethod, SkSVGSpreadMethod)
    SVG_OPTIONAL_ATTR(GradientUnits, SkSVGObjectBoundingBoxUnits)

protected:
    explicit SkSVGGradient(SkSVGTag t) : INHERITED(t) {}

    bool parseAndSetAttribute(const char*, const char*) override;

    bool onAsPaint(const SkSVGRenderContext&, SkPaint*) const final;

    virtual bool applyAttributes(sk_sp<SkSVGGradient>*) const;

    virtual sk_sp<SkShader> onMakeShader(const SkSVGRenderContext&,
                                         const SkColor4f*, const SkScalar*, int count,
                                         SkTileMode, const SkSVGObjectBoundingBoxUnits,
                                         const SkMatrix& localMatrix) const = 0;

private:
    using StopPositionArray = skia_private::STArray<2, SkScalar , true>;
    using    StopColorArray = skia_private::STArray<2, SkColor4f, true>;
    void getGradientWithTemplate(const SkSVGRenderContext& ctx, sk_sp<SkSVGGradient>* gradient,
                                 StopPositionArray* pos, StopColorArray* colors, int hrefCount) const;
    void collectColorStops(const SkSVGRenderContext&, StopPositionArray*, StopColorArray*) const;
    SkColor4f resolveStopColor(const SkSVGRenderContext&, const SkSVGStop&) const;

    using INHERITED = SkSVGHiddenContainer;
};

#endif // SkSVGGradient_DEFINED
