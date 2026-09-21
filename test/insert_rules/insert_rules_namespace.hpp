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

#ifndef _WTREE_TEST_INSERT_RULES_H_
#define _WTREE_TEST_INSERT_RULES_H_

#include <cassert>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sys/types.h>

#include "../utils/declaration.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"
#include "../../include/wtree/set.hpp"

namespace InsertRulesNamespace {

using namespace std;
using WUtils = WTreeLib::WTreeGenerationUtils;
using namespace TestPrinting;

using BaseSet = WTreeTestUtil::TestSet<int>;
using Fixture = WTreeTestUtil::TContainerFixture<BaseSet>;
using WTreeType = typename Fixture::wtree_type;
using NodeType = typename Fixture::node_type;
using KeyType = int;
using Field = typename NodeType::field_type;
using Printer = WTreeLib::WTreePrinter<WTreeType>;

bool Test_Validate_Assumptions() {
    using assumptions = WTreeLib::WTreeRulesAssumptions;
    job_title("Validate assumptions.");
    bool assumptions_are_correct = assumptions::slide_over_split &&
                                   assumptions::slide_to_right_first &&
                                   assumptions::split_to_right_first;
    if(!assumptions_are_correct) {
        job_bad_result(
            "Assumptions for testing insertion rules are not valid.");
        return false;
    }
    job_correct_result("Assumptions validated successfully.");
    return true;
}

bool Test_LateralSwap(Printer printer = Printer()) {
    Fixture fx;
    auto &storage = fx.storage;
    WTreeType *wt = fx.wt();
    NodeType *node = fx.root();
    bool result = true;
    string job_str;

    if(!Test_Validate_Assumptions())
        return false;

    const ulong k_size = NodeType::kTargetK;
    KeyType left = 0, right = numeric_limits<KeyType>::max();

    job_str = "Fill root node.";
    job_title(job_str);
    result = WUtils::fill_node(*wt, &node, k_size, left, right);
    assert(result);
    result &= validate_and_evaluate_test_result(job_str, result, result, *wt);
    if(!result) {
        job_bad_result(job_str);
        return result;
    }
    printer.print_tree(wt->root());

    job_str = "Replace rightmost key.";
    job_title(job_str);
    KeyType old_max = node->fields.values[k_size - 1];
    KeyType new_max = old_max + 1;
    auto insert_result = storage.insert(new_max);

    // Assert that child node is created correctly.
    bool lateral_result =
        insert_result.second &&
        (node->fields.values[k_size - 1] == new_max &&
         node->fields.pointers[k_size - 2]->fields.values[0] == old_max);
    result &= validate_and_evaluate_test_result(job_str, lateral_result,
                                                lateral_result, *wt);
    printer.print_tree(wt->root());
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Replace leftmost key.";
    job_title(job_str);
    KeyType old_min = node->fields.values[0];
    KeyType new_min = old_min - 1;
    insert_result = storage.insert(new_min);

    // Assert that child node is created correctly.
    lateral_result = insert_result.second &&
                     (node->fields.values[0] == new_min &&
                      node->fields.pointers[0]->fields.values[0] == old_min);
    result &= validate_and_evaluate_test_result(job_str, lateral_result,
                                                lateral_result, *wt);
    printer.print_tree(wt->root());
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    return true;
}

bool Test_Slides(Printer printer = Printer()) {
    Fixture fx;
    auto &storage = fx.storage;
    WTreeType *wt = fx.wt();
    NodeType *node = fx.root();
    bool result = true;
    string job_str;

    if(!Test_Validate_Assumptions())
        return false;

    const ulong k_size = NodeType::kTargetK;
    ulong total_size = 0;
    KeyType left = 0, right = numeric_limits<KeyType>::max();

    job_str = "Fill root node.";
    job_title(job_str);
    result = WUtils::fill_node(*wt, &node, k_size, left, right);
    assert(result);
    total_size += k_size;
    result &= validate_and_evaluate_test_result(job_str, result, result, *wt);
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Setup slide test - create child node.";
    job_title(job_str);
    ulong mid = k_size / 2;
    node->fields.pointers[mid] = wt->manager()->new_leaf_node(k_size);
    result = WUtils::fill_child(*wt, wt->root(), mid, k_size);
    assert(result);
    total_size += k_size;
    result &= validate_and_evaluate_test_result(job_str, result, result, *wt);
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Slide to right test.";
    job_title(job_str);
    auto insert_result =
        storage.insert(node->fields.values[mid + 1] + 1); // creates right
    assert(insert_result.second);
    insert_result =
        storage.insert(node->fields.values[mid + 1] - 1); // inserts in current
    total_size += 2;
    result &= validate_and_evaluate_test_result(job_str, insert_result.second,
                                                insert_result.second, *wt);
    printer.print_tree(wt->root());
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Slide to left test setup.";
    job_title(job_str);
    total_size -= wt->root()->child(mid)->size();
    result = WUtils::fill_child(*wt, wt->root(), mid, k_size); // fill mid
    assert(result);
    total_size += k_size;

    total_size -= wt->root()->child(mid + 1)->size();
    result = WUtils::fill_child(*wt, wt->root(), mid + 1,
                                k_size); // fill right
    total_size += k_size;
    assert(result);
    result &= validate_and_evaluate_test_result(job_str, result, result, *wt);
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Slide to left test.";
    job_title(job_str);
    insert_result =
        storage.insert(node->fields.values[mid - 1] + 1); // create left
    insert_result =
        storage.insert(node->fields.pointers[mid]->fields.values[mid] + 1);
    total_size += 2;
    result &= validate_and_evaluate_test_result(job_str, insert_result.second,
                                                insert_result.second, *wt);
    printer.print_tree(wt->root());
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    return true;
}

bool Test_Split(Printer printer = Printer()) {
    Fixture fx;
    auto &storage = fx.storage;
    WTreeType *wt = fx.wt();
    NodeType *node = fx.root();
    bool result = true;
    string job_str;

    if(!Test_Validate_Assumptions())
        return false;

    const ulong k_size = NodeType::kTargetK;
    ulong total_size = 0;
    KeyType left = 0, right = numeric_limits<KeyType>::max();

    job_str = "Fill root node.";
    job_title(job_str);
    result = WUtils::fill_node(*wt, &node, k_size, left, right);
    assert(result);
    total_size += k_size;
    result &= validate_and_evaluate_test_result(job_str, result, result, *wt);
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Setup split test - create child node.";
    job_title(job_str);
    ulong mid = k_size / 2;
    node->set_child(mid, wt->manager()->new_leaf_node(k_size));
    result = WUtils::fill_child(*wt, wt->root(), mid, k_size);
    assert(result);
    total_size += k_size;
    result &= validate_and_evaluate_test_result(job_str, result, result, *wt);
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Split to right test.";
    job_title(job_str);
    auto insert_result = storage.insert(node->child(mid)->value(0) + 1);
    assert(insert_result.second);
    total_size += 1;
    result &= validate_and_evaluate_test_result(job_str, insert_result.second,
                                                insert_result.second, *wt);
    printer.print_tree(wt->root());
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Split to left test setup.";
    job_title(job_str);
    total_size -= wt->root()->child(mid)->size();
    result = WUtils::fill_child(*wt, wt->root(), mid, k_size); // fill mid
    assert(result);
    total_size += k_size;

    total_size -= wt->root()->child(mid + 1)->size();
    result = WUtils::fill_child(*wt, wt->root(), mid + 1,
                                k_size); // fill right
    total_size += k_size;
    assert(result);
    result &= validate_and_evaluate_test_result(job_str, result, result, *wt);
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_str = "Split to left test.";
    job_title(job_str);
    insert_result = storage.insert(node->child(mid)->value(0) + 1);
    assert(insert_result.second);
    total_size += 1;
    result &= validate_and_evaluate_test_result(job_str, insert_result.second,
                                                insert_result.second, *wt);
    printer.print_tree(wt->root());
    if(!result) {
        job_bad_result(job_str);
        return result;
    }

    job_correct_result("All Split Rules tested successfully.");
    return true;
}

bool Test_all_Rules(Printer printer = Printer()) {
    bool result = true;
    string job_str;

    job_str = "Testing all insertion rules.";
    job_title(job_str);

    result = Test_LateralSwap(printer);
    if(!result) {
        job_bad_result("Lateral swap test failed.");
        return false;
    }

    result = Test_Slides(printer);
    if(!result) {
        job_bad_result("Slides test failed.");
        return false;
    }

    if constexpr(WTreeType::params_type::use_split) {
        result = Test_Split(printer);
        if(!result) {
            job_bad_result("Split test failed.");
            return false;
        }
    }

    job_correct_result("All insertion rules tested successfully.");
    return true;
}

} // namespace InsertRulesNamespace

#endif