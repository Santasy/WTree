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

#include <iostream>
#include <memory>
#include <string>

#include "../include/wtree/map.hpp"

#include "../include/wtree/optional/profile.hpp"

using namespace std;
using namespace WTreeLib;

#ifdef __BASIC_TEST_PAIR
bool test_pair();
#endif

struct DummyStruct {
    int x, y, z;
};
bool test_using_DummyStruct();

class DummyClass {
  public:
    int origin;
    vector<int> values;

    DummyClass(int origin, int n) : origin(origin) { values.assign(n, 1); }

    void printData() const {
        cout << "Origin: " << origin << "\n";
        cout << "Values: " << values.size() << "\n";
    }
};

int main(int argc, char **argv) {
    WTreeLib::map<DummyStruct, DummyClass, std::less<DummyStruct>,
                  std::allocator<std::pair<const DummyStruct, DummyClass>>, 128>
        wide_map;
    auto *wtree = wide_map.tree();

    WTreeMemoryInstrument reg;

    // WTreeProfiler<wide_map::params_type>::checkStatistics(*wtree, reg);
    // reg.evaluate<wide_map::value_type, class NODE>();
}