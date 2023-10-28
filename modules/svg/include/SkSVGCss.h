/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGCss_DEFINED
#define SkSVGCss_DEFINED

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <map>

#include "modules/svg/include/SkSVGNode.h"
#include "modules/skresources/include/SkResources.h"

#include <cssparser/CSSParser.h>
#include <resolvers/NodeSelectorResolver.h>
#include <utils/CSSUtil.h>

class SkFontMgr;

struct FontContext {
    FontContext(SkFontMgr* fontMgr, skresources::ResourceProvider* resourceProvider)
        : fFontMgr(fontMgr), fResourceProvider(resourceProvider) {}

    SkFontMgr* fFontMgr;
    skresources::ResourceProvider* fResourceProvider;
};

class SK_API SkSVGCss {
    class StyleMap {
    public:
        void addOrOverwrite(std::string key, std::string value, int specificity) {
            styleMap[key] = std::make_pair(value, specificity);
        }

        bool contains(std::string key)        { return styleMap.find(key) != styleMap.end(); }
        std::string getValue(std::string key) { return styleMap[key].first; }
        int getSpecificity(std::string key)   { return styleMap[key].second; }
        std::map<std::string, std::pair<std::string, int>> getMap() const { return styleMap; } 

    private:
        std::map<std::string, std::pair<std::string, int>> styleMap;
    };

public:
    void applyCssToNodeTree(const FontContext& ctx, const sk_sp<SkSVGNode>& root,
                            const SkString& cssStylesheet);

private:
    void getNodeStyles(SkSVGNode* node, const std::vector<future::Selector*>& selectors);
    void applyStyleRules(StyleMap& styles, std::map<std::string, std::string>& ruleMap,
                         int specificity);
    void addEmbeddedFonts(const FontContext& ctx,
                          const std::vector<future::KeywordItem*>& keywords);
};

#endif  // SkSVGCss_DEFINED