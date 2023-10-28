/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/core/SkFontMgr.h"
#include "modules/svg/include/SkSVGCss.h"
#include "src/core/SkStringUtils.h"


void SkSVGCss::applyCssToNodeTree(const FontContext& ctx, const sk_sp<SkSVGNode>& root,
                                  const SkString& cssStylesheet) {
    future::CSSParser* parser = new future::CSSParser;
    parser->parseByString(cssStylesheet.c_str());

    addEmbeddedFonts(ctx, parser->getKeywords());

    getNodeStyles(root.get(), parser->getSelectors());
}

void SkSVGCss::getNodeStyles(SkSVGNode* node, const std::vector<future::Selector*>& selectors) {
    auto children = node->GetChildrenDirect();
    for (const auto& child : children) {
        getNodeStyles(child.get(), selectors);
    }

    StyleMap styles;

    auto inlineStyle = node->GetInlineStyle();
    if (!inlineStyle.empty()) {
        auto inlineStyleMap = future::CSSUtil::GetRuleMapFromString(inlineStyle);
        applyStyleRules(styles, inlineStyleMap, 1000);
    }

    for (future::Selector* s : selectors) {
        if (future::NodeSelectorResolver::DoesNodeMatchSelector(node, s)) {
            auto spec = s->weight();
            auto ruleMap = s->getRuleDataMap();

            applyStyleRules(styles, ruleMap, spec);
        }
    }

    for (const auto& style : styles.getMap()) {
        if (node->parseAndSetAttribute(style.first.c_str(), style.second.first.c_str())) {
        }
    }
}

void SkSVGCss::applyStyleRules(StyleMap& styles, std::map<std::string, std::string>& ruleMap,
                               int specificity) {
    for (const auto& rule : ruleMap) {
        auto& key = rule.first;
        if (!styles.contains(key) || specificity > styles.getSpecificity(key))
            styles.addOrOverwrite(key, rule.second, specificity);
    }
}

void SkSVGCss::addEmbeddedFonts(const FontContext& ctx,
                                const std::vector<future::KeywordItem*>& keywords) {
    for (const auto& keyword : keywords) {
        if (keyword->getName() != "@font-face") continue;

        auto fontface = future::CSSUtil::GetRuleMapFromString(keyword->getData());

        if (fontface.count("font-family") && fontface.count("src") && fontface["src"].size() > 4) {
            auto delim = fontface["src"][4];
            auto fontSrc = SkStrBetweenChars(fontface["src"], delim, delim);

            auto tf = ctx.fResourceProvider->loadTypeface(fontface["font-family"].c_str(),
                                                          fontSrc.c_str());
            if (tf) {
                ctx.fFontMgr->addEmbeddedFont(tf);
            }
        }
    }
}