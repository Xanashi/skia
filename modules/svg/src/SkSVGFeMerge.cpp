/*
 * Copyright 2020 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/effects/SkImageFilters.h"
#include "modules/svg/include/SkSVGAttributeParser.h"
#include "modules/svg/include/SkSVGFeMerge.h"
#include "modules/svg/include/SkSVGFilterContext.h"
#include "modules/svg/include/SkSVGRenderContext.h"

void SkSVGFeMerge::appendChild(sk_sp<SkSVGNode> node) {
    //auto mergeNode = reinterpret_cast<sk_sp<SkSVGFeMergeNode>>(node);
    if (node && node->tag() == SkSVGTag::kFeMergeNode) {
        fMergeNodes.push_back(node);
    }
}

std::vector<SkSVGFeInputType> SkSVGFeMerge::getInputs() const {
    std::vector<SkSVGFeInputType> mergeInputTypes; 
    
    for (const auto& mergeNode : fMergeNodes) {
        auto input = reinterpret_cast<SkSVGFeMergeNode*>(mergeNode.get())->getInput();
        mergeInputTypes.push_back(input);
    }
    
    return mergeInputTypes;
}

sk_sp<SkImageFilter> SkSVGFeMerge::onMakeImageFilter(const SkSVGRenderContext& ctx,
                                                     const SkSVGFilterContext& fctx) const {
    std::vector<sk_sp<SkImageFilter>> mergeFilters;
    
    for (const auto& mergeNode : fMergeNodes) {
        auto filter = reinterpret_cast<SkSVGFeMergeNode*>(mergeNode.get())->getInputImageFilter(ctx, fctx);
        mergeFilters.push_back(filter);
    }
    
    SkRect cropRect = this->resolveFilterSubregion(ctx, fctx);

    return mergeFilters.size() < 1 ? nullptr : 
        SkImageFilters::Merge(mergeFilters.data(), mergeFilters.size(), cropRect);
}

sk_sp<SkImageFilter> SkSVGFeMergeNode::onMakeImageFilter(const SkSVGRenderContext& ctx,
                                                         const SkSVGFilterContext& fctx) const {
    const SkSVGColorspace colorspace = this->resolveColorspace(ctx, fctx);
    return fctx.resolveInput(ctx, this->getIn(), colorspace);
}
