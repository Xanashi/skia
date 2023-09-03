/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGDefs_DEFINED
#define SkSVGDefs_DEFINED

#include "modules/svg/include/SkSVGHiddenContainer.h"

class SK_API SkSVGDefs : public SkSVGHiddenContainer {
public:
    static sk_sp<SkSVGDefs> Make() { return sk_sp<SkSVGDefs>(new SkSVGDefs()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGDefs>(new SkSVGDefs(*this));
    }

private:
    SkSVGDefs() : INHERITED(SkSVGTag::kDefs) {}
    SkSVGDefs(const SkSVGDefs& other) : INHERITED(other) {}

    using INHERITED = SkSVGHiddenContainer;
};

#endif // SkSVGDefs_DEFINED
