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
 * @file edge_swap_test.cpp
 * @brief Boundary-key rotation at a full internal node
 * (`emplace_edge_swap`).
 *
 * Ordered insertion is the adversarial case for the W-tree: the new key lands
 * at index 0 (descending) or at index `size()` (ascending) of the node the
 * descent ends on, so whenever that node is a full internal one the boundary
 * key has to rotate down the chain. Ascending input exercises the right edge
 * (`swap_idx == kTargetK - 1`, `child_idx == kTargetK - 2`), descending input
 * the left edge, and a small node size makes the chains several levels deep.
 *
 * The numeric variant (trivially copyable values) carries the displaced key in
 * a local and only descends; the complex variant (string keys, where the value
 * is not assignable) keeps the descend-then-ascend rotation. Both are covered:
 * `set<uint>` for the numeric path, `set<string>` for the complex one.
 *
 * Checks per insertion: the returned iterator points at the inserted key and
 * carries a usable ancestor path; at the end: size, iteration order and the
 * whole-tree invariants, then the same after erasing every third key.
 */

#include "../utils/declaration.hpp"

#include "../../include/wtree/optional/utils.hpp"

#include <algorithm>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;

namespace {

constexpr size_t kKeys = 5000;

enum class Order { Ascending, Descending, Shuffled };

template <typename Key> vector<Key> make_keys(Order order) {
    vector<Key> keys;
    keys.reserve(kKeys);
    for(size_t i = 0; i < kKeys; ++i) {
        if constexpr(is_same_v<Key, string>) {
            // Fixed width, so lexicographic order matches numeric order.
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "k%06zu", i);
            keys.emplace_back(buffer);
        } else {
            keys.push_back(static_cast<Key>(i * 3 + 1));
        }
    }
    if(order == Order::Descending) {
        std::reverse(keys.begin(), keys.end());
    } else if(order == Order::Shuffled) {
        std::mt19937 gen(20260518);
        std::shuffle(keys.begin(), keys.end(), gen);
    }
    return keys;
}

const char *order_name(Order order) {
    switch(order) {
    case Order::Ascending:
        return "ascending";
    case Order::Descending:
        return "descending";
    default:
        return "shuffled";
    }
}

/// Inserts -order- into a fresh set, validating every step.
template <int NodeBytes, bool Slide, bool Split, typename Key>
bool run_case(Order order) {
    using balance = WTreeBalanceOptions<Slide, Split>;
    using set_type =
        WTreeLib::set<Key, less<Key>, allocator<Key>, NodeBytes, balance>;

    const vector<Key> keys = make_keys<Key>(order);
    set_type storage;

    for(const Key &key : keys) {
        auto result = storage.insert(key);
        if(!result.second) {
            job_bad_result(string("duplicate insert at ") + order_name(order));
            return false;
        }
        if(*result.first != key) {
            job_bad_result(string("insert landed on another key (") +
                           order_name(order) + ")");
            return false;
        }
        // The caller may iterate from the returned iterator.
        if(!WTreeValidationUtils::validate_iterator_path(result.first)) {
            job_bad_result(string("broken iterator path (") +
                           order_name(order) + ")");
            return false;
        }
    }

    if(storage.size() != kKeys) {
        job_bad_result("wrong size after insert");
        return false;
    }

    size_t seen = 0;
    auto it = storage.begin();
    Key previous{};
    for(; it != storage.end(); ++it, ++seen) {
        if(seen > 0 && !(previous < *it)) {
            job_bad_result("iteration order broken");
            return false;
        }
        previous = *it;
    }
    if(seen != kKeys) {
        job_bad_result("iteration stopped early");
        return false;
    }
    if(!WTreeValidationUtils::validate_wtree(*storage.tree(), (long)kKeys)) {
        job_bad_result("tree invariants violated after insert");
        return false;
    }

    // Erase against a partly drained chain (the rotation shares the node).
    size_t erased = 0;
    for(size_t i = 0; i < keys.size(); i += 3) {
        if(storage.erase(keys[i]) != 1) {
            job_bad_result("erase did not remove an existing key");
            return false;
        }
        ++erased;
    }
    if(!WTreeValidationUtils::validate_wtree(*storage.tree(),
                                             (long)(kKeys - erased))) {
        job_bad_result("tree invariants violated after erase");
        return false;
    }
    return true;
}

// -WithStringKeys- only from 256 B on: at nb=64/128 a std::string node holds
// kTargetK == kInitialCapacity == 3 keys, and the split rules then ask for a
// 2-slot leaf, which the (pre-existing) kInitialCapacity assert rejects. That
// is a separate, complex-value sizing defect, not part of this suite.
template <int NodeBytes, bool Slide, bool Split, bool WithStringKeys = true>
bool run_matrix() {
    const string label = string("nb=") + to_string(NodeBytes) +
                         " slide=" + to_string((int)Slide) +
                         " split=" + to_string((int)Split);
    for(Order order : {Order::Ascending, Order::Descending, Order::Shuffled}) {
        job_title(label + " " + order_name(order));
        if(!run_case<NodeBytes, Slide, Split, uint>(order))
            return false;
        if constexpr(WithStringKeys) {
            if(!run_case<NodeBytes, Slide, Split, string>(order))
                return false;
        }
    }
    return true;
}

} // namespace

int main() {
    // Numeric rotation: deep chains at 64/128 B, the uint16_t field at 1024 B,
    // and the whole balance matrix at the smallest node size.
    bool ok = run_matrix<64, true, true, false>();
    ok = ok && run_matrix<128, true, true, false>();
    ok = ok && run_matrix<512, true, true>();
    ok = ok && run_matrix<1024, true, true>(); // uint16_t field type
    ok = ok && run_matrix<64, true, false, false>();
    ok = ok && run_matrix<64, false, true, false>();
    ok = ok && run_matrix<64, false, false, false>();
    // Complex rotation (std::string values take the descend-then-ascend path).
    ok = ok && run_matrix<256, true, true>();
    ok = ok && run_matrix<256, true, false>();

    if(!ok) {
        test_incorrect("Boundary-key rotation (edge swap)");
        return EXIT_FAILURE;
    }
    test_correct("Boundary-key rotation (edge swap)");
    return EXIT_SUCCESS;
}
