/*
 * Copyright (c) 2026 Sebastián Pacheco Cáceres
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "../include/wtree/map.hpp"
#include <iostream>
#include <string>

using namespace std;
using namespace WTreeLib;

using WTreeLib::map;

struct StructWithMap {
    map<int, int> wmap;
};

template <typename K, typename V>
void init_struct(StructWithMap &s, map<K, V> m) {
    s.wmap = m;
}

template <typename K, typename V> void add_dummy_value(map<K, V> &wmap) {
    wmap[100] = 1000;
}

int main() {
    map<int, int> wmap;
    wmap[10] = 100;

    StructWithMap smap;
    init_struct(smap, wmap);

    add_dummy_value(wmap);

    cout << "From struct:\n";
    for(auto &item : smap.wmap) {
        cout << item.first << " ___ " << item.second << '\n';
    }

    cout << "Original map:\n";
    for(auto &item : wmap) {
        cout << item.first << " ___ " << item.second << '\n';
    }

    return 0;
}