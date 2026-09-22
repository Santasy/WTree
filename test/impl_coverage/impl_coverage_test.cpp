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

/**
 * @file impl_coverage_test.cpp
 * @brief Implementation-coverage test vs std::set/std::map (analysis suite).
 *
 * Runs the full std-parity battery — bounds, range/init-list construction,
 * hint-insert, swap and map insert_or_assign — driving the very same
 * operation sequences against the WTree and the std containers and comparing
 * outcomes. Tagged ANALYSIS_TEST so it is excluded from the default/core
 * runs; it is the single home of the std-parity checks.
 */

#include "std_parity.hpp"
#include "../utils/testing.hpp"

#include <cstdlib>
#include <cstring>

using namespace std;
using namespace TestPrinting;

int main(int argc, char **argv) {
    // Optional: byte-size shared by both WTree instantiations (256).
    constexpr int kNodeBytes = 256;

    cout << "========================================\n";
    cout << "WTREE DIFFERENTIAL TESTS (vs std::set/std::map)\n";
    cout << "========================================\n";

    using WSetType =
        WTreeLib::set<int, std::less<int>, std::allocator<int>, kNodeBytes>;
    using WMapType = WTreeLib::map<int, int, std::less<int>,
                                   std::allocator<int>, kNodeBytes>;

    bool all_passed = true;
    all_passed &= StdParity::run_all<WSetType, WMapType, kNodeBytes>();

    if(all_passed) {
        test_correct("Impl-Coverage Test");
        return EXIT_SUCCESS;
    }
    test_incorrect("Differential Test");
    return EXIT_FAILURE;
}