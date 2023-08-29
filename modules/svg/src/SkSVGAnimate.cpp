
#include "modules/svg/include/SkSVGAnimate.h"
#include "src/core/SkStringUtils.h"

bool SkSVGAnimate::parseAndSetAttribute(const char* name, const char* value) {
    return INHERITED::parseAndSetAttribute(name, value) ||
           this->setAttributeName(SkSVGAttributeParser::parse<SkSVGString>("attributeName", name, value)) ||
           this->setValues(SkSVGAttributeParser::parse<SkSVGString>("values", name, value)) ||
           this->setType(SkSVGAttributeParser::parse<SkSVGString>("type", name, value));
}

std::tuple<SkString, SkString> SkSVGAnimate::getFirstAttributeValue() const {
    skia_private::TArray<SkString> values;
    SkStrSplit(fValues.str().c_str(), ";", &values);
    auto value = !values.empty() ? values[0] : SkString();

    if (this->tag() == SkSVGTag::kAnimateTransform) {
        value = SkStringPrintf("%s(%s)", fType.str().c_str(), value.c_str());
    }

    return std::make_tuple(fAttributeName.str(), value);
}