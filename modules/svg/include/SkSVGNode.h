/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGNode_DEFINED
#define SkSVGNode_DEFINED

#include <map>

#include "include/core/SkRefCnt.h"
#include "include/private/base/SkTArray.h"
#include "modules/svg/include/SkSVGAttribute.h"
#include "modules/svg/include/SkSVGAttributeParser.h"

#include <third_party/externals/cssparser/CssParser/include/resolvers/INodeSelector.h>

class SkCanvas;
class SkMatrix;
class SkPaint;
class SkPath;
class SkSVGLengthContext;
class SkSVGRenderContext;
class SkSVGValue;

enum class SkSVGTag {
    kAnimate,
    kAnimateTransform,
    kAnimateMotion,
    kCircle,
    kClipPath,
    kDefs,
    kEllipse,
    kFeBlend,
    kFeColorMatrix,
    kFeComposite,
    kFeDiffuseLighting,
    kFeDisplacementMap,
    kFeDistantLight,
    kFeDropShadow,
    kFeFlood,
    kFeGaussianBlur,
    kFeImage,
    kFeMerge,
    kFeMergeNode,
    kFeMorphology,
    kFeOffset,
    kFePointLight,
    kFeSpecularLighting,
    kFeSpotLight,
    kFeTurbulence,
    kFilter,
    kG,
    kImage,
    kLine,
    kLinearGradient,
    kMarker,
    kMask,
    kPath,
    kPattern,
    kPolygon,
    kPolyline,
    kRadialGradient,
    kRect,
    kStop,
    kStyle,
    kSvg,
    kSymbol,
    kText,
    kTextLiteral,
    kTextPath,
    kTSpan,
    kUse
};

#define SVG_PRES_ATTR(attr_name, attr_type, attr_inherited)                  \
private:                                                                     \
    bool set##attr_name(SkSVGAttributeParser::ParseResult<                   \
                            SkSVGProperty<attr_type, attr_inherited>>&& pr) {\
        if (pr.isValid()) { this->set##attr_name(std::move(*pr)); }          \
        return pr.isValid();                                                 \
    }                                                                        \
                                                                             \
public:                                                                      \
    const SkSVGProperty<attr_type, attr_inherited>& get##attr_name() const { \
        return fPresentationAttributes.f##attr_name;                         \
    }                                                                        \
    void set##attr_name(const SkSVGProperty<attr_type, attr_inherited>& v) { \
        auto* dest = &fPresentationAttributes.f##attr_name;                  \
        if (!dest->isInheritable() || v.isValue()) {                         \
            /* TODO: If dest is not inheritable, handle v == "inherit" */    \
            *dest = v;                                                       \
        } else {                                                             \
            dest->set(SkSVGPropertyState::kInherit);                         \
        }                                                                    \
    }                                                                        \
    void set##attr_name(SkSVGProperty<attr_type, attr_inherited>&& v) {      \
        auto* dest = &fPresentationAttributes.f##attr_name;                  \
        if (!dest->isInheritable() || v.isValue()) {                         \
            /* TODO: If dest is not inheritable, handle v == "inherit" */    \
            *dest = std::move(v);                                            \
        } else {                                                             \
            dest->set(SkSVGPropertyState::kInherit);                         \
        }                                                                    \
    }

class SK_API SkSVGNode : public SkRefCnt, public INodeSelector {
public:
    ~SkSVGNode() override;

    SkSVGTag tag() const { return fTag; }
    SkString tagName() const { return fTagName; }

    // TODO: This value should be set by the child classes themselves
    void setTagName(const char* name) { fTagName = SkString(name); }

    virtual void appendChild(sk_sp<SkSVGNode>) = 0;

    void render(const SkSVGRenderContext&) const;
    bool asPaint(const SkSVGRenderContext&, SkPaint*) const;
    SkPath asPath(const SkSVGRenderContext&) const;
    SkRect objectBoundingBox(const SkSVGRenderContext&) const;

    void setAttribute(SkSVGAttribute, const SkSVGValue&);
    bool setAttribute(const char* attributeName, const char* attributeValue);

    // TODO: consolidate with existing setAttribute
    virtual bool parseAndSetAttribute(const char* name, const char* value);

    virtual sk_sp<SkSVGNode> makeShallowClone() const = 0;

    // inherited
    SVG_PRES_ATTR(BlendMode                , SkSVGBlendMode    , true)
    SVG_PRES_ATTR(ClipRule                 , SkSVGFillRule     , true)
    SVG_PRES_ATTR(Color                    , SkSVGColorType    , true)
    SVG_PRES_ATTR(ColorInterpolation       , SkSVGColorspace   , true)
    SVG_PRES_ATTR(ColorInterpolationFilters, SkSVGColorspace   , true)
    SVG_PRES_ATTR(CornerRadius             , SkSVGLength       , true)
    SVG_PRES_ATTR(FillRule                 , SkSVGFillRule     , true)
    SVG_PRES_ATTR(Fill                     , SkSVGPaint        , true)
    SVG_PRES_ATTR(FillOpacity              , SkSVGNumberType   , true)
    SVG_PRES_ATTR(FontFamily               , SkSVGFontFamily   , true)
    SVG_PRES_ATTR(FontSize                 , SkSVGFontSize     , true)
    SVG_PRES_ATTR(FontStyle                , SkSVGFontStyle    , true)
    SVG_PRES_ATTR(FontWeight               , SkSVGFontWeight   , true)
    SVG_PRES_ATTR(Stroke                   , SkSVGPaint        , true)
    SVG_PRES_ATTR(StrokeDashArray          , SkSVGDashArray    , true)
    SVG_PRES_ATTR(StrokeDashOffset         , SkSVGLength       , true)
    SVG_PRES_ATTR(StrokeLineCap            , SkSVGLineCap      , true)
    SVG_PRES_ATTR(StrokeLineJoin           , SkSVGLineJoin     , true)
    SVG_PRES_ATTR(StrokeMiterLimit         , SkSVGNumberType   , true)
    SVG_PRES_ATTR(StrokeOpacity            , SkSVGNumberType   , true)
    SVG_PRES_ATTR(StrokeWidth              , SkSVGLength       , true)
    SVG_PRES_ATTR(TextAnchor               , SkSVGTextAnchor   , true)
    SVG_PRES_ATTR(TextTransform            , SkSVGTextTransform, true)
    SVG_PRES_ATTR(Visibility               , SkSVGVisibility   , true)

    // not inherited
    SVG_PRES_ATTR(ClipPath                 , SkSVGFuncIRI      , false)
    SVG_PRES_ATTR(Display                  , SkSVGDisplay      , false)
    SVG_PRES_ATTR(Mask                     , SkSVGFuncIRI      , false)
    SVG_PRES_ATTR(Filter                   , SkSVGFuncIRI      , false)
    SVG_PRES_ATTR(Opacity                  , SkSVGNumberType   , false)
    SVG_PRES_ATTR(StopColor                , SkSVGColor        , false)
    SVG_PRES_ATTR(StopOpacity              , SkSVGNumberType   , false)
    SVG_PRES_ATTR(FloodColor               , SkSVGColor        , false)
    SVG_PRES_ATTR(FloodOpacity             , SkSVGNumberType   , false)
    SVG_PRES_ATTR(LightingColor            , SkSVGColor        , false)
                                                               
    SVG_PRES_ATTR(Class                    , SkSVGStringType   , false)
    SVG_PRES_ATTR(Id                       , SkSVGStringType   , false)
    SVG_PRES_ATTR(Style                    , SkSVGStringType   , false)

public:
    // INodeSelector Implementation 
    // For CSS styling

    #define GET_ATTR_STR(attr_name)                            \
        auto strAttr = &fPresentationAttributes.f##attr_name;  \
        return (strAttr->isValue() ? (*strAttr)->c_str() : "")

    virtual std::string GetTag()     override { return fTagName.c_str(); }
    virtual std::string GetId()      override { GET_ATTR_STR(Id); }
    virtual std::vector<std::string> GetClasses() override { return fClasses; }

    virtual std::pair<std::string, std::string> GetAttribute(std::string key) override {
        return fAttributes.find(key) != fAttributes.end() ? 
            std::make_pair(key, fAttributes[key]) : std::make_pair("", "");
    }

    virtual INodeSelector* GetParent() override { return fParent; }

    virtual std::vector<INodeSelector*> GetChildren() override {
        return std::vector<INodeSelector*>(); // Return empty vector 
    }

    virtual int GetIndexWithinParent() override { return fIndexWithinParent; }

    // Additional CSS accessors
    void setParent(SkSVGNode* parent) { fParent = parent; }
    void setIndexInParent(int idx) { fIndexWithinParent = idx; }
    std::string GetInlineStyle() { GET_ATTR_STR(Style); }
    virtual skia_private::STArray<1, sk_sp<SkSVGNode>, true> GetChildrenDirect() {
        return skia_private::STArray<1, sk_sp<SkSVGNode>, true>();
    }

protected:
    SkSVGNode(SkSVGTag);
    SkSVGNode(const SkSVGNode&);

    static SkMatrix ComputeViewboxMatrix(const SkRect&, const SkRect&, SkSVGPreserveAspectRatio);

    // Called before onRender(), to apply local attributes to the context.  Unlike onRender(),
    // onPrepareToRender() bubbles up the inheritance chain: overriders should always call
    // INHERITED::onPrepareToRender(), unless they intend to short-circuit rendering
    // (return false).
    // Implementations are expected to return true if rendering is to continue, or false if
    // the node/subtree rendering is disabled.
    virtual bool onPrepareToRender(SkSVGRenderContext*) const;

    virtual void onRender(const SkSVGRenderContext&) const = 0;

    virtual bool onAsPaint(const SkSVGRenderContext&, SkPaint*) const { return false; }

    virtual SkPath onAsPath(const SkSVGRenderContext&) const = 0;

    virtual void onSetAttribute(SkSVGAttribute, const SkSVGValue&) {}

    virtual bool hasChildren() const { return false; }

    virtual SkRect onObjectBoundingBox(const SkSVGRenderContext&) const {
        return SkRect::MakeEmpty();
    }

    bool setFontAttribute(const char* stringValue);
    void resolveFontSizes(SkSVGRenderContext* ctx) const;

    SkScalar getComputedFontSize() const { return fComputedFontSize; }

private:
    SkSVGTag                    fTag;
    SkString                    fTagName;
    SkSVGNode*                  fParent;
    int                         fIndexWithinParent;

    std::vector<std::string> fClasses;
    std::map<std::string, std::string> fAttributes;

    // FIXME: this should be sparse
    SkSVGPresentationAttributes fPresentationAttributes;
    mutable SkScalar            fComputedFontSize; // Needed for size calculations

    using INHERITED = SkRefCnt;
};

#undef SVG_PRES_ATTR // presentation attributes are only defined for the base class

#define _SVG_ATTR_SETTERS(attr_name, attr_type, attr_default, set_cp, set_mv) \
    private:                                                                  \
        bool set##attr_name(                                                  \
                const SkSVGAttributeParser::ParseResult<attr_type>& pr) {     \
            if (pr.isValid()) { this->set##attr_name(*pr); }                  \
            return pr.isValid();                                              \
        }                                                                     \
        bool set##attr_name(                                                  \
                SkSVGAttributeParser::ParseResult<attr_type>&& pr) {          \
            if (pr.isValid()) { this->set##attr_name(std::move(*pr)); }       \
            return pr.isValid();                                              \
        }                                                                     \
    public:                                                                   \
        void set##attr_name(const attr_type& a) { set_cp(a); }                \
        void set##attr_name(attr_type&& a) { set_mv(std::move(a)); }

#define SVG_ATTR(attr_name, attr_type, attr_default)                        \
    private:                                                                \
        attr_type fDef##attr_name = attr_default;                           \
        attr_type f##attr_name = attr_default;                              \
    public:                                                                 \
        const attr_type& getDefault##attr_name() const { return fDef##attr_name; } \
        const attr_type& get##attr_name() const { return f##attr_name; }    \
    _SVG_ATTR_SETTERS(                                                      \
            attr_name, attr_type, attr_default,                             \
            [this](const attr_type& a) { this->f##attr_name = a; },         \
            [this](attr_type&& a) { this->f##attr_name = std::move(a); })

#define SVG_OPTIONAL_ATTR(attr_name, attr_type)                                   \
    private:                                                                      \
        SkTLazy<attr_type> f##attr_name;                                          \
    public:                                                                       \
        const SkTLazy<attr_type>& get##attr_name() const { return f##attr_name; } \
    _SVG_ATTR_SETTERS(                                                            \
            attr_name, attr_type, attr_default,                                   \
            [this](const attr_type& a) { this->f##attr_name.set(a); },            \
            [this](attr_type&& a) { this->f##attr_name.set(std::move(a)); })

#endif // SkSVGNode_DEFINED
