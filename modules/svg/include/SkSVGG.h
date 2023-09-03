/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGG_DEFINED
#define SkSVGG_DEFINED

#include "modules/svg/include/SkSVGContainer.h"
#include <map>

class SK_API SkSVGG : public SkSVGContainer {
public:
    static sk_sp<SkSVGG> Make() { return sk_sp<SkSVGG>(new SkSVGG()); }

    sk_sp<SkSVGNode> makeShallowClone() const override { 
        return sk_sp<SkSVGG>(new SkSVGG(*this));
    }

    void applyAttributesToChild(const sk_sp<SkSVGNode>& node) {
        for (const auto& a : attrs) {
            node->parseAndSetAttribute(a.first, a.second);
        }
    }

protected:
    bool parseAndSetAttribute(const char* n, const char* v) override {        
        bool parsed = INHERITED::parseAndSetAttribute(n, v);
        if (!parsed) {
            attrs[n] = v; // Apply any non-inherited attributes to the child elements
        }
        return parsed;
    }

private:
    SkSVGG() : INHERITED(SkSVGTag::kG) {}
    SkSVGG(const SkSVGG& other) : INHERITED(other) {}

    using INHERITED = SkSVGContainer;

    std::map<const char*, const char*> attrs;
};

#endif // SkSVGG_DEFINED
