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
 * @file unique_set_test.cpp
 * @brief Tests for WTree as a unique-key set container.
 *
 * This is the deep set suite: interface coverage plus a deterministic
 * random workload.  The map (and later multi) suites only check the places
 * where those containers differ from the set, so set behavior that is
 * shared is exercised here exactly once.
 */

#include "../utils/declaration.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

#include <cstdlib>
#include <random>

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;

using BaseSet = WTreeLib::set<int>;
using BaseSetFixture = WTreeTestUtil::TContainerFixture<BaseSet>;

TestResults results;

// ============================================================================
// Tests
// ============================================================================

void test_predefined_insert() {
    BaseSet storage;

    vector<int> values{3000, 4000, 5000, 6000, 4100, 4200,
                       4300, 4400, 4250, 4260, 4270, 4280};

    for(const int &v : values) {
        auto res = storage.insert(v);
        if(!res.second) {
            results.fail("Predefined insert",
                         "Failed to insert " + to_string(v));
            return;
        }
    }

    if(storage.size() == values.size()) {
        results.pass("Predefined insert - size correct");
    } else {
        results.fail("Predefined insert - size mismatch",
                     "expected " + to_string(values.size()) + ", got " +
                         to_string(storage.size()));
    }

    if(!WTreeValidationUtils::validate_wtree(*storage.tree(),
                                             (long)values.size())) {
        results.fail("Predefined insert - tree validation");
    } else {
        results.pass("Predefined insert - tree valid");
    }
}

void test_search_true_positives() {
    BaseSet storage;
    vector<int> values{3000, 4000, 5000, 6000, 4100, 4200,
                       4300, 4400, 4250, 4260, 4270, 4280};

    for(const int &v : values)
        storage.insert(v);

    for(const int &v : values) {
        if(storage.find(v) == storage.end()) {
            results.fail("Search true positives",
                         "Key " + to_string(v) + " not found");
            return;
        }
        if(storage.count(v) != 1) {
            results.fail("Search true positives",
                         "count(" + to_string(v) + ") != 1");
            return;
        }
            }
    results.pass("Search true positives (find/count)");
}

void test_search_true_negatives() {
    BaseSet storage;
    vector<int> values{3000, 4000, 5000, 6000, 4100, 4200,
                       4300, 4400, 4250, 4260, 4270, 4280};

    for(const int &v : values)
        storage.insert(v);

    for(const int &v : {1, 2, 3, 10001, 10002, 10003, 4229, 4231, 5100}) {
        if(storage.find(v) != storage.end()) {
            results.fail("Search true negatives",
                         "False positive for key " + to_string(v));
            return;
        }

        if(storage.count(v) != 0) {
            results.fail("Search true negatives",
                         "count() nonzero for key " + to_string(v));
            return;
        }
    }
    results.pass("Search true negatives (find/count)");
}

void test_duplicate_insert_rejected() {
    BaseSet storage;
    storage.insert(42);
    auto res = storage.insert(42);

    if(!res.second && storage.size() == 1) {
        results.pass("Duplicate insert rejected");
    } else {
        results.fail("Duplicate insert rejected",
                     "Duplicate was inserted or size wrong");
    }
}

void test_random_insert_search() {
    BaseSet storage;

    mt19937 rng(42);
    vector<int> inserted;
    for(uint i = 0; i < 500; ++i) {
        int val;
        while(true) {
            val = (int)rng();
            auto res = storage.insert(val);
            if(!res.second)
                continue;
            if(!WTreeValidationUtils::validate_wtree(*storage.tree())) {
                results.fail("Random insert/search",
                             "Tree invalid inserting " + to_string(val));
                return;
            }
            break;
        }
        inserted.push_back(val);
    }

    for(const int &v : inserted) {
        if(storage.find(v) == storage.end()) {
            results.fail("Random insert/search",
                         "Key " + to_string(v) + " not found");
            return;
        }
    }
    results.pass("Random insert/search - 500 deterministic keys (mt19937)");
}

void test_erase_by_key() {
    BaseSet storage;
    for(int i = 0; i < 10; ++i)
        storage.insert(i * 100);

    size_t erased = (size_t)storage.erase(500);
    if(erased != 1 || storage.size() != 9) {
        results.fail("Erase by key");
        return;
    }

    if(storage.find(500) != storage.end()) {
        results.fail("Erase by key - key still found after erase");
        return;
    }

    // Erase non-existing key
    erased = (size_t)storage.erase(999);
    if(erased != 0) {
        results.fail("Erase non-existing key - expected 0");
        return;
    }

    results.pass("Erase by key");
}

void test_erase_by_iterator_and_range() {
    BaseSet storage;
    for(int i = 0; i < 20; ++i)
        storage.insert(i);

    auto it = storage.find(7);
    it = storage.erase(it);
    if(storage.find(7) != storage.end() || *it != 8) {
        results.fail("Erase by iterator - next element returned");
        return;
    }
    if(storage.size() != 19) {
        results.fail("Erase by iterator - size");
        return;
    }
auto first = storage.find(10);
    auto last = storage.find(15);
    storage.erase(first, last); // removes [10, 15): 10..14
    if(storage.find(10) != storage.end() || storage.find(14) != storage.end() ||
       storage.find(15) == storage.end()) {
        results.fail("Erase range - boundaries");
        return;
    }
    if(storage.size() != 14) { // 0..6, 8, 9, 15..19 (7 was erased above)
        results.fail("Erase range - size", "got " + to_string(storage.size()));
        return;
    }
    if(!WTreeValidationUtils::validate_wtree(*storage.tree(), 14)) {
        results.fail("Erase range - tree validation");
        return;
    }
    results.pass("Erase by iterator and range");
}

void test_clear_and_empty() {
    BaseSet storage;

    if(!storage.empty() || storage.size() != 0) {
        results.fail("Empty set - initial state");
        return;
    }

    for(int i = 0; i < 50; ++i)
        storage.insert(i);

    storage.clear();

    if(!storage.empty() || storage.size() != 0) {
        results.fail("Clear - set not empty after clear");
        return;
    }

    if(storage.find(0) == storage.end()) {
        results.pass("Clear and empty");
    } else {
        results.fail("Clear - key still found after clear");
    }
}

void test_iterator_traversal() {
    BaseSet storage;
    vector<int> input{50, 20, 80, 10, 90, 30};
    for(int v : input)
        storage.insert(v);

    vector<int> keys;
    for(auto it = storage.begin(); it != storage.end(); ++it)
        keys.push_back(it.key());

    bool sorted = true;
    for(size_t i = 1; i < keys.size(); ++i) {
        if(keys[i] <= keys[i - 1]) {
            sorted = false;
            break;
        }
    }

    if(sorted && keys.size() == input.size()) {
        results.pass("Iterator traversal - in-order");
    } else {
        results.fail("Iterator traversal - in-order");
    }
}

void test_reverse_iteration() {
    BaseSet storage;
    vector<int> input{50, 20, 80, 10, 90, 30};
    for(int v : input)
        storage.insert(v);

    vector<int> keys;
    for(auto it = storage.rbegin(); it != storage.rend(); ++it)
        keys.push_back(*it);

    bool descending = true;
    for(size_t i = 1; i < keys.size(); ++i) {
        if(keys[i] >= keys[i - 1]) {
            descending = false;
            break;
        }
    }

    if(descending && keys.size() == input.size()) {
        results.pass("Reverse iteration - in-order descending");
    } else {
        results.fail("Reverse iteration - in-order descending");
    }
}

void test_begin_end_on_empty() {
    BaseSet storage;

    if(storage.begin() == storage.end()) {
        results.pass("Empty set - begin() == end()");
    } else {
        results.fail("Empty set - begin() != end()");
    }
}

void test_bounds() {
    BaseSet storage;
    for(int i = 0; i < 20; i += 2)
        storage.insert(i); // {0,2,4,...,18}

    auto lb3 = storage.lower_bound(3);
    if(lb3 == storage.end() || *lb3 != 4) {
        results.fail("lower_bound - first key >= 3 should be 4");
        return;
    }
    if(*storage.lower_bound(4) != 4) {
        results.fail("lower_bound - exact key");
        return;
    }
    if(*storage.upper_bound(4) != 6) {
        results.fail("upper_bound - exact key");
        return;
    }
    if(*storage.upper_bound(18) != 0 && storage.upper_bound(18) != storage.end()) {
        results.fail("upper_bound - last key returns end()");
        return;
    }
    auto ers = storage.equal_range(4);
    int lb = *ers.first, ub = *ers.second;
    if(lb != 4 || ub != 6) {
        results.fail("equal_range - exact key");
        return;
    }
    auto lb1 = storage.lower_bound(1); // first key >= 1 is 2, not begin() (0)
    if(lb1 == storage.begin() || *lb1 != 2) {
        results.fail("lower_bound - before first key");
        return;
    }
    auto lbm1 = storage.lower_bound(-5); // below every key -> begin()
    if(lbm1 != storage.begin() || *lbm1 != 0) {
        results.fail("lower_bound - below all keys should be begin()");
        return;
    }
    if(storage.upper_bound(999) != storage.end()) {
        results.fail("upper_bound - beyond last key");
        return;
    }
    results.pass("Bounds (lower_bound/upper_bound/equal_range)");
}

void test_hint_insert() {
    BaseSet storage;
    storage.insert(10);
    storage.insert(20);

    // Hint before the future position: [10, 20] -> insert 15 before 20.
    auto it = storage.insert(storage.find(20), 15);
    if(*it != 15 || storage.size() != 3) {
        results.fail("Hint insert - insertion point misplaced");
        return;
    }

    // Hint at end: insert 30.
    it = storage.insert(storage.end(), 30);
    if(*it != 30) {
        results.fail("Hint insert - end hint");
        return;
    }

    if(!WTreeValidationUtils::validate_wtree(*storage.tree(), 4)) {
        results.fail("Hint insert - tree validation");
        return;
    }
    results.pass("Hint insert");
}

void test_swap() {
    BaseSet a, b;
    for(int i = 0; i < 5; ++i) {
        a.insert(i);
        b.insert(100 + i);
    }

    a.swap(b);

    if(a.find(100) != a.end() && a.find(0) == a.end() && b.find(0) != b.end() && b.find(100) == b.end() &&
       a.size() == 5 && b.size() == 5) {
        results.pass("Swap - contents exchanged");
    } else {
        results.fail("Swap - contents not exchanged");
        return;
    }

    swap(a, b); // free function
    if(a.find(0) != a.end() && b.find(100) != b.end()) {
        results.pass("Swap - free function");
    } else {
        results.fail("Swap - free function");
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    cout << "========================================\n";
    cout << "WTREE UNIQUE SET CONTAINER TESTS (deep set suite)\n";
    cout << "========================================\n";

    test_predefined_insert();
    test_search_true_positives();
    test_search_true_negatives();
    test_duplicate_insert_rejected();
    test_random_insert_search();
    test_erase_by_key();
    test_erase_by_iterator_and_range();
    test_clear_and_empty();
    test_iterator_traversal();
    test_reverse_iteration();
    test_begin_end_on_empty();
    test_bounds();
    test_hint_insert();
    test_swap();

    results.summary();
    return results.all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}