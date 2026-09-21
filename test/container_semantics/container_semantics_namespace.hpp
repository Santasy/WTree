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

#ifndef _WTREE_CONTAINER_SEMANTICS_NAMESPACE_H_
#define _WTREE_CONTAINER_SEMANTICS_NAMESPACE_H_

#include "../utils/testing.hpp"
#include "../utils/helpers.hpp"
#include "../utils/declaration.hpp"

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <vector>

namespace ContainerSemanticsNamespace {

using WTreeTestUtil::SetSelector;
using WTreeTestUtil::MapSelector;
using WTreeTestUtil::WTreeContainerTag;
using WTreeTestUtil::StdContainerTag;

// =============================================================================
// Test: Copy Constructor with Queue values
// =============================================================================

template <typename ContainerTag> bool Test_map_copy_constructor() {
    using MapType = typename MapSelector<int, Queue, ContainerTag>::type;
    const char *container_name = MapSelector<int, Queue, ContainerTag>::name;

    TestPrinting::job_title(std::string("Map Copy Constructor [") +
                            container_name + "]");

    MapType original;
    for(int i = 0; i < 100; ++i) {
        Queue q;
        const int base = i * 10;
        q.push(base);
        q.push(base + 1);
        q.push(base + 2);
        original.insert({i, std::move(q)});
        assert(original.size() == static_cast<size_t>(i + 1));
    }

    Queue::reset_counters();

    MapType copied(original);

    if(!maps_are_equal(original, copied, "After copy construction")) {
        TestPrinting::job_bad_result("Maps not equal after copy construction");
        return false;
    }

    if(original.size() != 100) {
        TestPrinting::job_bad_result("Original map size changed");
        return false;
    }

    if(Queue::copy_count == 0) {
        TestPrinting::job_bad_result(
            "No copies were made during copy construction");
        return false;
    }

    std::ostringstream msg;
    msg << "Copies: " << Queue::copy_count << ", Moves: " << Queue::move_count;
    TestPrinting::job_correct_result(msg.str());
    return true;
}

// =============================================================================
// Test: Copy Assignment with Queue values
// =============================================================================

template <typename ContainerTag> bool Test_map_copy_assignment() {
    using MapType = typename MapSelector<int, Queue, ContainerTag>::type;
    const char *container_name = MapSelector<int, Queue, ContainerTag>::name;

    TestPrinting::job_title(std::string("Map Copy Assignment [") +
                            container_name + "]");

    MapType original;
    for(int i = 0; i < 100; ++i) {
        Queue q;
        q.push(i * 10);
        q.push(i * 10 + 1);
        original.insert({i, std::move(q)});
    }

    MapType target;
    for(int i = 200; i < 250; ++i) {
        Queue q;
        q.push(i);
        target.insert({i, std::move(q)});
    }

    Queue::reset_counters();

    target = original;

    if(!maps_are_equal(original, target, "After copy assignment")) {
        TestPrinting::job_bad_result("Maps not equal after copy assignment");
        return false;
    }

    if(original.size() != 100) {
        TestPrinting::job_bad_result("Original map size changed");
        return false;
    }

    std::ostringstream msg;
    msg << "Copies: " << Queue::copy_count << ", Moves: " << Queue::move_count;
    TestPrinting::job_correct_result(msg.str());
    return true;
}

// =============================================================================
// Test: Move Constructor with Queue values
// =============================================================================

template <typename ContainerTag> bool Test_map_move_constructor() {
    using MapType = typename MapSelector<int, Queue, ContainerTag>::type;
    const char *container_name = MapSelector<int, Queue, ContainerTag>::name;

    TestPrinting::job_title(std::string("Map Move Constructor [") +
                            container_name + "]");

    MapType original;
    for(int i = 0; i < 100; ++i) {
        Queue q;
        q.push(i * 10);
        q.push(i * 10 + 1);
        original.insert({i, std::move(q)});
    }

    MapType reference(original);

    Queue::reset_counters();

    MapType moved(std::move(original));

    if(!maps_are_equal(reference, moved, "After move construction")) {
        TestPrinting::job_bad_result("Moved map doesn't match original");
        return false;
    }

    if(original.size() != 0) {
        TestPrinting::job_bad_result(
            "Original map not empty after move: size = " +
            std::to_string(original.size()));
        return false;
    }

    std::ostringstream msg;
    msg << "Copies: " << Queue::copy_count << ", Moves: " << Queue::move_count;
    TestPrinting::job_correct_result(msg.str());
    return true;
}

// =============================================================================
// Test: Move Assignment with Queue values
// =============================================================================

template <typename ContainerTag> bool Test_map_move_assignment() {
    using MapType = typename MapSelector<int, Queue, ContainerTag>::type;
    const char *container_name = MapSelector<int, Queue, ContainerTag>::name;

    TestPrinting::job_title(std::string("Map Move Assignment [") +
                            container_name + "]");

    MapType original;
    for(int i = 0; i < 100; ++i) {
        Queue q;
        q.push(i * 10);
        q.push(i * 10 + 1);
        original.insert({i, std::move(q)});
    }

    MapType reference(original);

    MapType target;
    for(int i = 200; i < 250; ++i) {
        Queue q;
        q.push(i);
        target.insert({i, std::move(q)});
    }

    Queue::reset_counters();

    target = std::move(original);

    if(!maps_are_equal(reference, target, "After move assignment")) {
        TestPrinting::job_bad_result(
            "Target map doesn't match original after move");
        return false;
    }

    std::ostringstream msg;
    msg << "Copies: " << Queue::copy_count << ", Moves: " << Queue::move_count;
    TestPrinting::job_correct_result(msg.str());
    return true;
}

// =============================================================================
// Test: Swap with Queue values
// =============================================================================

template <typename ContainerTag> bool Test_map_swap() {
    using MapType = typename MapSelector<int, Queue, ContainerTag>::type;
    const char *container_name = MapSelector<int, Queue, ContainerTag>::name;

    TestPrinting::job_title(std::string("Map Swap [") + container_name + "]");

    MapType map1, map2;

    for(int i = 0; i < 50; ++i) {
        Queue q;
        q.push(i * 10);
        map1.insert({i, std::move(q)});
    }

    for(int i = 100; i < 200; ++i) {
        Queue q;
        q.push(i * 20);
        map2.insert({i, std::move(q)});
    }

    MapType ref1(map1), ref2(map2);

    Queue::reset_counters();

    map1.swap(map2);

    if(!maps_are_equal(ref1, map2, "map2 after swap")) {
        TestPrinting::job_bad_result(
            "map2 doesn't match original map1 after swap");
        return false;
    }

    if(!maps_are_equal(ref2, map1, "map1 after swap")) {
        TestPrinting::job_bad_result(
            "map1 doesn't match original map2 after swap");
        return false;
    }

    std::ostringstream msg;
    msg << "Copies: " << Queue::copy_count << ", Moves: " << Queue::move_count;
    TestPrinting::job_correct_result(msg.str());
    return true;
}

// =============================================================================
// Test: Self-assignment
// =============================================================================

template <typename ContainerTag> bool Test_map_self_assignment() {
    using MapType = typename MapSelector<int, Queue, ContainerTag>::type;
    const char *container_name = MapSelector<int, Queue, ContainerTag>::name;

    TestPrinting::job_title(std::string("Map Self-Assignment [") +
                            container_name + "]");

    MapType original;
    for(int i = 0; i < 50; ++i) {
        Queue q;
        q.push(i * 10);
        original.insert({i, std::move(q)});
    }

    MapType reference(original);

    Queue::reset_counters();

    // Self copy assignment
    original = original;

    if(!maps_are_equal(reference, original, "After self copy-assignment")) {
        TestPrinting::job_bad_result(
            "Map corrupted after self copy-assignment");
        return false;
    }

    // Self move assignment — undefined for std::map, WTree handles it.
    if constexpr(std::is_same_v<ContainerTag, WTreeContainerTag>) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
        original = std::move(original);
#pragma GCC diagnostic pop

        if(!maps_are_equal(reference, original, "After self move-assignment")) {
            TestPrinting::job_bad_result(
                "Map corrupted after self move-assignment");
            return false;
        }
    }

    std::ostringstream msg;
    msg << "Self-assignment handled correctly. Copies: " << Queue::copy_count
        << ", Moves: " << Queue::move_count;
    TestPrinting::job_correct_result(msg.str());
    return true;
}

// =============================================================================
// Test: Empty container operations
// =============================================================================

template <typename ContainerTag> bool Test_empty_container_operations() {
    using MapType = typename MapSelector<int, Queue, ContainerTag>::type;
    const char *container_name = MapSelector<int, Queue, ContainerTag>::name;

    TestPrinting::job_title(std::string("Empty Container Operations [") +
                            container_name + "]");

    MapType empty1, empty2;

    MapType copied(empty1);
    if(copied.size() != 0) {
        TestPrinting::job_bad_result("Copy of empty map is not empty");
        return false;
    }

    MapType moved(std::move(empty1));
    if(moved.size() != 0) {
        TestPrinting::job_bad_result("Move of empty map is not empty");
        return false;
    }

    empty1.swap(empty2);
    if(empty1.size() != 0 || empty2.size() != 0) {
        TestPrinting::job_bad_result("Swap of empty maps failed");
        return false;
    }

    MapType non_empty;
    non_empty.insert({1, Queue({1, 2, 3})});
    non_empty = empty2;
    if(non_empty.size() != 0) {
        TestPrinting::job_bad_result("Assigning empty to non-empty failed");
        return false;
    }

    TestPrinting::job_correct_result("All empty container operations passed");
    return true;
}

// =============================================================================
// Test: Large container copy (stress test)
// =============================================================================

template <typename ContainerTag> bool Test_large_container_copy() {
    using MapType = typename MapSelector<int, Queue, ContainerTag>::type;
    const char *container_name = MapSelector<int, Queue, ContainerTag>::name;

    TestPrinting::job_title(std::string("Large Container Copy [") +
                            container_name + "]");

    MapType original;
    const int NUM_ELEMENTS = 1000;

    for(int i = 0; i < NUM_ELEMENTS; ++i) {
        Queue q;
        for(int j = 0; j < 10; ++j) {
            q.push(i * 10 + j);
        }
        original.insert({i, std::move(q)});
    }

    Queue::reset_counters();

    MapType copied(original);

    if(!maps_are_equal(original, copied, "Large container copy")) {
        TestPrinting::job_bad_result("Large container copy failed");
        return false;
    }

    std::ostringstream msg;
    msg << "Copied " << NUM_ELEMENTS
        << " elements. Copies: " << Queue::copy_count
        << ", Moves: " << Queue::move_count;
    TestPrinting::job_correct_result(msg.str());
    return true;
}

// =============================================================================
// Test: Set operations (simpler value type)
// =============================================================================

template <typename ContainerTag> bool Test_set_copy_move() {
    using SetType = typename SetSelector<int, ContainerTag>::type;
    const char *container_name = SetSelector<int, ContainerTag>::name;

    TestPrinting::job_title(std::string("Set Copy/Move [") + container_name +
                            "]");

    SetType original;
    for(int i = 0; i < 500; ++i) {
        original.insert(i * 2); // Even numbers
    }

    // Copy
    SetType copied(original);
    if(!sets_are_equal(original, copied, "Set copy")) {
        TestPrinting::job_bad_result("Set copy failed");
        return false;
    }

    // Move
    SetType reference(original);
    SetType moved(std::move(original));

    if(!sets_are_equal(reference, moved, "Set move")) {
        TestPrinting::job_bad_result("Set move failed");
        return false;
    }

    if(original.size() != 0) {
        TestPrinting::job_bad_result("Original set not empty after move");
        return false;
    }

    TestPrinting::job_correct_result("Set copy/move passed");
    return true;
}

// =============================================================================
// Contract tests: hardcoded-expectation checks, run under both tags.
// The StdTag run calibrates the expectations themselves — if a hardcoded
// successor/size/value is wrong, the std::set/std::map run fails first,
// proving a test bug rather than a library bug.
// =============================================================================

// Contract: bounds on the sorted key set {12, 27, 53, 60, 84}.
template <typename ContainerTag> bool Test_contract_bounds() {
    using SetType = typename SetSelector<int, ContainerTag>::type;
    const char *container_name = SetSelector<int, ContainerTag>::name;

    TestPrinting::job_title(std::string("Contract Bounds [") + container_name +
                            "]");

    SetType s({53, 27, 84, 12, 60});

    auto it = s.lower_bound(30); // between 27 and 53
    if(it == s.end() || *it != 53) {
        TestPrinting::job_bad_result("lower_bound(30) != 53");
        return false;
    }
    it = s.lower_bound(53); // exact key
    if(it == s.end() || *it != 53) {
        TestPrinting::job_bad_result("lower_bound(53) != 53");
        return false;
    }
    it = s.upper_bound(53); // first key strictly after 53
    if(it == s.end() || *it != 60) {
        TestPrinting::job_bad_result("upper_bound(53) != 60");
        return false;
    }
    it = s.upper_bound(60);
    if(it == s.end() || *it != 84) {
        TestPrinting::job_bad_result("upper_bound(60) != 84");
        return false;
    }
    it = s.upper_bound(84); // past the last key
    if(it != s.end()) {
        TestPrinting::job_bad_result("upper_bound(max) != end()");
        return false;
    }
    it = s.lower_bound(1); // before the first key
    if(it != s.begin() || *it != 12) {
        TestPrinting::job_bad_result("lower_bound(1) != begin()");
        return false;
    }
    auto er = s.equal_range(27);
    if(er.first == s.end() || *er.first != 27 || er.second == s.end() ||
       *er.second != 53) {
        TestPrinting::job_bad_result("equal_range(27) != {27, 53}");
        return false;
    }

    TestPrinting::job_correct_result("bounds contract OK");
    return true;
}

// Contract: hint-insert (ordered end() hints), duplicates, iteration order.
template <typename ContainerTag> bool Test_contract_hint_insert() {
    using SetType = typename SetSelector<int, ContainerTag>::type;
    const char *container_name = SetSelector<int, ContainerTag>::name;

    TestPrinting::job_title(std::string("Contract Hint Insert [") +
                            container_name + "]");

    SetType s;
    for(int i = 0; i < 10; ++i) {
        auto it = s.insert(s.end(), i);
        if(it == s.end() || *it != i) {
            TestPrinting::job_bad_result("hint insert(" + std::to_string(i) +
                                         ") returned wrong element");
            return false;
        }
    }
    if(s.size() != 10) {
        TestPrinting::job_bad_result("size after hint inserts != 10");
        return false;
    }

    int expected = 0;
    for(auto it = s.begin(); it != s.end(); ++it, ++expected) {
        if(*it != expected) {
            TestPrinting::job_bad_result("iteration order broken; expected " +
                                         std::to_string(expected));
            return false;
        }
    }

    // Duplicate with a bogus hint: no insertion, iterator points at 5.
    auto it2 = s.insert(s.begin(), 5);
    if(it2 == s.end() || *it2 != 5 || s.size() != 10) {
        TestPrinting::job_bad_result("duplicate hint insert wrong");
        return false;
    }

    TestPrinting::job_correct_result("hint-insert contract OK");
    return true;
}

// Contract: initializer-list and range construction.
template <typename ContainerTag> bool Test_contract_range_and_init_list() {
    using SetType = typename SetSelector<int, ContainerTag>::type;
    const char *container_name = SetSelector<int, ContainerTag>::name;

    TestPrinting::job_title(std::string("Contract Init-list / Range [") +
                            container_name + "]");

    SetType s{30, 0, 60, 10, 0}; // duplicates collapse
    if(s.size() != 4 || *s.begin() != 0 || *s.rbegin() != 60) {
        TestPrinting::job_bad_result("init-list construction wrong");
        return false;
    }

    std::vector<int> v{7, 3, 9, 1};
    SetType rng(v.begin(), v.end());
    if(rng.size() != 4 || *rng.begin() != 1 || *rng.rbegin() != 9) {
        TestPrinting::job_bad_result("range construction wrong");
        return false;
    }
    if(rng.find(3) == rng.end() || rng.find(8) != rng.end()) {
        TestPrinting::job_bad_result("range construction contents wrong");
        return false;
    }

    rng.insert(v.begin(), v.end()); // re-insert range: no-ops
    if(rng.size() != 4) {
        TestPrinting::job_bad_result("range re-insert changed size");
        return false;
    }

    TestPrinting::job_correct_result("init-list / range contract OK");
    return true;
}

// Contract: member swap exchanges contents.
template <typename ContainerTag> bool Test_contract_swap() {
    using SetType = typename SetSelector<int, ContainerTag>::type;
    const char *container_name = SetSelector<int, ContainerTag>::name;

    TestPrinting::job_title(std::string("Contract Swap [") + container_name +
                            "]");

    SetType a({1, 3, 5});
    SetType b({2, 4});

    a.swap(b);

    if(a.size() != 2 || *a.begin() != 2 || *a.rbegin() != 4) {
        TestPrinting::job_bad_result("a does not hold b's contents after swap");
        return false;
    }
    if(b.size() != 3 || *b.begin() != 1 || *b.rbegin() != 5) {
        TestPrinting::job_bad_result("b does not hold a's contents after swap");
        return false;
    }

    TestPrinting::job_correct_result("swap contract OK");
    return true;
}

// Contract: map insert_or_assign never duplicates the key and stores the new
// value.  NOTE: WTreeLib's insert_or_assign returns mapped_type& while
// std::map returns pair<iterator,bool> — an intentional divergence; the
// return shape is normalized below so the behavioral contract (key count and
// stored value) stays dual-tag checkable.
template <typename ContainerTag> bool Test_contract_insert_or_assign() {
    using MapType = typename MapSelector<int, int, ContainerTag>::type;
    const char *container_name = MapSelector<int, int, ContainerTag>::name;

    TestPrinting::job_title(std::string("Contract insert_or_assign [") +
                            container_name + "]");

    auto stored_value = [](MapType &m, int key, int val) -> int {
        auto res = m.insert_or_assign(key, val);
        if constexpr(std::is_same_v<ContainerTag, StdContainerTag>)
            return res.first->second;
        else
            return res;
    };

    MapType m;
    int v1 = stored_value(m, 5, 100);
    if(v1 != 100 || m.size() != 1) {
        TestPrinting::job_bad_result("insert_or_assign on new key wrong");
        return false;
    }

    int v2 = stored_value(m, 5, 200);
    if(v2 != 200 || m.size() != 1 || m.find(5)->second != 200) {
        TestPrinting::job_bad_result("insert_or_assign on existing key wrong");
        return false;
    }

    TestPrinting::job_correct_result("insert_or_assign contract OK");
    return true;
}

// =============================================================================
// Balance Options battery (WTree-only).  The tasks themselves are policy
// instantiations of the same int set; only the WTree side has policies.
// =============================================================================

namespace BalanceOptionsNamespace {

// Byte size for the set instantiations. Small enough that a one-thousand
// element battery keeps the tree several levels deep, so the configured
// balance operations are exercised repeatedly.
static constexpr int kTestNodeBytes = 256;

constexpr int kDefaultCount = 1000;

// The four balance-policy combinations under test.
using SlideOnly = WTreeLib::WTreeBalanceOptions<true, false>;
using SplitOnly = WTreeLib::WTreeBalanceOptions<false, true>;
using NoBalance = WTreeLib::WTreeBalanceOptions<false, false>;
using FullBalance = WTreeLib::WTreeBalanceOptions<true, true>;

// Maps a balance policy to the tested container instantiation.
// All use int keys on the WTreeLib::set container.
template <typename BalanceOptions> struct BalanceSetSelector;

template <> struct BalanceSetSelector<SlideOnly> {
    using type = WTreeLib::set<int, std::less<int>, std::allocator<int>,
                               kTestNodeBytes, SlideOnly>;
    static constexpr const char *name = "slide=true,  split=false";
};

template <> struct BalanceSetSelector<SplitOnly> {
    using type = WTreeLib::set<int, std::less<int>, std::allocator<int>,
                               kTestNodeBytes, SplitOnly>;
    static constexpr const char *name = "slide=false, split=true";
};

template <> struct BalanceSetSelector<NoBalance> {
    using type = WTreeLib::set<int, std::less<int>, std::allocator<int>,
                               kTestNodeBytes, NoBalance>;
    static constexpr const char *name = "slide=false, split=false";
};

template <> struct BalanceSetSelector<FullBalance> {
    using type = WTreeLib::set<int, std::less<int>, std::allocator<int>,
                               kTestNodeBytes, FullBalance>;
    static constexpr const char *name = "slide=true,  split=true";
};

template <typename SetType> bool set_is_valid(SetType &storage, long expected) {
    return WTreeLib::WTreeValidationUtils::validate_wtree(*(storage.tree()),
                                                          expected);
}

// Test: policy knobs are wired through to the instantiation.
template <typename Policy> bool Test_int_policy_knobs() {
    using SetType = typename BalanceSetSelector<Policy>::type;
    using WTreeType = typename SetType::wtree_type;
    using WSetParams = typename WTreeType::params_type;

    std::ostringstream msg;
    msg << "[knobs] slide=" << WSetParams::use_slide
        << " split=" << WSetParams::use_split
        << " numeric_threshold=" << WSetParams::binary_search_threshold_numeric
        << " complex_threshold=" << WSetParams::binary_search_threshold_complex
        << " kBinarySearchThreshold=" << WSetParams::kBinarySearchThreshold
        << " kTargetNodeBytes=" << WSetParams::kTargetNodeBytes
        << " kTargetK=" << WSetParams::kTargetK;

    TestPrinting::job_correct_result(msg.str());
    return true;
}

// Test: ascending insert.
template <typename Policy> bool Test_insert_ascending() {
    using SetType = typename BalanceSetSelector<Policy>::type;
    const char *policy_name = BalanceSetSelector<Policy>::name;

    TestPrinting::job_title(std::string("Insert ascending [") + policy_name +
                            "]");

    SetType storage;
    for(int i = 0; i < kDefaultCount; ++i) {
        auto [it, inserted] = storage.insert(i);
        if(!inserted) {
            TestPrinting::job_bad_result("insert(" + std::to_string(i) +
                                         ") returned false");
            return false;
        }
        if(*it != i) {
            TestPrinting::job_bad_result("insert(" + std::to_string(i) +
                                         ") landed on " + std::to_string(*it));
            return false;
        }
    }

    if(storage.size() != static_cast<size_t>(kDefaultCount)) {
        TestPrinting::job_bad_result("expected size " +
                                     std::to_string(kDefaultCount) + " got " +
                                     std::to_string(storage.size()));
        return false;
    }

    if(!set_is_valid(storage, kDefaultCount)) {
        TestPrinting::job_bad_result("tree invariants violated");
        return false;
    }

    TestPrinting::job_correct_result("ascending insert OK");
    return true;
}

// Test: shuffled insert.
template <typename Policy> bool Test_insert_shuffled() {
    using SetType = typename BalanceSetSelector<Policy>::type;
    const char *policy_name = BalanceSetSelector<Policy>::name;

    TestPrinting::job_title(std::string("Insert shuffled [") + policy_name +
                            "]");

    std::vector<int> keys(kDefaultCount);
    std::iota(keys.begin(), keys.end(), 0);
    std::mt19937 rng(20260908);
    std::shuffle(keys.begin(), keys.end(), rng);

    SetType storage;
    for(int i : keys) {
        auto [it, inserted] = storage.insert(i);
        if(!inserted) {
            TestPrinting::job_bad_result("insert(" + std::to_string(i) +
                                         ") returned false");
            return false;
        }
    }

    if(storage.size() != static_cast<size_t>(kDefaultCount)) {
        TestPrinting::job_bad_result("expected size " +
                                     std::to_string(kDefaultCount) + " got " +
                                     std::to_string(storage.size()));
        return false;
    }

    if(!set_is_valid(storage, kDefaultCount)) {
        TestPrinting::job_bad_result("tree invariants violated");
        return false;
    }

    TestPrinting::job_correct_result("shuffled insert OK");
    return true;
}

template <typename Policy> bool Run_battery_for_policy() {
    TestPrinting::job_title(std::string("Balance policy [") +
                            BalanceSetSelector<Policy>::name + "]");

    bool all_passed = true;
    all_passed &= Test_int_policy_knobs<Policy>();
    all_passed &= Test_insert_ascending<Policy>();
    all_passed &= Test_insert_shuffled<Policy>();

    return all_passed;
}

bool Run_all_balance_options_tests() {
    TestPrinting::job_title("Balance Options matrix (int set)");

    bool all_passed = true;
    all_passed &= Run_battery_for_policy<SlideOnly>();
    all_passed &= Run_battery_for_policy<SplitOnly>();
    all_passed &= Run_battery_for_policy<NoBalance>();
    all_passed &= Run_battery_for_policy<FullBalance>();

    return all_passed;
}

} // namespace BalanceOptionsNamespace

// =============================================================================
// Run all tests for a given container tag
// =============================================================================

template <typename ContainerTag> bool Run_all_semantics_tests() {
    bool all_passed = true;

    all_passed &= Test_map_copy_constructor<ContainerTag>();
    all_passed &= Test_map_copy_assignment<ContainerTag>();
    all_passed &= Test_map_move_constructor<ContainerTag>();
    all_passed &= Test_map_move_assignment<ContainerTag>();
    all_passed &= Test_map_swap<ContainerTag>();
    all_passed &= Test_map_self_assignment<ContainerTag>();
    all_passed &= Test_empty_container_operations<ContainerTag>();
    all_passed &= Test_large_container_copy<ContainerTag>();
    all_passed &= Test_set_copy_move<ContainerTag>();

    all_passed &= Test_contract_bounds<ContainerTag>();
    all_passed &= Test_contract_hint_insert<ContainerTag>();
    all_passed &= Test_contract_range_and_init_list<ContainerTag>();
    all_passed &= Test_contract_swap<ContainerTag>();
    all_passed &= Test_contract_insert_or_assign<ContainerTag>();

    return all_passed;
}

} // namespace ContainerSemanticsNamespace

#endif
