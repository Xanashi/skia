/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGStop_DEFINED
#define SkSVGStop_DEFINED

#include "modules/svg/include/SkSVGHiddenContainer.h"
#include "modules/svg/include/SkSVGTypes.h"

class SkSVGLengthContext;

class SK_API SkSVGStop : public SkSVGHiddenContainer {
public:
    static sk_sp<SkSVGStop> Make() {
        return sk_sp<SkSVGStop>(new SkSVGStop());
    }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGStop>(new SkSVGStop(*this));
    }

    SVG_ATTR(Offset, SkSVGLength, SkSVGLength(0, SkSVGLength::Unit::kPercentage))

protected:
    bool parseAndSetAttribute(const char*, const char*) override;

private:
    SkSVGStop();
    SkSVGStop(const SkSVGStop& other) : INHERITED(other)
        , fOffset(other.fOffset) {}

    using INHERITED = SkSVGHiddenContainer;
};

#endif // SkSVGStop_DEFINED
