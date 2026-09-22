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

#include "erase_str_map_namespace.hpp"

#include <cstdlib>

namespace EraseStrMapNamespace {
TestResults results;
} // namespace EraseStrMapNamespace

int main() {
    using namespace EraseStrMapNamespace;

    erase_all_ascending_strings<512>(100); // CRASH today (dies on delete #11)
    erase_even_strings<512>(100);
    erase_all_hashed_strings<512>(200);
    erase_all_ascending_strings<256>(100);
    erase_all_ascending_strings<128>(100);

    results.summary();
    return results.all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}