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

using strmap_type =
    WTreeLib::map<string, string, std::less<string>,
                  std::allocator<std::pair<const string, string>>, 256>;

void print_stats(auto &stats);
void example_collect_stats_from_tree();
void example_collect_stats_from_profile() {
    // TODO:
}

int main(int argc, char **argv) {
    example_collect_stats_from_tree();
    example_collect_stats_from_profile();
    return 0;
}

void example_collect_stats_from_tree() {
    strmap_type wide_map;

    cout << "=====\nEmpty map container:\n";
    auto stats = wide_map.tree()->collect_stats();
    print_stats(stats);

    const size_t size = 1'000;
    for(size_t i = 0; i < size; ++i) {
        string key = format("user00{}", i);
        string value = format("data{}data", i * 123);
        wide_map[key] = value;
    }

    cout << "\n=====\nMap with " << size << " keys:\n";
    stats = wide_map.tree()->collect_stats();
    print_stats(stats);
}

void print_stats(auto &stats) {
    cout << format("\"keys\": {:10},", stats.keys);
    cout << format("\"height\": {:3},\n", stats.height);
    cout << format("\"nodes\": {:6},", stats.nodes());
    cout << format("\"internals\": {:4},", stats.internal_nodes);
    cout << format("\"leaves\": {:4},", stats.leaf_nodes);
    cout << format("\"unused_keycells\": {:6},", stats.unused_keycells);
    cout << format("\"unused_ptrcells\": {:6},\n", stats.unused_pointers);
    cout << format("\"total_bytes\": {:10},", stats.bytes_used());
    cout << format("\"total_overhead\": {:10},", stats.total_overhead());
    cout << format("\"overhead\": {:f},", stats.overhead());
    cout << format("\"fullness\": {:f},", stats.fullness());
    cout << format("\"occupancy\": {:f},\n", stats.occupancy());
}
