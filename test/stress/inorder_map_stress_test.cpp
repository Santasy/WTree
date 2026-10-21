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
 * @file inorder_map_stress_test.cpp
 * @brief In-order (sequential) insertion test for the WTree map container.
 *
 * The map holds the key/value pairs ("{index}", "val_{index}"), inserted in
 * ascending order.
 *
 * Usage: inorder_map_stress_test [final_size]
 *
 * The final size is the execution parameter of the suite; it defaults to
 * 100000 pairs when it is not given. The balance policy (slide / split) is a
 * compilation knob set from the CMake cache (WTREE_STRESS_SLIDE /
 * WTREE_STRESS_SPLIT).
 */

#include <cstdlib>

#include "inorder_map_stress_namespace.hpp"

const std::string title = "In-Order Map Stress Test";

using namespace std;
using namespace TestPrinting;

int main(int argc, char **argv) {
    const size_t final_size =
        InOrderMapStressNamespace::parse_final_size(argc, argv);

    printf("[[INFO]] Final size: %lu pairs.\n", (unsigned long)final_size);

    const bool res =
        InOrderMapStressNamespace::Test_inorder_insert<>(final_size);
    if(!res) {
        test_incorrect(title);
        return EXIT_FAILURE;
    }

    test_correct(title);
    return 0;
}
