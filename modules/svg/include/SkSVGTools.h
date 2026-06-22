/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#define GETVAL(val, def) val.has_value() ? *val : def

template <typename T> int 
inherit_if_needed(const std::optional<T>& src, std::optional<T>& dst) {
    if (!dst.has_value()) {
        dst = src;
        return 1;
    }
    return 0;
}