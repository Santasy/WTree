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

#include "../utils/testing.hpp"

#include "erase_rules_namespace.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>

const std::string title = "Erase Rules Test";

using namespace std;
using namespace TestPrinting;
using namespace EraseRulesNamespace;

int main() {
    Printer printer;
    printer.options.color_output = true;
    bool result = true;

    result &= Test_only_root(printer);
    if(!result) {
        test_incorrect(title);
        return EXIT_FAILURE;
    }

    result &= Test_erase_root_children_with_one_value(printer) &&
              Test_erase_inside_root_children(printer) &&
              Test_internal_erase_replacing_from_left(printer) &&
              Test_internal_erase_replacing_from_right(printer);
    if(!result) {
        test_incorrect(title);
        return EXIT_FAILURE;
    }

    TestPrinting::test_correct(title);
    return 0;
}
