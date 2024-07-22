/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/core/SkStringUtils.h"

#include "include/core/SkString.h"
#include "include/private/base/SkAssert.h"
#include "include/private/base/SkTArray.h"
#include "src/base/SkFloatBits.h"
#include "src/base/SkUTF.h"

#include <cstring>

using namespace skia_private;

void SkAppendScalar(SkString* str, SkScalar value, SkScalarAsStringType asType) {
    switch (asType) {
        case kHex_SkScalarAsStringType:
            str->appendf("SkBits2Float(0x%08x)", SkFloat2Bits(value));
            break;
        case kDec_SkScalarAsStringType: {
            SkString tmp;
            tmp.printf("%.9g", value);
            if (tmp.contains('.')) {
                tmp.appendUnichar('f');
            }
            str->append(tmp);
            break;
        }
    }
}

SkString SkTabString(const SkString& string, int tabCnt) {
    if (tabCnt <= 0) {
        return string;
    }
    SkString tabs;
    for (int i = 0; i < tabCnt; ++i) {
        tabs.append("\t");
    }
    SkString result;
    static const char newline[] = "\n";
    const char* input = string.c_str();
    int nextNL = SkStrFind(input, newline);
    while (nextNL >= 0) {
        if (nextNL > 0) {
            result.append(tabs);
        }
        result.append(input, nextNL + 1);
        input += nextNL + 1;
        nextNL = SkStrFind(input, newline);
    }
    if (*input != '\0') {
        result.append(tabs);
        result.append(input);
    }
    return result;
}

SkString SkStringFromUTF16(const uint16_t* src, size_t count) {
    SkString ret;
    const uint16_t* stop = src + count;
    if (count > 0) {
        SkASSERT(src);
        size_t n = 0;
        const uint16_t* end = src + count;
        for (const uint16_t* ptr = src; ptr < end;) {
            const uint16_t* last = ptr;
            SkUnichar u = SkUTF::NextUTF16(&ptr, stop);
            size_t s = SkUTF::ToUTF8(u);
            if (n > UINT32_MAX - s) {
                end = last;  // truncate input string
                break;
            }
            n += s;
        }
        ret = SkString(n);
        char* out = ret.data();
        for (const uint16_t* ptr = src; ptr < end;) {
            out += SkUTF::ToUTF8(SkUTF::NextUTF16(&ptr, stop), out);
        }
        SkASSERT(out == ret.data() + n);
    }
    return ret;
}

void SkStrSplit(const char* str,
                const char* delimiters,
                SkStrSplitMode splitMode,
                TArray<SkString>* out) {
    if (splitMode == kCoalesce_SkStrSplitMode) {
        // Skip any delimiters.
        str += strspn(str, delimiters);
    }
    if (!*str) {
        return;
    }

    while (true) {
        // Find a token.
        const size_t len = strcspn(str, delimiters);
        if (splitMode == kStrict_SkStrSplitMode || len > 0) {
            out->push_back().set(str, len);
            str += len;
        }

        if (!*str) {
            return;
        }
        if (splitMode == kCoalesce_SkStrSplitMode) {
            // Skip any delimiters.
            str += strspn(str, delimiters);
        } else {
            // Skip one delimiter.
            str += 1;
        }
    }
}

inline bool is_whitespace(char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }

SkString SkStrTrim(SkString& s) {
    char* str = s.data();
    const char* start = str;              // start is inclusive
    const char* end = start + s.size();  // end is exclusive
    while (is_whitespace(*start)) {
        ++start;
    }
    if (start != end) {
        --end;  // make end inclusive
        while (is_whitespace(*end)) {
            --end;
        }
        ++end;  // make end exclusive
    }
    size_t len = end - start;
    memmove(str, start, len);
    s.resize(len);

    return s;
}

SkString SkStrBetweenChars(SkString s, char startChar, char endChar) {
    std::string result;

    const char* startPos = strchr(s.c_str(), startChar);
    if (startPos != nullptr) {
        const char* endPos = strchr(startPos + 1, endChar);
        if (endPos != nullptr) {
            size_t substringLength = endPos - (startPos + 1);
            result = std::string(startPos + 1, substringLength);
        }
    }

    return SkString(result);
}

std::string SkStrBetweenChars(const std::string& strValue, char startChar, char endChar) {
    std::string str = "";
    size_t pos1 = strValue.find(startChar);
    if (pos1 != std::string::npos) {
        size_t pos2 = strValue.find(endChar, pos1 + 1);
        if (pos2 != std::string::npos) {
            str = strValue.substr(pos1 + 1, pos2 - pos1 - 1);
        }
    }
    return str;
}

bool SkStrStrI(const char* haystack, const char* needle) {
    size_t haystackLength = std::strlen(haystack);
    size_t needleLength = std::strlen(needle);

    for (size_t i = 0; i <= haystackLength - needleLength; ++i) {
        size_t j = 0;
        while (j < needleLength && std::tolower(haystack[i + j]) == std::tolower(needle[j])) {
            ++j;
        }
        if (j == needleLength) {
            return true;
        }
    }

    return false;
}