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
 * @file set_stress_test.cpp
 * @brief Stress tests for WTree set and map with keys of different byte-size.
 *
 * Exercises massive insertions with validation on multiple key types:
 *   - uint   (4 bytes)
 *   - ushort (2 bytes)
 *   - unsigned char (1 byte)
 *   - ulong keys (8-byte keys) on big nodes.
 */

#include <cstdlib>
#include <ctime>

#include "set_stress_namespace.hpp"

const std::string title = "Set Stress Test";

using namespace std;
using namespace TestPrinting;

int main() {
    bool res;

    { // uint keys (4 bytes)
        using KeyType = uint;
        res = SetStressNamespace::Test_massive_insert<KeyType, 256>();
        if(!res) {
            test_incorrect(title);
            return EXIT_FAILURE;
        }
    }

    { // ushort keys (2 bytes)
        using KeyType = ushort;
        res = SetStressNamespace::Test_massive_insert<KeyType, 256>();
        if(!res) {
            test_incorrect(title);
            return EXIT_FAILURE;
        }
    }

    { // unsigned char keys (1 byte)
        using KeyType = unsigned char;
        res = SetStressNamespace::Test_massive_insert<KeyType, 256>();
        if(!res) {
            test_incorrect(title);
            return EXIT_FAILURE;
        }
    }

    { // using ulong keys (8 bytes) on big nodes.
        using KeyType = ulong;
        res = SetStressNamespace::Test_massive_insert<KeyType, 4096>();
        if(!res) {
            test_incorrect(title);
            return EXIT_FAILURE;
        }
    }

    test_correct(title);
    return 0;
}
