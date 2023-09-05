/*
 * Copyright 2019 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGText_DEFINED
#define SkSVGText_DEFINED

#include <vector>

#include "modules/svg/include/SkSVGTransformableNode.h"
#include "modules/svg/include/SkSVGTypes.h"

class SK_API SkSVGTextContext;

// Base class for text-rendering nodes.
class SkSVGTextFragment : public SkSVGTransformableNode {
public:
    void renderText(const SkSVGRenderContext&, SkSVGTextContext*, SkSVGXmlSpace) const;

protected:
    explicit SkSVGTextFragment(SkSVGTag t) : INHERITED(t) {}
    SkSVGTextFragment(const SkSVGTextFragment& other) : INHERITED(other) {}

    virtual void onShapeText(const SkSVGRenderContext&, SkSVGTextContext*, SkSVGXmlSpace) const = 0;

    // Text nodes other than the root <text> element are not rendered directly.
    void onRender(const SkSVGRenderContext&) const override {}

private:
    SkPath onAsPath(const SkSVGRenderContext&) const override;

    using INHERITED = SkSVGTransformableNode;
};

// Base class for nestable text containers (<text>, <tspan>, etc).
class SkSVGTextContainer : public SkSVGTextFragment {
public:
    SVG_ATTR(X, std::vector<SkSVGLength>, {})
    SVG_ATTR(Y, std::vector<SkSVGLength>, {})
    SVG_ATTR(Dx, std::vector<SkSVGLength>, {})
    SVG_ATTR(Dy, std::vector<SkSVGLength>, {})
    SVG_ATTR(Rotate, std::vector<SkSVGNumberType>, {})
    SVG_ATTR(LetterSpacing, SkSVGLength, SkSVGLength(0))

    SVG_ATTR(XmlSpace, SkSVGXmlSpace, SkSVGXmlSpace::kDefault)

    void appendChild(sk_sp<SkSVGNode>) final;

protected:
    explicit SkSVGTextContainer(SkSVGTag t) : INHERITED(t) {}
    SkSVGTextContainer(const SkSVGTextContainer& other) : INHERITED(other)
        , fX(other.fX), fY(other.fY), fDx(other.fDx), fDy(other.fDy)
        , fRotate(other.fRotate) {
        for (int i = 0; i < other.fChildren.size(); ++i) {
            this->appendChild(other.fChildren[i]);
        }
    }

    void onShapeText(const SkSVGRenderContext&, SkSVGTextContext*, SkSVGXmlSpace) const override;

    bool parseAndSetAttribute(const char*, const char*) override;

private:
    std::vector<sk_sp<SkSVGTextFragment>> fChildren;

    using INHERITED = SkSVGTextFragment;
};

class SkSVGText final : public SkSVGTextContainer {
public:
    static sk_sp<SkSVGText> Make() { return sk_sp<SkSVGText>(new SkSVGText()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGText>(new SkSVGText(*this));
    }

private:
    SkSVGText() : INHERITED(SkSVGTag::kText) {}
    SkSVGText(const SkSVGText& other) : INHERITED(other) {}

    void onRender(const SkSVGRenderContext&) const override;

    SkRect onObjectBoundingBox(const SkSVGRenderContext&) const override;
    SkPath onAsPath(const SkSVGRenderContext&) const override;

    using INHERITED = SkSVGTextContainer;
};

class SkSVGTSpan final : public SkSVGTextContainer {
public:
    static sk_sp<SkSVGTSpan> Make() { return sk_sp<SkSVGTSpan>(new SkSVGTSpan()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGTSpan>(new SkSVGTSpan(*this));
    }

private:
    SkSVGTSpan() : INHERITED(SkSVGTag::kTSpan) {}
    SkSVGTSpan(const SkSVGTSpan& other) : INHERITED(other) {}

    using INHERITED = SkSVGTextContainer;
};

class SkSVGTextLiteral final : public SkSVGTextFragment {
public:
    static sk_sp<SkSVGTextLiteral> Make() {
        return sk_sp<SkSVGTextLiteral>(new SkSVGTextLiteral());
    }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGTextLiteral>(new SkSVGTextLiteral(*this));
    }

    SVG_ATTR(Text, SkSVGStringType, SkSVGStringType())

private:
    SkSVGTextLiteral() : INHERITED(SkSVGTag::kTextLiteral) {}
    SkSVGTextLiteral(const SkSVGTextLiteral& other) : INHERITED(other), fText(other.fText) {}

    void onShapeText(const SkSVGRenderContext&, SkSVGTextContext*, SkSVGXmlSpace) const override;

    void appendChild(sk_sp<SkSVGNode>) override {}

    using INHERITED = SkSVGTextFragment;
};

class SkSVGTextPath final : public SkSVGTextContainer {
public:
    static sk_sp<SkSVGTextPath> Make() { return sk_sp<SkSVGTextPath>(new SkSVGTextPath()); }

    sk_sp<SkSVGNode> makeShallowClone() const override {
        return sk_sp<SkSVGTextPath>(new SkSVGTextPath(*this));
    }

    SVG_ATTR(Href       , SkSVGIRI   , {}  )
    SVG_ATTR(StartOffset, SkSVGLength, SkSVGLength(0))

private:
    SkSVGTextPath() : INHERITED(SkSVGTag::kTextPath) {}
    SkSVGTextPath(const SkSVGTextPath& other) : INHERITED(other)
        , fHref(other.fHref), fStartOffset(other.fStartOffset) {}

    void onShapeText(const SkSVGRenderContext&, SkSVGTextContext*, SkSVGXmlSpace) const override;
    bool parseAndSetAttribute(const char*, const char*) override;

    using INHERITED = SkSVGTextContainer;
};

#endif  // SkSVGText_DEFINED
