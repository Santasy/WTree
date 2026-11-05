/*
 * Copyright (c) 2026 Sebastián Pacheco Cáceres
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WTREE_TRAITS__H_
#define _WTREE_TRAITS__H_

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

// === User setup ===
// These parameters may be configured by compilation flags.

// * [FEATURE] Use this flag to debug the iterator constructor calls:
// WTREE_DBG_ITERATORS

// * [FEATURE] Use this flag for additional debug-related messages:
// DBGVERBOSE

// * [FEATURE] Use this flag to enable the slide branch applied on balance when
// inserting in the W-Tree (default: true):
// WTREE_USE_SLIDE

// * [FEATURE] Use this flag to enable the split branch applied on balance when
// inserting in the W-Tree (default: false):
// WTREE_USE_SPLIT

#ifndef WTREE_USE_SLIDE
#define WTREE_USE_SLIDE true
#endif

#ifndef WTREE_USE_SPLIT
#define WTREE_USE_SPLIT false
#endif

#ifndef WTREE_TARGET_NODE_BYTES
#define WTREE_TARGET_NODE_BYTES 512
#endif

// Binary search thresholds: minimum node fill (leaf) or fan-out (always-full
// internal) that justifies binary search over the linear scan. A binary scan
// of 8 slots costs ~3 comparisons against up to 8 for linear, so 8 keKys is
// the break-even point; below it the short linear scan is cheaper and keeps
// sequential memory access. Both classes share the value: key-type
// complexity only re-tuned these two numbers (256/32), which made binary
// search unreachable for every node size the W-tree derives below a 2 KB
// target (k < 256 for numeric keys, k < 32 for typical complex keys).

#ifndef WTREE_BINARY_SEARCH_THRESHOLD_COMPLEX
#define WTREE_BINARY_SEARCH_THRESHOLD_COMPLEX 16
#endif

// Leaf in-node search policy. Internal nodes decide at compile time (they are
// always full); leaves have a variable fill, so the classic rule re-decides on
// every visit with `size() >= threshold`.
//
// The default resolves per key class:
//   0 = always linear      (no fill check)
//   1 = class default      ← DEFAULT: numeric keys linear; complex keys keep
//                            the fill check (Auto), because a complex
//                            comparison costs far more than a linear scan
//                            step.
//   2 = always binary      (no fill check)
//   3 = always fill-checked (the previous behaviour, kept for A/B)
//
// A binary-search threshold of 1 still forces binary for leaves.
#ifndef WTREE_LEAF_SEARCH
#define WTREE_LEAF_SEARCH 1
#endif

// Boundary probes before the in-node search:
//  -1 = auto (DEFAULT: 0 when kTargetK >= 127, else 2)
//   0 = none (plain search, explicit end test)
//   1 = first key only (plain search; the far side scans)
//   2 = first and last (bounded closed_* search)
//
// Only 2 may use the closed_* search: those loops drop the per-element end
// test because the probes guarantee the key is inside the node. Modes 0/1
// must use the unbounded search and re-check exactness. The probe/loop pair
// is one design (FINDINGS.md §10/H12): do not mix them.
//
// The auto border is the 10M allops result: the probes
// protect the small-node insert path (k <= 63), while from k >= 127 the
// search win dominates. This is the single place to adjust the border for
// complex keys once that data exists.
#ifndef WTREE_SEARCH_PROBES
#define WTREE_SEARCH_PROBES -1
#endif

// Edge insertion at a full internal node rotates the boundary key down the
// chain. For trivially copyable values a single descending pass carrying the
// key in a local replaces the descend-then-ascend rotation (see
// emplace_edge_swap). 0 forces the complex path for measurements.
#ifndef WTREE_EDGE_SWAP_NUMERIC
#define WTREE_EDGE_SWAP_NUMERIC 1
#endif

// Form of the in-node linear scan. -1 = auto (pointer walk at k <= 63, indexed
// loop above), 1 = pointer walk, 0 = indexed. Both forms are semantically
// identical (same comparator, same bounded no-end-test contract, same
// exact-match flag), but the indexed form zero-extends the uint8 index twice
// per key examined while the pointer walk compiles to a bare `cmp mem,reg`
// (P16/FINDINGS §28). The 10M A/B: pointer wins 6-12 % at nb 128/256 and is
// neutral at nb 512, hence the k <= 63 border.
#ifndef WTREE_SCAN_POINTER
#define WTREE_SCAN_POINTER -1
#endif

// Numeric keys: a linear scan of contiguous ints is cheaper than a binary one
// until a node holds ~254 keys (measured on 4-byte keys: k=127 linear wins by
// 20-45 %, k=254 binary wins by 8-30 % on insert/search/miss, k=510 binary wins
// by ~30 %). Binary is only worth it for *full* leaves, hence the threshold is
// compared against a node's fill; internal nodes always decide on kTargetK.
// 254 is the smallest capacity the grid reaches where binary pays, so it is the
// inclusive boundary, and it also covers k = 255 (a 1028 B node for uint).
// See experiments/analysis/comparing/FINDINGS.md §1, §7 and §11.
#ifndef WTREE_BINARY_SEARCH_THRESHOLD_NUMERIC
#define WTREE_BINARY_SEARCH_THRESHOLD_NUMERIC 254
#endif

// Initial byte budget of a fresh leaf (it grows x1.25 per step up to
// WTREE_TARGET_NODE_BYTES). 0 = derive it from the target node size (the
// library default: <=64 B nodes start at 16, larger ones at 40). Set it to
// sweep the growth ratchet in the comparing microbench.
#ifndef WTREE_TARGET_INITIAL_BYTES
#define WTREE_TARGET_INITIAL_BYTES 0
#endif

// === End of user setup ===
// =========================

namespace WTreeLib {
// Define a function for the next capacity of a leaf node.
// Current keeps n/k >= 80%.
//
// Growth adds one quarter of the current size, rounded via (c+1)>>2. The
// rounding keeps the increment >= 1 for every c >= 3 (kInitialCapacity is
// clamped to >= 3), so no separate floor branch is needed.
template <typename Field> constexpr Field NEW_SIZE(Field current_size) {
    return current_size + ((current_size + 1) >> 2);
}

// Same growth, but once current_size reaches the last-growth limit no more
// arithmetic is performed and the capacity becomes K directly. This avoids
// uint8_t overflow (current_size + (current_size+1)>>2) near the top.
template <typename Field, Field Limit, Field K>
constexpr Field SAFE_NEW_SIZE(Field current_size) {
    return current_size < Limit ? NEW_SIZE<Field>(current_size) : K;
}

// ============================================================================
// Rules & Size Traits
// ============================================================================

/**
 * @brief User-selectable balance and search-threshold options for a WTree.
 * @details The selectable balance operations are slide and split, plus the
 * binary-search thresholds that decide linear vs binary search inside a
 * node. Pass this type as the trailing template argument to set/map so
 * different instantiations can use different policies; the defaults rely on
 * the WTREE_USE_SLIDE, WTREE_USE_SPLIT and WTREE_BINARY_SEARCH_THRESHOLD_*
 * macros.
 */
template <bool UseSlide = WTREE_USE_SLIDE, bool UseSplit = WTREE_USE_SPLIT,
          int NumericSearchThreshold = WTREE_BINARY_SEARCH_THRESHOLD_NUMERIC,
          int ComplexSearchThreshold = WTREE_BINARY_SEARCH_THRESHOLD_COMPLEX>
struct WTreeBalanceOptions {
    static constexpr bool use_slide = UseSlide;
    static constexpr bool use_split = UseSplit;
    static constexpr int binary_search_threshold_numeric =
        NumericSearchThreshold;
    static constexpr int binary_search_threshold_complex =
        ComplexSearchThreshold;
};

/**
 * @brief Compile-time validation of assumptions over the implementation.
 * A change in these priorities must be carefully deliberated.
 */
struct WTreeRulesAssumptions {
    // Assumptions for efficient performance. constexpr so the balance rules can
    // be folded at compile time (HPC: no runtime read of a global).
    static constexpr bool slide_over_split = true;
    static constexpr bool slide_to_right_first = true;
    static constexpr bool split_to_right_first = true;
};

template <typename Key, int TargetNodeBytes = WTREE_TARGET_NODE_BYTES,
          typename ValueType = Key>
struct WTreeSizeHelper {
  public:
    using key_type = Key;
    using value_type = ValueType;
    using difference_type = std::ptrdiff_t;

    static constexpr uint kTargetBytes = TargetNodeBytes;
    static constexpr ushort value_size = sizeof(value_type);

    // Helper to compute aligned offset for values array.
    // This accounts for padding between base_fields and values due to
    // alignment requirements of value_type.
    static constexpr size_t align_up(size_t size, size_t alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }

    // Mirror of node::base_fields layout: two FieldType + bool.
    // Using sizeof on this lets the compiler handle alignment and padding.
    template <typename FieldType> struct base_fields_layout {
        FieldType size;
        FieldType capacity;
        bool is_internal;
    };

    // Calculate the actual size of base_fields struct for a given field_type.
    template <typename FieldType>
    static constexpr size_t calculate_base_fields_size() {
        return sizeof(base_fields_layout<FieldType>);
    }

    // Calculate total base size when followed by value_type array
    // (accounts for padding between base_fields and the values array)
    template <typename FieldType>
    static constexpr size_t calculate_aligned_base_size() {
        constexpr size_t base_size = calculate_base_fields_size<FieldType>();
        return align_up(base_size, alignof(value_type));
    }

    /**
     * @brief Determines the optimal k-value from target_node_bytes.
     * @details First tries a small uchar field type; if the number of keys
     * exceeds uchar, a ushort type is used.
     */
    /**
     * @brief Keys per node that fit in -target_node_bytes- (no clamping).
     * @details The caller rejects a too-small result at compile time
     * (WTreeCommonParams asserts k >= 3), so the value is returned as is: a
     * silent clamp to 3 would produce a node larger than the requested budget
     * and leaves whose capacity is below the growth floor.
     */
    template <typename ValType = value_type>
    static constexpr int determine_optimal_k(int target_node_bytes) {
        // Try with small field_type (uint8_t) first
        constexpr size_t small_base_size =
            calculate_aligned_base_size<unsigned char>();
        const int max_available_bytes = target_node_bytes - small_base_size;
        const int k_for_small_base = max_available_bytes / value_size;
        if(k_for_small_base < 255)
            return k_for_small_base;

        // Need larger field_type (uint16_t) for k >= 255
        constexpr size_t large_base_size =
            calculate_aligned_base_size<unsigned short>();
        const int min_available_bytes = target_node_bytes - large_base_size;
        return min_available_bytes / value_size;
    }

    // N ->S + S/4
    // N ->S 5/4
    // N 4/5 -> S
    static constexpr int determine_last_growth_limit(int limit) {
        return (limit / 5) << 2;
    }

    // Initial node byte budget, cache-size inspired: a fresh leaf starts
    // small enough to stay L1-resident and grows by x1.25 from there.
    // WTREE_TARGET_INITIAL_BYTES overrides it (clamped to the target size);
    // 0 keeps the derived default: 16 B at <=64 B nodes, 40 B above.
    // The node layer still enforces
    // kInitialCapacity <= kTargetK and >= 3.
    static constexpr int determine_initial_bytes(int target_node_bytes) {
        if constexpr(WTREE_TARGET_INITIAL_BYTES > 0) {
            return WTREE_TARGET_INITIAL_BYTES < target_node_bytes
                       ? WTREE_TARGET_INITIAL_BYTES
                       : target_node_bytes;
        } else {
            return target_node_bytes > 64    ? 40
                   : target_node_bytes >= 16 ? 16
                                             : target_node_bytes;
        }
    }
};

// ============================================================================
// Key Comparison Traits — tag dispatch, adapters, and comparers
// ============================================================================

// A helper type used to indicate that a key-compare-to functor has been
// provided. A user can specify a key-compare-to functor by doing:
//
//  struct MyStringComparer
//      : public util::wtree::wtree_key_compare_to_tag {
//    int operator()(const string& a, const string& b) const {
//      return a.compare(b);
//    }
//  };
//
// Note that the return type is an int and not a bool. There is a
// COMPILE_ASSERT which enforces this return type.
struct WTreeKeyCompareToTag {};

// A helper class that indicates if the Compare parameter is derived from
// wtree_key_compare_to_tag.
template <typename Compare>
struct WTreeIsKeyCompareTo
    : public std::is_convertible<Compare, WTreeKeyCompareToTag> {};

// A helper class to convert a boolean comparison into a three-way
// "compare-to" comparison that returns a negative value to indicate
// less-than, zero to indicate equality and a positive value to
// indicate greater-than. This helper class is specialized for
// less<string> and greater<string>.
//
// The wtree_key_compare_to_adapter class is provided for efficiency seek.
template <typename Compare> struct WTreeKeyCompareToAdapter : Compare {
    using Compare::Compare;
};

template <>
struct WTreeKeyCompareToAdapter<std::less<std::string>>
    : public WTreeKeyCompareToTag {
    WTreeKeyCompareToAdapter() {}
    WTreeKeyCompareToAdapter(const std::less<std::string> &s) {}
    int operator()(const std::string &a, const std::string &b) const {
        return a.compare(b);
    }
};

template <>
struct WTreeKeyCompareToAdapter<std::greater<std::string>>
    : public WTreeKeyCompareToTag {
    WTreeKeyCompareToAdapter() {}
    WTreeKeyCompareToAdapter(const std::greater<std::string> &s) {}
    int operator()(const std::string &a, const std::string &b) const {
        return b.compare(a);
    }
};

// A helper class that allows a compare-to functor to behave like a plain
// compare functor. When a compare-to functor is present some parts of the tree
// (e.g. insert-with-hint) still need a plain (boolean) comparison; the
// three-way result is normalized through a `< 0` test.
template <typename Key, typename Compare, bool HaveCompareTo>
struct WTreeKeyComparer {
    WTreeKeyComparer() {}
    WTreeKeyComparer(Compare c) : comp(c) {}
    static bool bool_compare(const Compare &comp, const Key &x, const Key &y) {
        if constexpr(HaveCompareTo)
            return comp(x, y) < 0;
        else
            return comp(x, y);
    }
    bool operator()(const Key &x, const Key &y) const {
        return bool_compare(comp, x, y);
    }
    Compare comp;
};

// A helper function to compare to keys using the specified compare
// functor. This dispatches to the appropriate wtree_key_comparer comparison,
// depending on whether we have a compare-to functor or not (which depends on
// whether Compare is derived from wtree_key_compare_to_tag).
template <typename Key, typename Compare>
static bool wtree_compare_keys(const Compare &comp, const Key &x,
                               const Key &y) {
    typedef WTreeKeyComparer<Key, Compare, WTreeIsKeyCompareTo<Compare>::value>
        key_comparer;
    return key_comparer::bool_compare(comp, x, y);
}

// ============================================================================
// Key Extraction Traits — tag dispatch for set vs map value extraction
// ============================================================================

struct WTreeExtractKeyFailTag {};
struct WTreeExtractKeySelfTag {};
struct WTreeExtractKeyFirstTag {};

template <typename ValueType, typename Key,
          typename RawValueType = std::remove_const<
              typename std::remove_reference<ValueType>::type>::type>
struct WTreeCanExtractKey
    : std::conditional<std::is_same<RawValueType, Key>::value,
                       WTreeExtractKeySelfTag, WTreeExtractKeyFailTag>::type {};

template <typename PairType, typename Key, typename First, typename Second>
struct WTreeCanExtractKey<PairType, Key, std::pair<First, Second>>
    : std::conditional<
          std::is_same<typename std::remove_const<First>::type, Key>::value,
          WTreeExtractKeyFirstTag, WTreeExtractKeyFailTag>::type {};

// wtree_can_extract_map_key uses true_type/false_type instead of the tags.
// It returns true if Key != ContainerValueType (the container is a map not a
// set) and ValueType == Key.
template <typename ValueType, typename Key, typename ContainerValueType,
          typename RawValueType = std::remove_const<
              typename std::remove_reference<ValueType>::type>::type>
struct WTreeCanExtractMapKey
    : std::integral_constant<bool, std::is_same<RawValueType, Key>::value> {};

// This specialization returns wtree_extract_key_fail_tag for non-map
// containers because Key == ContainerValueType
template <typename ValueType, typename Key, typename RawValueType>
struct WTreeCanExtractMapKey<ValueType, Key, Key, RawValueType>
    : std::false_type {};

// ============================================================================
// Search Dispatch — bound adapters and search strategy selection
// ============================================================================

// An adapter class that converts a lower-bound compare into an upper-bound
// compare.
template <typename Key, typename Compare>
struct WTreeUpperBoundAdapter : public Compare {
    WTreeUpperBoundAdapter(Compare c) : Compare(c) {}
    bool operator()(const Key &a, const Key &b) const {
        return !static_cast<const Compare &>(*this)(b, a);
    }
};

// ============================================================================
// Utilities
// ============================================================================

template <typename T> inline void wtree_swap_helper(T &a, T &b) {
    using std::swap;
    swap(a, b);
}

} // namespace WTreeLib
#endif
