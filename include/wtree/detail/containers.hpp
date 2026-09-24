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

#ifndef _WTREE_CONTAINERS__H_
#define _WTREE_CONTAINERS__H_

// Must include for traits macros resolution:
#include "traits.hpp"

#include <utility>

namespace WTreeLib {

/**
 * @brief Holds all types and constants derived from the required types and
 * node byte-sizes.
 * @tparam BalanceOptions A @ref WTreeBalanceOptions policy used by this
 *         instantiation.
 */
template <typename Key, typename Compare, typename Alloc,
          int TargetNodeBytes = WTREE_TARGET_NODE_BYTES,
          typename ValueType = Key, bool HoldsUnique = true,
          typename BalanceOptions = WTreeBalanceOptions<>>
struct WTreeCommonParams : public BalanceOptions {
  public:
    // Whether this container enforces unique keys (set/map) or allows
    // duplicates (multiset/multimap).
    static constexpr bool kUnique = HoldsUnique;
    // The balance policy selected for this instantiation.
    using balance_options = BalanceOptions;
    // If Compare is derived from wtree_key_compare_to_tag then use it as the
    // key_compare type. Otherwise, use wtree_key_compare_to_adapter<> which
    // will fall-back to Compare if we don't have an appropriate specialization.
    using key_compare =
        std::conditional_t<WTreeIsKeyCompareTo<Compare>::value, Compare,
                           WTreeKeyCompareToAdapter<Compare>>;
    // A type which indicates if we have a key-compare-to functor or a plain old
    // key-compare functor.
    using is_key_compare_to = WTreeIsKeyCompareTo<key_compare>;

    using key_type = Key;
    using value_type = ValueType;

    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using allocator_type = Alloc;
    using allocator_traits = std::allocator_traits<allocator_type>;

    // Internal allocator for node storage
    using internal_allocator_type =
        typename allocator_traits::template rebind_alloc<char>;
    using internal_allocator_traits =
        std::allocator_traits<internal_allocator_type>;

    using size_helper = WTreeSizeHelper<key_type, TargetNodeBytes, value_type>;

    static constexpr uint kTargetNodeBytes = TargetNodeBytes;
    static constexpr uint kTargetK =
        size_helper::determine_optimal_k(kTargetNodeBytes);
    static constexpr uint kLastGrowth =
        size_helper::determine_last_growth_limit(kTargetK);
    // Initial byte budget for a fresh leaf, derived from the instantiated
    // node size (cache-size inspired).
    static constexpr uint kTargetInitialBytes =
        size_helper::determine_initial_bytes(kTargetNodeBytes);

    using field_type =
        typename std::conditional < kTargetK<255, uint8_t, uint16_t>::type;

    // Binary search thresholds based on key type complexity.
    static constexpr bool is_numeric_key =
        std::is_integral<key_type>::value ||
        std::is_floating_point<key_type>::value;
    static constexpr uint kBinarySearchThreshold =
        is_numeric_key ? BalanceOptions::binary_search_threshold_numeric
                       : BalanceOptions::binary_search_threshold_complex;

    // Whether binary search should be used for full internal nodes.
    // Internal nodes are always full with kTargetK elements.
    static constexpr bool kUseBinarySearchForInternal =
        kTargetK >= kBinarySearchThreshold;
};

/**
 * A common base class for WTreeLib::set, map, multiset and multimap.
 * @tparam Tree A @ref WTree "WTree<Params>" instantiation, where Params is
 *         @ref WTreeSetParams or @ref WTreeMapParams.
 */
template <typename Tree> class WTreeContainer {

  public:
    using wtree_type = Tree;

    using params_type = typename Tree::params_type;
    using key_type = typename Tree::key_type;
    using value_type = typename Tree::value_type;

    using key_compare = typename Tree::key_compare;
    using allocator_type = typename Tree::allocator_type;

    using pointer = typename Tree::pointer;
    using const_pointer = typename Tree::const_pointer;
    using reference = typename Tree::reference;
    using const_reference = typename Tree::const_reference;

    using size_type = typename Tree::size_type;
    using difference_type = typename Tree::difference_type;

    using iterator = typename Tree::iterator;
    using const_iterator = typename Tree::const_iterator;
    using reverse_iterator = typename Tree::reverse_iterator;
    using const_reverse_iterator = typename Tree::const_reverse_iterator;

    static constexpr int kExactMatch = Tree::kExactMatch;
    static constexpr int kMatchMask = Tree::kMatchMask;

  protected:
    Tree m_tree;

  public:
    // Default constructor.
    WTreeContainer(const key_compare &comp = key_compare(),
                   const allocator_type &alloc = allocator_type())
        : m_tree(comp, alloc) {}

    // Copy/move/assign.
    WTreeContainer(const WTreeContainer &) = default;
    WTreeContainer(WTreeContainer &&) noexcept = default;
    WTreeContainer &operator=(const WTreeContainer &) = default;
    WTreeContainer &operator=(WTreeContainer &&) noexcept = default;

    Tree *tree() noexcept { return &m_tree; };
    const Tree *tree() const noexcept { return &m_tree; };

    void clear() noexcept { m_tree.clear(); }

    void swap(WTreeContainer &other) noexcept { m_tree.swap(other.m_tree); }

    // Iterator routines.
    iterator begin() noexcept { return m_tree.begin(); }
    const_iterator begin() const noexcept { return m_tree.begin(); }
    const_iterator cbegin() const noexcept { return m_tree.cbegin(); }
    iterator end() noexcept { return m_tree.end(); }
    const_iterator end() const noexcept { return m_tree.cend(); }
    const_iterator cend() const noexcept { return m_tree.cend(); }
    reverse_iterator rbegin() noexcept { return m_tree.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return m_tree.rbegin(); }
    const_reverse_iterator crbegin() const noexcept { return m_tree.crbegin(); }
    reverse_iterator rend() noexcept { return m_tree.rend(); }
    const_reverse_iterator rend() const noexcept { return m_tree.rend(); }
    const_reverse_iterator crend() const noexcept { return m_tree.crend(); }

    allocator_type get_allocator() const noexcept {
        return allocator_type(m_tree.allocator());
    }

    size_type size() const noexcept { return m_tree.size(); }
    size_type max_size() const noexcept { return m_tree.max_size(); }
    bool empty() const noexcept { return m_tree.is_empty(); }

    key_compare key_comp() const { return m_tree.key_comp(); }

    // === Lookup routines (shared for unique and multi) ===

    bool contains(const key_type &key) const {
        const_iterator iter(m_tree.croot(), 0);
        auto res = m_tree.locate(key, iter);
        return res.second == kExactMatch;
    }

    iterator lower_bound(const key_type &key) {
        iterator iter(m_tree.root(), 0);
        return m_tree.lower_bound(key, iter).first;
    }
    const_iterator lower_bound(const key_type &key) const {
        const_iterator iter(m_tree.croot(), 0);
        return m_tree.lower_bound(key, iter).first;
    }

    iterator upper_bound(const key_type &key) {
        iterator iter(m_tree.root(), 0);
        return m_tree.upper_bound(key, iter).first;
    }
    const_iterator upper_bound(const key_type &key) const {
        const_iterator iter(m_tree.croot(), 0);
        return m_tree.upper_bound(key, iter).first;
    }

    std::pair<iterator, iterator> equal_range(const key_type &key) {
        return {lower_bound(key), upper_bound(key)};
    }
    std::pair<const_iterator, const_iterator>
    equal_range(const key_type &key) const {
        return {lower_bound(key), upper_bound(key)};
    }

    // === Deletion routines (shared) ===

    iterator erase(iterator iter) noexcept { return m_tree.erase(iter); }
    void erase(const iterator &first, const iterator &last) noexcept {
        m_tree.erase(first, last);
    }

    // === WTree-specific stats ===
    // Each single-metric accessor below traverses the whole tree (O(nodes));
    // call collect_stats() once when several metrics are needed and read the
    // returned node_stats fields instead.
    // Follow to tree.hpp for more details.

    using node_stats = Tree::node_stats;

    node_stats collect_stats() const { return m_tree.collect_stats(); }

    size_type height() const { return m_tree.height(); }
    size_type leaf_nodes() const { return m_tree.leaf_nodes(); }
    size_type internal_nodes() const { return m_tree.internal_nodes(); }
    size_type nodes() const { return m_tree.nodes(); }
    size_type bytes_used() const { return m_tree.bytes_used(); }
    double average_bytes_per_value() {
        return m_tree.average_bytes_per_value();
    }
    double total_overhead() const { return m_tree.total_overhead(); }
    double overhead() const { return m_tree.overhead(); }
    double fullness() const { return m_tree.fullness(); }
    double occupancy() const { return m_tree.occupancy(); }
};

} // namespace WTreeLib

#endif