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

#include "manager_test.hpp"

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;
using namespace ManagerTest;

// ============================================================================
// Main
// ============================================================================

int main() {
    TestPrinting::job_title("Manager-layer relocations (move_values_to_node / "
                            "move_values_to_blank_node)");

    if(Mgr::kTargetK < 12) {
        TestPrinting::job_bad_result("kTargetK too small for the scratch"
                                     " layout");
        return EXIT_FAILURE;
    }

    TestResults results;
    bool ok = true;

    if(Track::g_alive != 0)
        results.fail("start alive counter", "nonzero");
    else
        results.pass("start alive counter");

    ok &= group1_left_shifts(results);
    ok &= group2_right_shifts(results);
    ok &= group3_blank_moves(results);
    ok &= group4_live_dest_contract(results);

    if(Track::g_alive != 0) {
        results.fail("final alive counter",
                     "nonzero " + std::to_string(Track::g_alive));
        ok = false;
    } else {
        results.pass("final alive counter");
    }

    results.summary();
    return ok && results.all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}