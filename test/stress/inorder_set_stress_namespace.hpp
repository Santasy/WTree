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

#ifndef _WTREE_TEST_INORDER_SET_STRESS_H_
#define _WTREE_TEST_INORDER_SET_STRESS_H_

/**
 * @brief In-order (sequential) insertion suite for the WTree set container.
 * @details Inserts an ascending key sequence (0, 1, 2, ... N-1) into a
 * @ref WTreeLib::set, which is the worst case for a node-based, ordered
 * structure: every new key lands at the right end of the tree and forces the
 * balance policy (slide and/or split, see @ref WTreeLib::WTreeBalanceOptions)
 * to relocate keys upwards on every rebalance.
 *
 * The final size N is an execution parameter (argv[1]); the balance policy is
 * a compilation knob (WTREE_STRESS_USE_SLIDE / WTREE_STRESS_USE_SPLIT, set
 * from the CMake cache variables WTREE_STRESS_SLIDE / WTREE_STRESS_SPLIT).
 */

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <sys/types.h>

#include "../utils/declaration.hpp"
#include "../utils/testing.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

namespace InOrderSetStressNamespace {

using namespace std;
using WTreeLib::WTreeBalanceOptions;

// === Balance policy configuration ===
// The CMake knobs WTREE_STRESS_SLIDE / WTREE_STRESS_SPLIT map to these
// macros; when they are not defined the library-wide policy
// (WTREE_USE_SLIDE / WTREE_USE_SPLIT) is kept.
#ifndef WTREE_STRESS_USE_SLIDE
#define WTREE_STRESS_USE_SLIDE WTREE_USE_SLIDE
#endif

#ifndef WTREE_STRESS_USE_SPLIT
#define WTREE_STRESS_USE_SPLIT WTREE_USE_SPLIT
#endif

using StressBalance =
    WTreeBalanceOptions<WTREE_STRESS_USE_SLIDE, WTREE_STRESS_USE_SPLIT>;

// Target node size (bytes) used by the in-order suites.
constexpr int kStressNodeBytes = 512;

// Default final size, overridden by the execution parameter (argv[1]).
constexpr size_t kDefaultFinalSize = 100000;

using TestPrinting::job_bad_result, TestPrinting::job_correct_result,
    TestPrinting::job_title;

/**
 * @brief Reads the final size from the execution parameters.
 * @param argc        Argument count.
 * @param argv        Argument vector.
 * @param default_size Value returned when no (valid) parameter is given.
 * @return The requested final size.
 */
inline size_t parse_final_size(int argc, char **argv,
                               size_t default_size = kDefaultFinalSize) {
    if(argc < 2)
        return default_size;

    const char *arg = argv[1];
    char *end = nullptr;
    const unsigned long long parsed = strtoull(arg, &end, 10);

    if(end == arg || *end != '\0' || parsed == 0) {
        TestPrinting::job_information("Invalid final size '" +
                                      std::string(arg) + "'; using " +
                                      std::to_string(default_size) + ".");
        return default_size;
    }

    return (size_t)parsed;
}

/**
 * @brief Inserts an ascending key sequence into a WTree set and validates it.
 * @tparam Key       Key type of the set.
 * @tparam NodeBytes Target node size (bytes) of the tree.
 * @param final_size Number of keys inserted (the execution parameter).
 * @return True when every check passed.
 */
template <typename Key = uint, int NodeBytes = kStressNodeBytes>
bool Test_inorder_insert(size_t final_size) {
    using SetType = WTreeLib::set<Key, std::less<Key>, std::allocator<Key>,
                                  NodeBytes, StressBalance>;
    using WTreeType = typename SetType::wtree_type;
    using ParamsType = typename WTreeType::params_type;

    std::ostringstream job_out;
    job_out << "In-order set insert of " << final_size << " keys (key="
            << sizeof(Key) << "B, node=" << NodeBytes << "B, slide="
            << ParamsType::use_slide << ", split=" << ParamsType::use_split
            << ").";
    job_title(job_out.str());

    SetType storage;

    if(final_size == 0) {
        TestPrinting::job_information("Nothing to insert (final size is 0).");
        return true;
    }

    for(size_t i = 0; i < final_size; ++i) {
        const Key key = static_cast<Key>(i);
        auto local_insert_result = storage.insert(key);

        if(!local_insert_result.second) {
            job_bad_result("insert(" + std::to_string(i) +
                           ") did not insert the key");
            return false;
        }
        if(*(local_insert_result.first) != key) {
            job_bad_result("insert(" + std::to_string(i) +
                           ") landed on a different key");
            return false;
        }
        if(!WTreeLib::WTreeValidationUtils::validate_iterator_path(
               local_insert_result.first)) {
            job_bad_result("iterator path broken at key " + std::to_string(i));
            return false;
        }
    }

    if(storage.size() != final_size) {
        job_bad_result("expected size " + std::to_string(final_size) +
                       " got " + std::to_string(storage.size()));
        return false;
    }

    if(!WTreeLib::WTreeValidationUtils::validate_wtree(*(storage.tree()),
                                                         (long)final_size)) {
        job_bad_result("tree invariants violated");
        return false;
    }

    // The shape of the tree after the sequential insertions is reported (not
    // asserted): a balanced tree keeps the height around log_k(N).
    const auto stats = storage.tree()->collect_stats();
    TestPrinting::job_information(
        "Tree shape: height=" + std::to_string(stats.height) + ", nodes=" +
        std::to_string(stats.nodes()) + ", leaves=" +
        std::to_string(stats.leaf_nodes) + ", internal=" +
        std::to_string(stats.internal_nodes));

    // The in-order walk must return the ascending key sequence.
    size_t expected = 0;
    for(auto it = storage.begin(); it != storage.end(); ++it, ++expected) {
        if(*it != static_cast<Key>(expected)) {
            job_bad_result("in-order walk broke at position " +
                           std::to_string(expected));
            return false;
        }
    }
    if(expected != final_size) {
        job_bad_result("in-order walk visited " + std::to_string(expected) +
                       " keys, expected " + std::to_string(final_size));
        return false;
    }

    // Spot-check the search path of the first, last and middle keys.
    for(size_t probe : {size_t(0), final_size / 2, final_size - 1}) {
        auto it = storage.find(static_cast<Key>(probe));
        if(it == storage.end() || *it != static_cast<Key>(probe)) {
            job_bad_result("find(" + std::to_string(probe) + ") failed");
            return false;
        }
    }

    job_correct_result("in-order insert of " + std::to_string(final_size) +
                       " keys OK");
    return true;
}

} // namespace InOrderSetStressNamespace

#endif
