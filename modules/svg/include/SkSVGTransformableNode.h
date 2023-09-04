/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGTransformableNode_DEFINED
#define SkSVGTransformableNode_DEFINED

#include "include/core/SkMatrix.h"
#include "modules/svg/include/SkSVGNode.h"

class SK_API SkSVGTransformableNode : public SkSVGNode {
public:
    SkSVGTransformType getTransform() const { return fTransform; }
    void setTransform(const SkSVGTransformType& t) { fTransform = t; }
    bool isTransformSet() const { return fTransform != SkMatrix::I(); }

protected:
    SkSVGTransformableNode(SkSVGTag);
    SkSVGTransformableNode(const SkSVGTransformableNode& other) : INHERITED(other)
        , fTransform(other.fTransform) {}

    bool onPrepareToRender(SkSVGRenderContext*) const override;

    void onSetAttribute(SkSVGAttribute, const SkSVGValue&) override;

    void mapToParent(SkPath*) const;

    void mapToParent(SkRect*) const;

private:
    // FIXME: should be sparse
    SkSVGTransformType fTransform;

    using INHERITED = SkSVGNode;
};

#endif // SkSVGTransformableNode_DEFINED
