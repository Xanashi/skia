/*
 * Copyright 2020 Google, LLC
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "fuzz/Fuzz.h"
#include "tools/fonts/FontToolUtils.h"

void fuzz_CreateDDL(Fuzz* f);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size > 4000) {
        return 0;
    }
    ToolUtils::UsePortableFontMgr();
    auto fuzz = Fuzz(data, size);
    fuzz_CreateDDL(&fuzz);
    return 0;
}
