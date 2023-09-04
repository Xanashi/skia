
#include "modules/svg/include/SkSVGAnimate.h"
#include "src/core/SkStringUtils.h"

bool SkSVGAnimate::parseAndSetAttribute(const char* name, const char* value) {
    return INHERITED::parseAndSetAttribute(name, value) ||
           this->setAttributeName(SkSVGAttributeParser::parse<SkSVGStringType>("attributeName", name, value)) ||
           this->setValues(SkSVGAttributeParser::parse<SkSVGStringType>("values", name, value)) ||
           this->setType(SkSVGAttributeParser::parse<SkSVGStringType>("type", name, value));
}

std::tuple<SkString, SkString> SkSVGAnimate::getFirstAttributeValue() const {
    skia_private::TArray<SkString> values;
    SkStrSplit(fValues.c_str(), ";", &values);
    auto value = !values.empty() ? values[0] : SkString();

    if (this->tag() == SkSVGTag::kAnimateTransform) {
        value = SkStringPrintf("%s(%s)", fType.c_str(), value.c_str());
    }

    return std::make_tuple(fAttributeName, value);
}