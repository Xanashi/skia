/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "modules/svg/include/SkSVGUse.h"

#include "include/core/SkCanvas.h"
#include "modules/svg/include/SkSVGRenderContext.h"
#include "modules/svg/include/SkSVGValue.h"
#include "modules/svg/include/SkSVGAnimate.h"
#include "modules/svg/include/SkSVGSymbol.h"

SkSVGUse::SkSVGUse() : INHERITED(SkSVGTag::kUse) {}

void SkSVGUse::appendChild(sk_sp<SkSVGNode> node) {
    if (node->tag() == SkSVGTag::kAnimateTransform) {
        if (!isTransformSet()) {
            const auto [n, v] = static_cast<const SkSVGAnimate*>(node.get())->getFirstAttributeValue();
            auto parseResult = SkSVGAttributeParser::parse<SkSVGTransformType>(v.c_str());
            if (parseResult.isValid()) {
                this->setTransform(SkSVGTransformValue(*parseResult));
            }        
        }
    } else {
        SkDebugf("cannot append child nodes to an SVG Use element.\n");
    }
}

bool SkSVGUse::parseAndSetAttribute(const char* n, const char* v) {
    bool parsed = INHERITED::parseAndSetAttribute(n, v) ||
           this->setX(SkSVGAttributeParser::parse<SkSVGLength>("x", n, v)) ||
           this->setY(SkSVGAttributeParser::parse<SkSVGLength>("y", n, v)) ||
           this->setHref(SkSVGAttributeParser::parse<SkSVGIRI>("href", n, v)) ||
           this->setHref(SkSVGAttributeParser::parse<SkSVGIRI>("xlink:href", n, v));

    if (!parsed) {
        attrs[SkString(n)] = SkString(v);  // Apply any non-inherited attributes to the child elements
    }

    return parsed;
}

bool SkSVGUse::onPrepareToRender(SkSVGRenderContext* ctx) const {
    if (fHref.iri().isEmpty() || !INHERITED::onPrepareToRender(ctx)) {
        return false;
    }

    if (fX.value() || fY.value()) {
        // Restored when the local SkSVGRenderContext leaves scope.
        ctx->saveOnce();
        ctx->canvas()->translate(fX.value(), fY.value());
    }

    // TODO: width/height override for <svg> targets.

    return true;
}

void SkSVGUse::onRender(const SkSVGRenderContext& ctx) const {
    const auto ref = ctx.findNodeById(fHref);
    if (!ref) {
        return;
    }

    auto clone = ref->makeShallowClone();
    for (const auto& a : attrs) {
        clone->parseAndSetAttribute(a.first.c_str(), a.second.c_str());
    }

    if (clone->tag() == SkSVGTag::kSymbol) {
        static_cast<const SkSVGSymbol*>(clone.get())->renderSymbol(ctx);
    } else {
        clone->render(ctx);
    }
}

SkPath SkSVGUse::onAsPath(const SkSVGRenderContext& ctx) const {
    const auto ref = ctx.findNodeById(fHref);
    if (!ref) {
        return SkPath();
    }

    auto path = ref->asPath(ctx);

    const SkSVGLengthContext& lctx = ctx.lengthContext();
    const SkScalar x = lctx.resolve(fX, SkSVGLengthContext::LengthType::kHorizontal);
    const SkScalar y = lctx.resolve(fY, SkSVGLengthContext::LengthType::kVertical);

    if (x != 0 || y != 0) {
        path.offset(x, y);
    }

    return path;
}

SkRect SkSVGUse::onObjectBoundingBox(const SkSVGRenderContext& ctx) const {
    const auto ref = ctx.findNodeById(fHref);
    if (!ref) {
        return SkRect::MakeEmpty();
    }

    const SkSVGLengthContext& lctx = ctx.lengthContext();
    const SkScalar x = lctx.resolve(fX, SkSVGLengthContext::LengthType::kHorizontal);
    const SkScalar y = lctx.resolve(fY, SkSVGLengthContext::LengthType::kVertical);

    SkRect bounds = ref->objectBoundingBox(ctx);
    bounds.offset(x, y);

    return bounds;
}
