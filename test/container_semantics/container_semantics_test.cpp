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

#include <cstdlib>
#include <iostream>

#include "container_semantics_namespace.hpp"

const std::string title = "Container Semantics Test";

using namespace std;
using namespace TestPrinting;
using namespace ContainerSemanticsNamespace;

int main(int argc, char **argv) {
    bool all_passed = true;

    cout << "\n";
    cout << "============================================================\n";
    cout << "           WTree Container Semantics Tests                  \n";
    cout << "============================================================\n";

    all_passed &= Run_all_semantics_tests<WTreeContainerTag>();

    cout << "\n";
    cout << "============================================================\n";
    cout << "            STD Container Semantics Tests                   \n";
    cout << "============================================================\n";

    all_passed &= Run_all_semantics_tests<StdContainerTag>();

    cout << "\n";
    cout << "============================================================\n";
    cout << "          Balance Options (WTree-only) Tests                \n";
    cout << "============================================================\n";

    all_passed &= BalanceOptionsNamespace::Run_all_balance_options_tests();

    cout << "\n";
    cout << "============================================================\n";

    if(all_passed) {
        test_correct(title);
        return EXIT_SUCCESS;
    }

    test_incorrect(title);
    return EXIT_FAILURE;
}