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
 * @file map_stress_test.cpp
 * @brief Stress tests for WTree map at scale.
 *
 * Validates correctness under:
 * - Node splits / balance operations
 * - Large tree sizes
 * - Mixed insert/erase/find operations (compared against std::set)
 * - Insert/delete cycles
 */

#include "../utils/testing.hpp"

#include "../../include/wtree/map.hpp"

#include <algorithm>
#include <cstdlib>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;

// ============================================================================
// Configuration
// ============================================================================
constexpr int DEFAULT_NODE_SIZE = 128;

constexpr int SMALL_SIZE = 1000;
constexpr int MEDIUM_SIZE = 10000;
constexpr int LARGE_SIZE = 50000;

TestResults results;

// ============================================================================
// SECTION 1: Sequential Insert — Verify After Node Splits
// ============================================================================

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_sequential_insert(int count) {
    string test_name = "Sequential insert (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    for(int i = 0; i < count; ++i) {
        storage[i] = i * 10;
    }

    if(storage.size() != static_cast<size_t>(count)) {
        results.fail(test_name, "Size mismatch: expected " + to_string(count) +
                                    ", got " + to_string(storage.size()));
        return;
    }

    int first_missing = -1;
    for(int i = 0; i < count; ++i) {
        auto it = storage.find(i);
        if(it == storage.end() || it->second != i * 10) {
            first_missing = i;
            break;
        }
    }

    if(first_missing == -1) {
        results.pass(test_name);
    } else {
        results.fail(test_name,
                     "Missing or incorrect key: " + to_string(first_missing));
    }
}

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_reverse_insert(int count) {
    string test_name = "Reverse insert (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    for(int i = count - 1; i >= 0; --i) {
        storage[i] = i * 10;
    }

    if(storage.size() != static_cast<size_t>(count)) {
        results.fail(test_name, "Size mismatch");
        return;
    }

    int first_missing = -1;
    for(int i = 0; i < count; ++i) {
        auto it = storage.find(i);
        if(it == storage.end() || it->second != i * 10) {
            first_missing = i;
            break;
        }
    }

    if(first_missing == -1) {
        results.pass(test_name);
    } else {
        results.fail(test_name, "Missing key: " + to_string(first_missing));
    }
}

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_random_insert(int count, unsigned seed = 42) {
    string test_name = "Random insert (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    vector<int> keys(count);

    mt19937 gen(seed);
    for(int i = 0; i < count; ++i) {
        keys[i] = i;
    }
    shuffle(keys.begin(), keys.end(), gen);

    for(int key : keys) {
        storage[key] = key * 10;
    }

    if(storage.size() != static_cast<size_t>(count)) {
        results.fail(test_name, "Size mismatch: expected " + to_string(count) +
                                    ", got " + to_string(storage.size()));
        return;
    }

    int first_missing = -1;
    for(int i = 0; i < count; ++i) {
        auto it = storage.find(i);
        if(it == storage.end() || it->second != i * 10) {
            first_missing = i;
            break;
        }
    }

    if(first_missing == -1) {
        results.pass(test_name);
    } else {
        results.fail(test_name, "Missing key: " + to_string(first_missing));
    }
}

// ============================================================================
// SECTION 2: Erase Operations — Verify Tree Integrity
// ============================================================================

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_erase_even_keys(int count) {
    string test_name = "Erase even keys (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    for(int i = 0; i < count; ++i) {
        storage[i] = i * 10;
    }

    for(int i = 0; i < count; i += 2) {
        storage.erase(i);
    }

    size_t expected_size = count / 2;
    if(storage.size() != expected_size) {
        results.fail(test_name, "Size mismatch after erase: expected " +
                                    to_string(expected_size) + ", got " +
                                    to_string(storage.size()));
        return;
    }

    int first_missing = -1;
    for(int i = 1; i < count; i += 2) {
        auto it = storage.find(i);
        if(it == storage.end() || it->second != i * 10) {
            first_missing = i;
            break;
        }
    }

    int first_found = -1;
    for(int i = 0; i < count; i += 2) {
        if(storage.find(i) != storage.end()) {
            first_found = i;
            break;
        }
    }

    if(first_missing == -1 && first_found == -1) {
        results.pass(test_name);
    } else if(first_missing != -1) {
        results.fail(test_name, "Odd key missing: " + to_string(first_missing));
    } else {
        results.fail(test_name,
                     "Even key not erased: " + to_string(first_found));
    }
}

template <int NodeSize = DEFAULT_NODE_SIZE> void test_erase_all(int count) {
    string test_name = "Erase all (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    for(int i = 0; i < count; ++i) {
        storage[i] = i * 10;
    }

    for(int i = 0; i < count; ++i) {
        storage.erase(i);
    }

    if(storage.empty() && storage.size() == 0) {
        results.pass(test_name);
    } else {
        results.fail(test_name, "Map not empty after erasing all, size=" +
                                    to_string(storage.size()));
    }
}

// ============================================================================
// SECTION 3: Iterator Traversal — Verify Ordering
// ============================================================================

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_iterator_ordering(int count) {
    string test_name = "Iterator ordering (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    vector<int> keys(count);
    for(int i = 0; i < count; ++i) {
        keys[i] = i;
    }
    mt19937 gen(42);
    shuffle(keys.begin(), keys.end(), gen);

    for(int key : keys) {
        storage[key] = key;
    }

    int prev = -1;
    int element_count = 0;
    int first_unsorted = -1;

    for(auto it = storage.begin(); it != storage.end(); ++it) {
        if(it->first <= prev) {
            first_unsorted = it->first;
            break;
        }
        prev = it->first;
        element_count++;
    }

    if(first_unsorted == -1 && element_count == count) {
        results.pass(test_name);
    } else if(first_unsorted != -1) {
        results.fail(test_name,
                     "Out of order at key: " + to_string(first_unsorted));
    } else {
        results.fail(test_name,
                     "Element count mismatch: " + to_string(element_count));
    }
}

// ============================================================================
// SECTION 4: Reference Stability
// ============================================================================

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_reference_stability(int count) {
    string test_name = "Reference stability (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    storage[0] = 12345;
    int *ptr = &(storage[0]);

    for(int i = 1; i < count; ++i) {
        storage[i] = i;
    }

    if(ptr == &(storage[0]) && *ptr == 12345) {
        results.pass(test_name);
    } else if(*ptr == 12345) {
        results.fail(test_name, "Address changed (node reallocation)");
    } else {
        results.fail(test_name, "Value corrupted");
    }
}

// ============================================================================
// SECTION 5: Copy and Move Operations
// ============================================================================

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_copy_large_map(int count) {
    string test_name = "Copy large map (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    for(int i = 0; i < count; ++i) {
        storage[i] = i * 10;
    }

    WMap storage2 = storage;
    if(storage2.size() != storage.size()) {
        results.fail(test_name, "Size mismatch after copy");
        return;
    }

    storage[0] = 99999;

    if(storage2[0] == 0) {
        results.pass(test_name);
    } else {
        results.fail(test_name, "Copy not independent from original");
    }
}

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_move_large_map(int count) {
    string test_name = "Move large map (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    for(int i = 0; i < count; ++i) {
        storage[i] = i * 10;
    }

    WMap storage2 = std::move(storage);

    if(storage2.size() != static_cast<size_t>(count)) {
        results.fail(test_name, "Size mismatch after move: got " +
                                    to_string(storage2.size()));
        return;
    }

    int first_missing = -1;
    for(int i = 0; i < count; ++i) {
        auto it = storage2.find(i);
        if(it == storage2.end() || it->second != i * 10) {
            first_missing = i;
            break;
        }
    }

    if(first_missing == -1) {
        results.pass(test_name);
    } else {
        results.fail(test_name,
                     "Data not correctly transferred, missing key: " +
                         to_string(first_missing));
    }
}

// ============================================================================
// SECTION 6: Mixed Operations — Compare Against std::set
// ============================================================================

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_mixed_operations(int count, unsigned seed = 42) {
    string test_name =
        "Mixed operations vs std::set (n=" + to_string(count) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;
    std::set<int> reference;

    mt19937 gen(seed);
    uniform_int_distribution<> op_dist(0, 2);
    uniform_int_distribution<> key_dist(0, count * 2);

    for(int i = 0; i < count; ++i) {
        int op = op_dist(gen);
        int key = key_dist(gen);

        switch(op) {
        case 0: // Insert
            storage[key] = key;
            reference.insert(key);
            break;
        case 1: // Erase
            storage.erase(key);
            reference.erase(key);
            break;
        case 2: // Find
        {
            auto it_m = storage.find(key);
            auto it_r = reference.find(key);
            bool m_found = (it_m != storage.end());
            bool r_found = (it_r != reference.end());
            if(m_found != r_found) {
                results.fail(test_name, "Find mismatch for key " +
                                            to_string(key) + " at iteration " +
                                            to_string(i));
                return;
            }
        } break;
        }
    }

    if(storage.size() != reference.size()) {
        results.fail(test_name,
                     "Final size mismatch: WTree=" + to_string(storage.size()) +
                         ", STL=" + to_string(reference.size()));
        return;
    }

    for(int key : reference) {
        if(storage.find(key) == storage.end()) {
            results.fail(test_name, "State mismatch: key " + to_string(key) +
                                        " in STL but not in WTree");
            return;
        }
    }

    results.pass(test_name);
}

// ============================================================================
// SECTION 7: Insert/Delete Cycles
// ============================================================================

template <int NodeSize = DEFAULT_NODE_SIZE>
void test_insert_delete_cycles(int count, int cycles) {
    string test_name = "Insert/delete cycles (n=" + to_string(count) +
                       ", cycles=" + to_string(cycles) + ")";
    using WMap = WTreeLib::map<int, int, std::less<int>, std::allocator<int>, NodeSize>;
    WMap storage;

    for(int c = 0; c < cycles; ++c) {
        for(int i = 0; i < count; ++i) {
            storage[i] = i;
        }

        for(int i = 0; i < count; ++i) {
            if(storage.find(i) == storage.end()) {
                results.fail(test_name, "Missing key " + to_string(i) +
                                            " in cycle " + to_string(c) +
                                            " insert phase");
                return;
            }
        }

        for(int i = 0; i < count; ++i) {
            storage.erase(i);
        }

        if(!storage.empty()) {
            results.fail(test_name,
                         "Map not empty after cycle " + to_string(c));
            return;
        }
    }

    results.pass(test_name);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    cout << "========================================\n";
    cout << "WTREE MAP STRESS TESTS\n";
    cout << "========================================\n";

    // Sequential insert at various sizes
    test_sequential_insert<DEFAULT_NODE_SIZE>(SMALL_SIZE);
    test_sequential_insert<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);
    test_sequential_insert<DEFAULT_NODE_SIZE>(LARGE_SIZE);

    // Reverse insert
    test_reverse_insert<DEFAULT_NODE_SIZE>(SMALL_SIZE);
    test_reverse_insert<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);

    // Random insert
    test_random_insert<DEFAULT_NODE_SIZE>(SMALL_SIZE);
    test_random_insert<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);

    // Erase tests
    test_erase_even_keys<DEFAULT_NODE_SIZE>(SMALL_SIZE);
    test_erase_even_keys<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);
    test_erase_all<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);

    // Iterator ordering
    test_iterator_ordering<DEFAULT_NODE_SIZE>(SMALL_SIZE);
    test_iterator_ordering<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);

    // Reference stability
    test_reference_stability<DEFAULT_NODE_SIZE>(SMALL_SIZE);
    test_reference_stability<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);

    // Copy/Move tests
    test_copy_large_map<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);
    test_move_large_map<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);

    // Mixed operations vs std::set
    test_mixed_operations<DEFAULT_NODE_SIZE>(SMALL_SIZE);
    test_mixed_operations<DEFAULT_NODE_SIZE>(MEDIUM_SIZE);

    // Insert/delete cycles
    test_insert_delete_cycles<DEFAULT_NODE_SIZE>(SMALL_SIZE, 5);
    test_insert_delete_cycles<DEFAULT_NODE_SIZE>(MEDIUM_SIZE, 3);

    // Summary
    results.summary();
    return results.all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}
