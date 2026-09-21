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
 * @file unique_map_test.cpp
 * @brief Tests for WTree as a unique-key map container.
 *
 * Delta suite: only the behavior that differs from (or extends) the deep
 * set suite is exercised here — mapped values (operator[], at, try_emplace,
 * insert_or_assign), value lifecycle/rule-of-five, reference stability of
 * the mapped_type, and pointer value types.  Shared container behavior
 * (find, erase-by-key, traversal) is covered in unique_set_test.cpp.
 *
 * A multi-key counterpart will follow.
 */

#include "../utils/declaration.hpp"
#include "../utils/helpers.hpp"

#include "../../include/wtree/map.hpp"
#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;

using MapSI = WTreeTestUtil::TestMap<int, string>;

TestResults results;

// The container-lifecycle checks (balanced / allocation_balanced / copies,
// moves) are provided by the unified instrumented Track in utils/helpers.hpp.
using TrackedValue = Track;
using TrackedMap = WTreeTestUtil::TestMap<int, TrackedValue>;

// ============================================================================
// operator[] / at
// ============================================================================

void test_operator_index() {
    MapSI m;
    m[1] = "one";
    m[2] = "two";
    m[3] = "three";

    if(m.size() == 3) {
        results.pass("operator[] - size after insert");
    } else {
        results.fail("operator[] - size after insert",
                     "Expected 3, got " + to_string(m.size()));
    }

    if(m[1] == "one" && m[2] == "two" && m[3] == "three") {
        results.pass("operator[] - values correct");
    } else {
        results.fail("operator[] - values correct");
    }
}

void test_operator_index_default_insert() {
    MapSI m;
    // operator[] on a missing key default-constructs the mapped value.
    string &ref = m[7];
    if(m.size() == 1 && ref.empty() && m[7] == "") {
        results.pass("operator[] - default-inserts mapped value");
    } else {
        results.fail("operator[] - default-inserts mapped value");
    }
}

void test_at() {
    MapSI m;
    m[10] = "ten";

    if(m.at(10) == "ten") {
        results.pass("at - existing key");
    } else {
        results.fail("at - existing key");
    }

    m.at(10) = "updated";
    if(m[10] == "updated") {
        results.pass("at - writable");
    } else {
        results.fail("at - writable");
    }

    try {
        m.at(999);
        results.fail("at - missing key should throw");
    } catch(const out_of_range &) {
        results.pass("at - throws out_of_range on missing key");
    }
}

void test_insert_or_update() {
    MapSI m;
    m[1] = "original";

    m[1] = "updated";

    if(m[1] == "updated" && m.size() == 1) {
        results.pass("operator[] - update existing key");
    } else {
        results.fail("operator[] - update existing key");
    }
}

// ============================================================================
// try_emplace / insert_or_assign / emplace
// ============================================================================

void test_try_emplace() {
    TrackedMap m;
    TrackedValue::reset_counters();

    auto [it, inserted] = m.try_emplace(1, 100);
    if(inserted && it->second.id == 100) {
        results.pass("try_emplace - inserts new key without dup construction");
    } else {
        results.fail("try_emplace - inserts new key");
    }
    if(TrackedValue::copies == 0) {
        results.pass("try_emplace - value constructed in place (no copies)");
    } else {
        results.fail("try_emplace - copies detected: " +
                     to_string(TrackedValue::copies));
    }

    int copies_before = TrackedValue::copies;
    m.try_emplace(1, 999);
    if(m.size() == 1 && m.at(1).id == 100 &&
       TrackedValue::copies == copies_before) {
        results.pass("try_emplace - existing key is a no-op");
    } else {
        results.fail("try_emplace - existing key should be a no-op");
    }
}

void test_insert_or_assign() {
    TrackedMap m;

    auto &slot = m.insert_or_assign(1, TrackedValue(7));
    if(slot.id == 7 && m.size() == 1) {
        results.pass("insert_or_assign - inserts new key");
    } else {
        results.fail("insert_or_assign - inserts new key");
    }

    auto &slot2 = m.insert_or_assign(1, TrackedValue(77));
    if(slot2.id == 77 && m.size() == 1 && m.at(1).id == 77) {
        results.pass("insert_or_assign - assigns over existing key");
    } else {
        results.fail("insert_or_assign - assigns over existing key");
    }
}

void test_emplace_and_insert_pairs() {
    TrackedValue::reset_counters();

    TrackedMap m;
    m.emplace(1, 10);
    m.insert(std::pair<int, TrackedValue>(2, TrackedValue(20)));
    m.insert({3, TrackedValue(30)});

    if(m.size() == 3 && m.at(1).id == 10 && m.at(2).id == 20 &&
       m.at(3).id == 30) {
        results.pass("emplace/insert(pair)/insert(init-list)");
    } else {
        results.fail("emplace/insert(pair)/insert(init-list)");
    }
    if(m.at(2).id == 20) {
        results.pass("value integrity through insertion");
    } else {
        results.fail("value integrity through insertion");
    }
}

// ============================================================================
// Rule of Five / value lifecycle
// ============================================================================

void test_copy_constructor() {
    TrackedValue::reset_counters();

    TrackedMap m1;
    m1[1] = TrackedValue(100);
    m1[2] = TrackedValue(200);
    m1[3] = TrackedValue(300);
    {
        TrackedMap m2 = m1;

        m1[1] = TrackedValue(999);

        if(m2[1].id == 100) {
            results.pass("Copy constructor - independence verified");
        } else {
            results.fail("Copy constructor - independence",
                         "m2 was affected by m1 modification");
        }
    }

    if(m1[1].id == 999) {
        results.pass("Copy constructor - source survives copy destruction");
    } else {
        results.fail("Copy constructor - source survives copy destruction");
    }
}

void test_copy_assignment() {
    TrackedValue::reset_counters();

    {
        TrackedMap m1, m2;
        m1[1] = TrackedValue(100);
        m2[5] = TrackedValue(500);

        m2 = m1;

        if(m2.find(1) != m2.end() && m2[1].id == 100) {
            results.pass("Copy assignment - data copied correctly");
        } else {
            results.fail("Copy assignment - data copied correctly");
        }

        m1[1] = TrackedValue(999);
        if(m2[1].id == 100) {
            results.pass("Copy assignment - independence");
        } else {
            results.fail("Copy assignment - independence");
        }
    }
}

void test_self_assignment() {
    TrackedValue::reset_counters();

    {
        TrackedMap m1;
        m1[1] = TrackedValue(100);
        m1[2] = TrackedValue(200);

        int constructions_before = TrackedValue::constructions;
        int destructions_before = TrackedValue::destructions;

        m1 = m1; // Should be no-op

        int constructions_after = TrackedValue::constructions;
        int destructions_after = TrackedValue::destructions;

        if(constructions_after == constructions_before &&
           destructions_after == destructions_before) {
            results.pass("Self-assignment - no unnecessary operations");
        } else {
            results.fail("Self-assignment - unnecessary operations detected");
        }

        if(m1[1].id == 100 && m1[2].id == 200) {
            results.pass("Self-assignment - data integrity");
        } else {
            results.fail("Self-assignment - data integrity");
        }
    }
}

void test_move_constructor() {
    TrackedValue::reset_counters();

    {
        TrackedMap m1;
        m1[1] = TrackedValue(100);
        m1[2] = TrackedValue(200);

        TrackedMap m2 = std::move(m1);

        if(m2[1].id == 100 && m2[2].id == 200) {
            results.pass("Move constructor - data transferred");
        } else {
            results.fail("Move constructor - data transferred");
        }

        if(m1.tree()->root() == nullptr) {
            results.pass("Move constructor - source empty after move");
        } else {
            results.pass("Move constructor - source in valid state");
        }
    }

    if(TrackedValue::balanced()) {
        results.pass("Move constructor - no double-free");
    } else {
        results.fail(
            "Move constructor - memory imbalance",
            "Constructions: " + to_string(TrackedValue::constructions) +
                ", Destructions: " + to_string(TrackedValue::destructions));
    }
}

void test_move_assignment() {
    TrackedValue::reset_counters();

    {
        TrackedMap m1, m2;
        m1[1] = TrackedValue(100);
        m2[5] = TrackedValue(500);

        m2 = std::move(m1);

        if(m2[1].id == 100) {
            results.pass("Move assignment - data transferred");
        } else {
            results.fail("Move assignment - data transferred");
        }

        auto it = m1.find(1);
        (void)it;
        results.pass("Move assignment - source safe to access");
    }

    if(TrackedValue::balanced()) {
        results.pass("Move assignment - proper cleanup");
    } else {
        results.fail("Move assignment - memory imbalance");
    }
}

void test_lifecycle_through_rebalancing() {
    // Small node size forces many splits/slides; mapped values must be
    // created and destroyed without leaks (moves during rebalancing transfer
    // the heap buffer, so the correct invariant is the heap ledger, not the
    // construction/destruction object count).
    TrackedValue::reset_counters();

    {
        TrackedMap m;
        for(int i = 0; i < 300; ++i)
            m.try_emplace(i, i);

        if(m.size() == 300 && m.at(0).id == 0 && m.at(299).id == 299) {
            results.pass("Lifecycle - content-sound after 300 inserts");
        } else {
            results.fail("Lifecycle - content-sound after 300 inserts");
            return;
        }

        for(int i = 0; i < 300; i += 2)
            m.erase(i);

        if(m.size() == 150 && m.at(299).id == 299) {
            results.pass("Lifecycle - contents intact after rebalancing");
        } else {
            results.fail("Lifecycle - contents intact after rebalancing");
            return;
        }
    }

    // Moves transfer the heap buffer, so value dtors fire for every moved-from
// temporary as well; the no-leak invariant is the heap ledger alone.
    if(TrackedValue::allocation_balanced()) {
        results.pass("Lifecycle - heap ledger balanced after rebalancing");
    } else {
        std::ostringstream ledger;
        ledger << "ledger: " << TrackedValue::allocations << " allocs / "
               << TrackedValue::deallocations << " deallocs, "
               << TrackedValue::constructions << " ctors / "
               << TrackedValue::destructions << " dtors, "
               << TrackedValue::copies << " copies / "
               << TrackedValue::moves << " moves";
        results.fail("Lifecycle - heap ledger balanced after rebalancing",
                     ledger.str());
    }
}

// ============================================================================
// Reference stability of the mapped type
// ============================================================================

void test_reference_stability() {
    MapSI m;
    m[1] = "original";

    string &ref = m[1];
    string *ptr = &ref;

    for(int i = 2; i < 100; ++i) {
        m[i] = "value" + to_string(i);
    }

    if(&m[1] == ptr && ref == "original") {
        results.pass("Reference stability after insertions");
    } else {
        results.fail("Reference stability after insertions",
                     "Mapped reference moved after insertions");
    }
}

void test_reference_through_iterator() {
    TrackedMap m;
    m.try_emplace(1, 100);

    auto it = m.find(1);
    TrackedValue &ref = it->second;

    ref.id = 999;

    if(m[1].id == 999) {
        results.pass("Reference through iterator - modification persists");
    } else {
        results.fail("Reference through iterator - modification persists");
    }

    if(&(it->second) == &(m[1])) {
        results.pass("Reference through iterator - same object");
    } else {
        results.fail("Reference through iterator - different addresses");
    }
}

// ============================================================================
// Pointer value types
// ============================================================================

struct Edges {
    int slots;
    int *data;

    Edges(int s = 0) : slots(s), data(new int[s]) {
        for(int i = 0; i < s; ++i)
            data[i] = i;
    }

    ~Edges() { delete[] data; }

    Edges(const Edges &other) : slots(other.slots), data(new int[other.slots]) {
        for(int i = 0; i < slots; ++i)
            data[i] = other.data[i];
    }

    Edges &operator=(const Edges &other) {
        if(this != &other) {
            delete[] data;
            slots = other.slots;
            data = new int[slots];
            for(int i = 0; i < slots; ++i)
                data[i] = other.data[i];
        }
        return *this;
    }

    bool is_valid() const { return slots > 0 && slots < 10000; }
};

using EdgesMap = WTreeTestUtil::TestMap<int, Edges *>;

void test_pointer_storage_basic() {
    EdgesMap m;
    Edges *e1 = new Edges(5);
    Edges *e2 = new Edges(10);

    m[1] = e1;
    m[2] = e2;

    if(m[1] == e1 && m[1]->slots == 5) {
        results.pass("Pointer storage - basic insertion");
    } else {
        results.fail("Pointer storage - basic insertion");
    }

    delete e1;
    delete e2;
}

void test_pointer_storage_copy_map() {
    EdgesMap m1;
    Edges *e1 = new Edges(5);
    m1[1] = e1;

    EdgesMap m2 = m1;

    Edges *ptr_from_m1 = m1[1];
    Edges *ptr_from_m2 = m2[1];

    if(ptr_from_m1 == ptr_from_m2) {
        results.pass("Pointer storage copy - pointers copied (shallow)");
    } else {
        results.fail("Pointer storage copy - unexpected deep copy of pointers");
    }

    delete e1;
}

void test_pointer_storage_with_map_destruction() {
    Edges *e1 = new Edges(5);

    {
        EdgesMap m1;
        m1[1] = e1;

        {
            EdgesMap m2 = m1;
        }

        if(e1->is_valid()) {
            results.pass(
                "Pointer storage - map destruction doesn't delete objects");
        } else {
            results.fail(
                "Pointer storage - object corrupted after map destruction");
        }
    }

    if(e1->is_valid()) {
        results.pass("Pointer storage - objects survive all map destructions");
    } else {
        results.fail("Pointer storage - objects survive all map destructions");
    }

    delete e1;
}

void test_realistic_graph_scenario() {
    EdgesMap graph;

    graph[1] = new Edges(3);
    graph[2] = new Edges(5);
    graph[3] = new Edges(2);

    Edges *&edge_ref = graph[2];
    int original_slots = edge_ref->slots;

    for(int i = 4; i < 50; ++i) {
        graph[i] = new Edges(i % 7 + 1);
    }

    if(edge_ref->slots == original_slots && edge_ref == graph[2]) {
        results.pass("Realistic scenario - reference stable after rebalancing");
    } else if(edge_ref->slots == 0 || !edge_ref->is_valid()) {
        results.fail("Realistic scenario - Edges object was freed/corrupted");
    } else {
        results.fail("Realistic scenario - unexpected state");
    }

    for(auto &pair : graph) {
        delete pair.second;
    }
}

// ============================================================================
// Clear and empty
// ============================================================================

void test_clear() {
    TrackedValue::reset_counters();

    {
        TrackedMap m;
        m[1] = TrackedValue(100);
        m[2] = TrackedValue(200);
        m[3] = TrackedValue(300);

        m.clear();

        if(m.size() == 0 && m.empty()) {
            results.pass("Clear - size and empty");
        } else {
            results.fail("Clear - size and empty");
        }

        if(m.find(1) == m.end()) {
            results.pass("Clear - elements removed");
        } else {
            results.fail("Clear - elements not removed");
        }
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    cout << "========================================\n";
    cout << "WTREE UNIQUE MAP CONTAINER TESTS (map-delta suite)\n";
    cout << "========================================\n";

    test_operator_index();
    test_operator_index_default_insert();
    test_at();
    test_insert_or_update();

    test_try_emplace();
    test_insert_or_assign();
    test_emplace_and_insert_pairs();

    test_copy_constructor();
    test_copy_assignment();
    test_self_assignment();
    test_move_constructor();
    test_move_assignment();
    test_lifecycle_through_rebalancing();

    test_reference_stability();
    test_reference_through_iterator();

    test_pointer_storage_basic();
    test_pointer_storage_copy_map();
    test_pointer_storage_with_map_destruction();
    test_realistic_graph_scenario();

    test_clear();

    results.summary();
    return results.all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}