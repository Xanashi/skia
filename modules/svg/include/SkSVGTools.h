/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define GETVAL(val, def) val.isValid() ? *val : def

template <typename T> int 
inherit_if_needed(const SkTLazy<T>& src, SkTLazy<T>& dst) {
    if (!dst.isValid()) {
        dst = src;
        return 1;
    }
    return 0;
}