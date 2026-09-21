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

#ifndef _WTREE_TEST_TESTING_H_
#define _WTREE_TEST_TESTING_H_

#include "../../include/wtree/map.hpp"
#include "../../include/wtree/set.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
// Logging — all test output goes through these helpers.
// Only two emojis: ✅ (pass) and ❌ (fail).
// ============================================================================

namespace TestPrinting {
using std::cin;
using std::cout;
using std::string;

void test_correct(string title) {
    printf("\n\n====================\n"
           "[✅] %s passed.\n",
           title.c_str());
}

void test_incorrect(string title) {
    printf("\n\n====================\n"
           "[❌] %s did not pass.\n",
           title.c_str());
}

void job_title(string title) {
    cout << "\n==========\n";
    printf("[[TEST]] %s\n", title.c_str());
}

void job_correct_result(string description = "") {
    printf("[✅] %s\n", description.c_str());
    cout << "==========\n";
}

void job_bad_result(string description = "") {
    printf("[❌] %s\n", description.c_str());
    cout << "==========\n";
}

// Neutral informational line (skips, diagnostics); not a pass/fail verdict.
void job_information(string description = "") {
    printf("[→] %s\n", description.c_str());
}

void print_dbg_correct_msg(string msg) {
#ifdef DBGVERBOSE
    TestPrinting::job_correct_result("[IN JOB] " + msg);
#endif
}

// ============================================================================
// TestResults — pass/fail counter for tests with many sub-cases.
// ============================================================================

struct TestResults {
    int passed = 0;
    int failed = 0;

    void pass(const string &test_name) {
        cout << "[✅] " << test_name << "\n";
        passed++;
    }

    void fail(const string &test_name, const string &reason = "") {
        cout << "[❌] " << test_name;
        if(!reason.empty())
            cout << " - " << reason;
        cout << "\n";
        failed++;
    }

    void summary() const {
        cout << "\n========================================\n";
        cout << "TOTAL: " << (passed + failed) << " tests\n";
        cout << "PASSED: " << passed << "\n";
        cout << "FAILED: " << failed << "\n";
        cout << "========================================\n";
    }

    bool all_passed() const { return failed == 0; }
};

// ============================================================================
// Evaluation helpers
// ============================================================================

template <typename T>
bool evaluate_test_result(string job_str, T result, bool is_valid,
                          string description = "") {
    std::ostringstream job_out;
    if(!is_valid) {
        if(!description.empty()) {
            job_out << description;
        } else {
            job_out << "result = " << result;
        }
        TestPrinting::job_bad_result(job_out.str());
        return false;
    }
    TestPrinting::print_dbg_correct_msg(job_str);
    return true;
}

template <typename Params, typename T>
bool validate_and_evaluate_test_result(string job_str, T result, bool is_valid,
                                       WTreeLib::WTree<Params> &wt,
                                       long expected_values = -1,
                                       string description = "") {
    std::ostringstream job_out;
    if(!is_valid ||
       !WTreeLib::WTreeValidationUtils::validate_wtree(wt, expected_values)) {
        if(!description.empty()) {
            job_out << description;
        } else {
            job_out << "result = " << result;
        }
        TestPrinting::job_bad_result(job_out.str());
        return false;
    }
    TestPrinting::print_dbg_correct_msg(job_str);
    return true;
}

// ============================================================================
// Helpers
// ============================================================================

template <typename WTreeType>
WTreeLib::WTreePrinter<WTreeType> build_test_printer() {
    WTreeLib::WTreePrinter<WTreeType> printer;
    printer.options.color_output = true;
    printer.options.compact_values = true;
    return printer;
}

template <typename T>
void print_vector(std::vector<T> &v, string title = "V: ") {
    cout << title << "\n";
    for(const T &val : v) {
        cout << val << " ";
    }
    cout << "\n";
}

template <typename T, typename Params>
std::vector<T> construct_values_vector(WTreeLib::WTreeNode<Params> *node) {
    T *begin = node->fields.values;
    return std::vector<T>(begin, begin + node->size());
}
} // namespace TestPrinting

#endif