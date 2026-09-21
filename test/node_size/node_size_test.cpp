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
 * @file node_size_test.cpp
 * @brief Node-size edge-case instantiation suite.
 *
 * Coverts the interplay between the requested node byte size
 * (WTREE_TARGET_NODE_BYTES analog), the derived k (kTargetK) and the index
 * field type (uint8_t/uint16_t):
 *
 *  - kTargetK is clamped to a minimum of 3.
 *  - field_type must be able to represent kTargetK (flips at k >= 255).
 *  - when kTargetK > 3, a full node must fit in the requested byte budget
 *    (base_fields + kTargetK * sizeof(value_type) <= kTargetNodeBytes);
 *    at the clamped k == 3 the budget may be exceeded.
 *
 * Both compile-time invariants (static_assert) and a runtime insert/erase
 * battery across many target sizes and key types are exercised here.
 */

#include "../utils/declaration.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

#include <cstdint>
#include <limits>
#include <string>

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;

// ============================================================================
// Compile-time invariants for a set instantiation
// ============================================================================

template <typename SetType> constexpr bool node_size_invariants() {
    using P = typename SetType::wtree_type::params_type;
    constexpr uint k = P::kTargetK;
    using F = typename P::field_type;

    if(k < 3)
        return false;
    if((uint)std::numeric_limits<F>::max() < k)
        return false;

    // field_type is uint8_t exactly while kTargetK < 255, uint16_t from 255.
    constexpr bool small_field = (k < 255);
    if(small_field != std::is_same_v<F, std::uint8_t>)
        return false;

    // Fit invariant (holds whenever k is not clamped to the 3 minimum).
    if(k > 3) {
        constexpr size_t base =
            P::size_helper::template calculate_aligned_base_size<F>();
        constexpr size_t full =
            base + (size_t)k * sizeof(typename P::value_type);
        if(full > (size_t)P::kTargetNodeBytes)
            return false;
    }
    return true;
}

// ============================================================================
// Tag types mapping node byte sizes to container instantiations
// ============================================================================

template <int Target> struct IntSet {
    using set_type = WTreeLib::set<int, std::less<int>, std::allocator<int>,
                                   Target>;
    static constexpr const char *key_name = "int";
    static constexpr int target = Target;
};

template <int Target> struct UcharSet {
    using set_type = WTreeLib::set<unsigned char, std::less<unsigned char>,
                                   std::allocator<unsigned char>, Target>;
    static constexpr const char *key_name = "unsigned char";
    static constexpr int target = Target;
};

template <int Target> struct UshortSet {
    using set_type = WTreeLib::set<unsigned short, std::less<unsigned short>,
                                   std::allocator<unsigned short>, Target>;
    static constexpr const char *key_name = "unsigned short";
    static constexpr int target = Target;
};

template <int Target> struct UlongSet {
    using set_type = WTreeLib::set<unsigned long, std::less<unsigned long>,
                                   std::allocator<unsigned long>, Target>;
    static constexpr const char *key_name = "unsigned long";
    static constexpr int target = Target;
};

template <int Target> struct StrSet {
    using set_type = WTreeLib::set<std::string, std::less<std::string>,
                                   std::allocator<std::string>, Target>;
    static constexpr const char *key_name = "std::string";
    static constexpr int target = Target;
};

// Every instantiation in the runtime matrix must satisfy the invariants.
using NodeSizeMatrix = std::tuple<
    IntSet<8>, IntSet<16>, IntSet<17>, IntSet<32>, IntSet<64>, IntSet<128>,
    IntSet<256>, IntSet<1020>, IntSet<1026>, IntSet<4096>,
    UcharSet<4>, UcharSet<8>, UcharSet<64>, UcharSet<256>, UcharSet<512>,
    UshortSet<8>, UshortSet<16>, UshortSet<64>, UshortSet<256>, UshortSet<512>,
    UshortSet<1020>, UshortSet<1026>,
    UlongSet<16>, UlongSet<64>, UlongSet<512>,
    StrSet<64>, StrSet<512>>;

template <typename Matrix, size_t... I>
constexpr bool check_matrix_invariants(std::index_sequence<I...>) {
    return (node_size_invariants<typename std::tuple_element_t<
                I, Matrix>::set_type>() &&
            ...);
}

static_assert(check_matrix_invariants<NodeSizeMatrix>(
                  std::make_index_sequence<std::tuple_size_v<NodeSizeMatrix>>()),
              "node-size matrix instantiation violates invariants");

// ============================================================================
// Runtime battery for one target size
// ============================================================================

constexpr int kElementsPerType = 3000;

template <typename SetType>
typename SetType::key_type make_key(int i) {
    using Key = typename SetType::key_type;
    if constexpr(std::is_integral_v<Key>) {
        return (Key)i;
    } else {
        return std::to_string(i);
    }
}

template <typename TagType> bool run_battery() {
    using SetType = typename TagType::set_type;
    using P = typename SetType::wtree_type::params_type;

    std::ostringstream title;
    title << "Node size battery [" << TagType::key_name << " keys, target="
          << TagType::target << " B, k=" << P::kTargetK
          << ", field=" << (P::kTargetK < 255 ? "uint8_t" : "uint16_t") << "]";
    TestPrinting::job_title(title.str());

    // Runtime battery: relocation is now allocator-safe for all value types
    // (move_values_to_node/move_values_to_blank_node placement-construct the
    // destination and destroy sources; std::string tail values are never
    // moved-from via std::move*).
    SetType storage;

    // Unsigned integral keys are truncated to their native domain; the 3000
    // default would wrap and collide, so cap at std::numeric_limits<Key>::max() + 1.
    const int n = [&]() {
        using Key = typename SetType::key_type;
        if constexpr(std::is_integral_v<Key>) {
            const unsigned long long domain =
                (unsigned long long)std::numeric_limits<Key>::max() + 1ull;
            return (int)std::min((unsigned long long)kElementsPerType, domain);
        } else {
            return kElementsPerType;
        }
    }();

    for(int i = 0; i < n; ++i) {
        auto [it, inserted] = storage.insert(make_key<SetType>(i));
        if(!inserted) {
            TestPrinting::job_bad_result("insert(" + std::to_string(i) +
                                         ") returned false");
            return false;
        }
    }

    if(storage.size() != (size_t)n) {
        TestPrinting::job_bad_result("unexpected size " +
                                     std::to_string(storage.size()));
        return false;
    }

    if(!WTreeValidationUtils::validate_wtree(*storage.tree(), n)) {
        TestPrinting::job_bad_result("tree invariants violated after insert");
        return false;
    }

    // Erase every third key, then re-validate.
    for(int i = 0; i < n; i += 3) {
        storage.erase(make_key<SetType>(i));
    }
    const int remaining = n - (n + 2) / 3;
    if(storage.size() != (size_t)remaining) {
        TestPrinting::job_bad_result("unexpected size after erase " +
                                     std::to_string(storage.size()) + " vs " +
                                     std::to_string(remaining));
        return false;
    }
    if(!WTreeValidationUtils::validate_wtree(*storage.tree(), remaining)) {
        TestPrinting::job_bad_result("tree invariants violated after erase");
        return false;
    }

    TestPrinting::job_correct_result("battery OK (size " +
                                     std::to_string(remaining) + ")");
    return true;
}

// ============================================================================
// Main
// ============================================================================

int main() {
    TestPrinting::job_title("Node-size parametrization (kTargetK vs "
                            "kTargetNodeBytes vs field_type)");

    TestResults results;
    results.pass("compile-time invariants for the whole matrix");

    bool ok = true;
    ok &= run_battery<IntSet<8>>();
    ok &= run_battery<IntSet<16>>();
    ok &= run_battery<IntSet<17>>();
    ok &= run_battery<IntSet<32>>();
    ok &= run_battery<IntSet<64>>();
    ok &= run_battery<IntSet<128>>();
    ok &= run_battery<IntSet<256>>();
    ok &= run_battery<IntSet<1020>>();
    ok &= run_battery<IntSet<1026>>();
    ok &= run_battery<IntSet<4096>>();

    ok &= run_battery<UcharSet<4>>();
    ok &= run_battery<UcharSet<8>>();
    ok &= run_battery<UcharSet<64>>();
    ok &= run_battery<UcharSet<256>>();
    ok &= run_battery<UcharSet<512>>();

    ok &= run_battery<UshortSet<8>>();
    ok &= run_battery<UshortSet<16>>();
    ok &= run_battery<UshortSet<64>>();
    ok &= run_battery<UshortSet<256>>();
    ok &= run_battery<UshortSet<512>>();
    ok &= run_battery<UshortSet<1020>>();
    ok &= run_battery<UshortSet<1026>>();

    ok &= run_battery<UlongSet<16>>();
    ok &= run_battery<UlongSet<64>>();
    ok &= run_battery<UlongSet<512>>();

    ok &= run_battery<StrSet<64>>();
    ok &= run_battery<StrSet<512>>();

    if(ok)
        results.pass("runtime battery for all target sizes");
    else
        results.fail("runtime battery for all target sizes");

    results.summary();
    return results.all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}