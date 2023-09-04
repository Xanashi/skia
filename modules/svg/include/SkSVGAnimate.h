#ifndef SkSVGAnimate_DEFINED
#define SkSVGAnimate_DEFINED

#include <tuple>

#include "modules/svg/include/SkSVGHiddenContainer.h"
#include "modules/svg/include/SkSVGTypes.h"

class SK_API SkSVGAnimate final : public SkSVGHiddenContainer {
public:
    static sk_sp<SkSVGAnimate> Make(SkSVGAnimationTag animTag) {
        return sk_sp<SkSVGAnimate>(new SkSVGAnimate(animTag));
    }

    sk_sp<SkSVGNode> makeShallowClone() const override { 
        return sk_sp<SkSVGAnimate>(new SkSVGAnimate(*this));
    }

    std::tuple<SkString, SkString> getFirstAttributeValue() const;

    SVG_ATTR(AttributeName, SkSVGStringType, SkSVGStringType())
    SVG_ATTR(Values, SkSVGStringType, SkSVGStringType())
    SVG_ATTR(Type, SkSVGStringType, SkSVGStringType())

private:
    SkSVGTag GetAnimateTag(SkSVGAnimationTag animTag) {
        switch (animTag) {
            case SkSVGAnimationTag::kAnimate: return SkSVGTag::kAnimate;
            case SkSVGAnimationTag::kAnimateTransform: return SkSVGTag::kAnimateTransform;
            case SkSVGAnimationTag::kAnimateMotion: return SkSVGTag::kAnimateMotion;
        }
    }

    SkSVGAnimate(SkSVGAnimationTag animTag) : INHERITED(GetAnimateTag(animTag)) {}
    SkSVGAnimate(const SkSVGAnimate& other) : INHERITED(other) 
            , fAttributeName(other.fAttributeName), fValues(other.fValues), fType(other.fType) {}

    bool parseAndSetAttribute(const char*, const char*) override;

    using INHERITED = SkSVGHiddenContainer;
};

#endif  // SkSVGAnimate_DEFINED