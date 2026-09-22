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

#ifndef _WTREE_TEST_ERASE_STR_MAP_H_
#define _WTREE_TEST_ERASE_STR_MAP_H_

#include "../utils/testing.hpp"

#include "../../include/wtree/map.hpp"

#include <algorithm>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

namespace EraseStrMapNamespace {

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;

extern TestResults results;

template <int NodeSize> bool erase_all_ascending_strings(int count) {
    string t = "String-key erase all ascending (n=" + to_string(count) +
               ", Node=" + to_string(NodeSize) + ")";
    using WMap = map<string, string, less<string>, allocator<string>, NodeSize>;
    WMap m;
    for(int i = 0; i < count; ++i)
        m["user" + to_string(i)] = "v";
    if(m.size() != (size_t)count) {
        results.fail(t, "bad insert size");
        return false;
    }
    for(int i = 0; i < count; ++i)
        m.erase("user" + to_string(i));
    if(!m.empty()) {
        results.fail(t, "not empty: " + to_string(m.size()));
        return false;
    }
    results.pass(t);
    return true;
}

template <int NodeSize> bool erase_even_strings(int count) {
    string t = "String-key erase evens (n=" + to_string(count) +
               ", Node=" + to_string(NodeSize) + ")";
    using WMap = map<string, string, less<string>, allocator<string>, NodeSize>;
    WMap m;
    for(int i = 0; i < count; ++i)
        m["user" + to_string(i)] = "v";
    for(int i = 0; i < count; i += 2)
        m.erase("user" + to_string(i));
    bool ok = m.size() == (size_t)count / 2;
    for(int i = 1; ok && i < count; i += 2)
        if(m.find("user" + to_string(i)) == m.end())
            ok = false;
    for(int i = 0; ok && i < count; i += 2)
        if(m.find("user" + to_string(i)) != m.end())
            ok = false;
    ok ? results.pass(t) : results.fail(t, "content mismatch");
    return ok;
}

template <int NodeSize>
bool erase_all_hashed_strings(int count, unsigned seed = 42) {
    string t = "String-key erase all hashed (n=" + to_string(count) + ")";
    using WMap = map<string, string, less<string>, allocator<string>, NodeSize>;
    WMap m;
    vector<int> order(count);
    mt19937 g(seed);
    for(int i = 0; i < count; ++i)
        order[i] = i;
    shuffle(order.begin(), order.end(), g);
    for(int k : order)
        m["user" + to_string(k)] = "v";
    shuffle(order.begin(), order.end(), g);
    for(int k : order)
        m.erase("user" + to_string(k));
    bool ok = m.empty();
    ok ? results.pass(t) : results.fail(t, "not empty: " + to_string(m.size()));
    return ok;
}

} // namespace EraseStrMapNamespace

#endif