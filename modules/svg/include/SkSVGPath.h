/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGPath_DEFINED
#define SkSVGPath_DEFINED

#include "include/core/SkPath.h"
#include "modules/svg/include/SkSVGShape.h"

class SK_API SkSVGPath final : public SkSVGShape {
public:
    static sk_sp<SkSVGPath> Make() { return sk_sp<SkSVGPath>(new SkSVGPath()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGPath>(new SkSVGPath(*this));
    }

    SVG_ATTR(Path, SkPath, SkPath())

protected:
    bool parseAndSetAttribute(const char*, const char*) override;

    const SkPath* onResolvePath(const SkSVGRenderContext&) const override;

    void onDraw(SkCanvas*, const SkSVGLengthContext&, const SkPaint&,
                SkPathFillType) const override;

    SkPath onAsPath(const SkSVGRenderContext&) const override;

    SkRect onObjectBoundingBox(const SkSVGRenderContext&) const override;

    bool onSupportsMarkers() const override { return true; }

    void onApplyAnimationNode(const SkSVGAnimate*) override;

private:
    SkSVGPath();
    SkSVGPath(const SkSVGPath& other) : INHERITED(other), fPath(other.fPath) {}

    using INHERITED = SkSVGShape;
};

#endif // SkSVGPath_DEFINED
