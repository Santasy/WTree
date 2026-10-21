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

#ifndef _WTREE_TEST_INORDER_MAP_STRESS_H_
#define _WTREE_TEST_INORDER_MAP_STRESS_H_

/**
 * @brief In-order (sequential) insertion suite for the WTree map container.
 * @details The map stores the key/value pairs ("{index}", "val_{index}") and
 * fills it with an ascending key sequence, which is the worst case for a
 * node-based, ordered structure: every new key lands at the right end of the
 * tree and forces the balance policy (slide and/or split, see
 * @ref WTreeLib::WTreeBalanceOptions) to relocate keys upwards on every
 * rebalance.
 *
 * Two insertion sequences are exercised:
 *  - *index order*: the keys are inserted as generated (0, 1, 2, ... N-1);
 *  - *key order*: the same keys are inserted in the order the tree keeps
 *    them (lexicographic), which is the strictly sequential case.
 *
 * The final size N is an execution parameter (argv[1]); the balance policy is
 * a compilation knob (WTREE_STRESS_USE_SLIDE / WTREE_STRESS_USE_SPLIT, set
 * from the CMake cache variables WTREE_STRESS_SLIDE / WTREE_STRESS_SPLIT).
 */

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>

#include "../utils/declaration.hpp"
#include "../utils/testing.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

namespace InOrderMapStressNamespace {

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

// Target node size (bytes) used by the in-order suites. A map slot holds a
// (string, string) pair, hence the larger budget to keep a sane fanout.
constexpr int kStressNodeBytes = 256;

// Default final size, overridden by the execution parameter (argv[1]).
constexpr size_t kDefaultFinalSize = 100000;

// Every n-th insertion also validates the values bound along the iterator
// path (the structural path is validated on every insertion).
constexpr size_t kValuePathSampleStride = 1000;

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

// Value associated with a key: "val_{index}".
inline std::string value_for(const std::string &key) { return "val_" + key; }

// Allocation-free check of the "val_{index}" binding.
inline bool value_matches(const std::string &key, const std::string &value) {
    static const std::string prefix = "val_";
    return value.size() == prefix.size() + key.size() &&
           value.compare(0, prefix.size(), prefix) == 0 &&
           value.compare(prefix.size(), key.size(), key) == 0;
}

/**
 * @brief Fills a WTree map with the given key sequence and validates it.
 * @param insertion_order Keys to insert, in insertion order.
 * @param final_size      Expected number of entries.
 * @param label           Description of the insertion sequence.
 * @return True when every check passed.
 */
template <int NodeBytes = kStressNodeBytes>
bool run_inorder_case(const std::vector<std::string> &insertion_order,
                      size_t final_size, const std::string &label) {
    using MapType =
        WTreeLib::map<std::string, std::string, std::less<std::string>,
                      std::allocator<std::pair<const std::string, std::string>>,
                      NodeBytes, true, StressBalance>;
    using WTreeType = typename MapType::wtree_type;
    using ParamsType = typename WTreeType::params_type;

    std::ostringstream job_out;
    job_out << "In-order map insert [" << label << "] of " << final_size
            << " pairs (node=" << NodeBytes
            << "B, slide=" << ParamsType::use_slide
            << ", split=" << ParamsType::use_split << ").";
    job_title(job_out.str());

    MapType storage;
    size_t inserted_count = 0;

    if(insertion_order.empty()) {
        TestPrinting::job_information(
            "Nothing to insert (empty key sequence).");
        return true;
    }

    for(const std::string &key : insertion_order) {
        const std::string value = value_for(key);
        auto local_insert_result = storage.insert(
            std::pair<const std::string, std::string>(key, value));

        if(!local_insert_result.second) {
            job_bad_result("insert('" + key + "') did not insert the key");
            return false;
        }
        if((local_insert_result.first)->first != key ||
           (local_insert_result.first)->second != value) {
            job_bad_result("insert('" + key + "') landed on a different pair");
            return false;
        }

        // The structural path is checked on every insertion; the (costlier)
        // value binding of the whole path is checked on a sample, since the
        // final in-order walk validates every stored value anyway.
        if(!WTreeLib::WTreeValidationUtils::validate_iterator_path(
               local_insert_result.first)) {
            job_bad_result("iterator path broken at key '" + key + "'");
            return false;
        }
        if(inserted_count % kValuePathSampleStride == 0 &&
           !WTreeLib::WTreeValidationUtils::validate_map_iterator_path(
               local_insert_result.first,
               [](const std::string &k, const std::string &v) {
                   return value_matches(k, v);
               })) {
            job_bad_result("value binding broken at key '" + key + "'");
            return false;
        }
        ++inserted_count;
    }

    if(storage.size() != final_size) {
        job_bad_result("expected size " + std::to_string(final_size) + " got " +
                       std::to_string(storage.size()));
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
        "Tree shape: height=" + std::to_string(stats.height) +
        ", nodes=" + std::to_string(stats.nodes()) +
        ", leaves=" + std::to_string(stats.leaf_nodes) +
        ", internal=" + std::to_string(stats.internal_nodes));

    // The in-order walk must return the keys in ascending order, each one
    // bound to its "val_{index}" value.
    std::string previous_key;
    size_t visited = 0;
    for(auto it = storage.begin(); it != storage.end(); ++it, ++visited) {
        const std::string &key = (it)->first;
        if(visited > 0 && !(previous_key < key)) {
            job_bad_result("in-order walk broke at key '" + key + "'");
            return false;
        }
        if(!value_matches(key, (it)->second)) {
            job_bad_result("value mismatch for key '" + key + "'");
            return false;
        }
        previous_key = key;
    }
    if(visited != final_size) {
        job_bad_result("in-order walk visited " + std::to_string(visited) +
                       " pairs, expected " + std::to_string(final_size));
        return false;
    }

    // Spot-check the search path of the first, last and middle keys.
    for(const std::string &probe :
        {insertion_order.front(), insertion_order[insertion_order.size() / 2],
         insertion_order.back()}) {
        auto it = storage.find(probe);
        if(it == storage.end() || (it)->first != probe ||
           !value_matches(probe, (it)->second)) {
            job_bad_result("find('" + probe + "') failed");
            return false;
        }
    }

    job_correct_result("in-order insert [" + label + "] of " +
                       std::to_string(final_size) + " pairs OK");
    return true;
}

/**
 * @brief Inserts an ascending key sequence into a WTree map and validates it.
 * @tparam NodeBytes Target node size (bytes) of the tree.
 * @param final_size Number of pairs inserted (the execution parameter).
 * @return True when every check passed.
 */
template <int NodeBytes = kStressNodeBytes>
bool Test_inorder_insert(size_t final_size) {
    if(final_size == 0) {
        TestPrinting::job_information("Nothing to insert (final size is 0).");
        return true;
    }

    std::vector<std::string> index_order;
    index_order.reserve(final_size);
    for(size_t i = 0; i < final_size; ++i)
        index_order.push_back(std::to_string(i));

    // The same keys, in the order the tree keeps them (lexicographic).
    std::vector<std::string> key_order = index_order;
    std::sort(key_order.begin(), key_order.end());

    bool result = true;
    result &=
        run_inorder_case<NodeBytes>(index_order, final_size, "index order");
    result &= run_inorder_case<NodeBytes>(key_order, final_size, "key order");

    return result;
}

} // namespace InOrderMapStressNamespace

#endif
