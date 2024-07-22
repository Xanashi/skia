/*
 * Copyright 2024 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGFeMerge_DEFINED
#define SkSVGFeMerge_DEFINED

#include "modules/svg/include/SkSVGFe.h"
#include "modules/svg/include/SkSVGTypes.h"

class SkSVGFeMergeNode;

class SK_API SkSVGFeMerge final : public SkSVGFe {
public:
    static sk_sp<SkSVGFeMerge> Make() {
        return sk_sp<SkSVGFeMerge>(new SkSVGFeMerge());
    }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGFeMerge>(new SkSVGFeMerge(*this));
    }

protected:
    void appendChild(sk_sp<SkSVGNode>) override;

    sk_sp<SkImageFilter> onMakeImageFilter(const SkSVGRenderContext&,
                                           const SkSVGFilterContext&) const override;

    std::vector<SkSVGFeInputType> getInputs() const override;

private:
    SkSVGFeMerge() : INHERITED(SkSVGTag::kFeMerge) {}
    SkSVGFeMerge(const SkSVGFeMerge& other) : INHERITED(other) {}

    std::vector<sk_sp<SkSVGNode>> fMergeNodes;

    using INHERITED = SkSVGFe;
};

class SK_API SkSVGFeMergeNode final : public SkSVGFe {
public:
    static sk_sp<SkSVGFeMergeNode> Make() {
        return sk_sp<SkSVGFeMergeNode>(new SkSVGFeMergeNode());
    }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGFeMergeNode>(new SkSVGFeMergeNode(*this));
    }

    SkSVGFeInputType getInput() const { return this->getIn(); }

    sk_sp<SkImageFilter> getInputImageFilter(const SkSVGRenderContext& ctx,
                                             const SkSVGFilterContext& fctx) const {
        return this->onMakeImageFilter(ctx, fctx);
    }

    SkRect getFilterSubregion(const SkSVGRenderContext& ctx,
                              const SkSVGFilterContext& fctx) const {
        return this->resolveFilterSubregion(ctx, fctx);
    }

protected:
    sk_sp<SkImageFilter> onMakeImageFilter(const SkSVGRenderContext&,
                                           const SkSVGFilterContext&) const override;

    std::vector<SkSVGFeInputType> getInputs() const override { return {this->getIn()}; }

private:
    SkSVGFeMergeNode() : INHERITED(SkSVGTag::kFeMergeNode) {}
    SkSVGFeMergeNode(const SkSVGFeMergeNode& other) : INHERITED(other) {}

    using INHERITED = SkSVGFe;
};

#endif  // SkSVGFeMerge_DEFINED
