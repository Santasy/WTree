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

#ifndef _WTREE_LOCATOR__H_
#define _WTREE_LOCATOR__H_

#include "traits.hpp"
#include "type_aliases.hpp"

#include <cassert>

namespace WTreeLib {
/**
 * @brief Key-to-position descent routines, picking the search that matches
 * the key-comparison style of the instantiation.
 * @tparam Params A @ref WTree "WTree<Params>" instantiation.
 */
template <typename Params>
class WTreeLocator : protected WTreeTypeAliases<Params> {
    typedef WTreeLocator<Params> self_type;
    using Aliases = WTreeTypeAliases<Params>;

  public:
    using typename Aliases::const_iterator;
    using typename Aliases::const_pointer;
    using typename Aliases::const_reference;
    using typename Aliases::const_reverse_iterator;
    using typename Aliases::field_type;
    using typename Aliases::is_key_compare_to;
    using typename Aliases::iterator;
    using typename Aliases::key_compare;
    using typename Aliases::key_type;
    using typename Aliases::node_type;
    using typename Aliases::params_type;
    using typename Aliases::pointer;
    using typename Aliases::reference;
    using typename Aliases::reverse_iterator;
    using typename Aliases::value_type;

    using Aliases::kExactMatch;
    using Aliases::kMatchMask;
    using Aliases::kTargetK;

    const key_compare &m_comp;

    explicit WTreeLocator(const key_compare &comp) : m_comp(comp) {}

    const key_compare &key_comp() const { return m_comp; }

    bool compare_keys(const key_type &x, const key_type &y) const {
        return wtree_compare_keys(key_comp(), x, y);
    }

    /**
     * @brief Locates with early-stop descent, stopping at the first exact
     * match; does not guarantee finding the first or last occurrence.
     *
     * @return Match encoding depends on the compare style:
     * - compare_to:    kExactMatch on hit, -kExactMatch on miss.
     * - plain_compare: kExactMatch on hit, 0 on miss.
     */
    template <typename IterType>
    int internal_locate_any(const key_type &key, IterType &iter) const;

    template <typename IterType>
    int internal_locate_any_hint(const key_type &key, IterType &iter) const {
        internal_ascend_until_bound(key, iter);
        return internal_locate_any(key, iter);
    }

    template <typename IterType>
    void internal_ascend_until_bound(const key_type &key,
                                     IterType &iter) const {
        // First look upwards.
        // compare_keys normalizes the three-way result (compare-to) into the
        // containment test "key outside node key-range", so both compare
        // styles drive the ascent correctly.
        auto s = iter.ascendants.size();
        while(s-- > 0) {
            assert(iter.node->size() > 0);
            if(compare_keys(key, iter.node->key(0))) {
                iter.ascend();
                continue;
            }

            if(compare_keys(iter.node->key(iter.node->size() - 1), key)) {
                iter.ascend();
                ++iter.index;
                continue;
            }

            // Break when key is surely bounded by the node.
            break;
        }
    }

    // =====================================================================
    // Bound search — single parametric descent for lower and upper bounds.
    // UseLowerBound = true  → first key >= search key; exact matches are
    //                          detected (compare-to via the kExactMatch flag,
    //                          plain via one re-compare) and returned. The
    //                          node-level searches already find the leftmost
    //                          occurrence, so unique and multi share this.
    // UseLowerBound = false → first key > search key; never an exact match.
    //                          The node-level search runs through the
    //                          upper-bound adapter, which never sets
    //                          kExactMatch.
    // Boundary checks: key < key(0) → index 0. The trailing boundary differs:
    //   lower: strict key(last) < key → index size() (equality passes through
    //          so the bounded search can confirm the exact match);
    //   upper: key >= key(last) → index size() (upper bound excludes equality).
    // =====================================================================

    // From root — descend only.
    /**
     * @brief Bounded key-position descent for a lower or upper bound.
     * @tparam UseLowerBound Whether to find the first key >= key (`true`) or
     *         the first key > key (`false`, an upper bound).
     * @param iter Iterator carrying the search node + index; left holding the
     *        bound position in the leaf (index == size() means the key sorts
     *        past this subtree).
     * @return kExactMatch on hit, -kExactMatch/0 on miss (compare-to/plain);
     *         as an upper bound it never matches.
     */
    template <bool UseLowerBound, typename IterType>
    int internal_bound(const key_type &key, IterType &iter) const;

    // Lower bound entry — first key >= search key.
    template <typename IterType>
    int internal_lower_bound(const key_type &key, IterType &iter) const {
        return internal_bound<true>(key, iter);
    };

    // From hint — may ascend.
    /**
     * @brief Finds the first key >= key, ascending from the hint first.
     * @param iter Iterator carrying the search node + index; the hint's
     *        ascendants are walked until the search key is bounded by a node,
     *        then descent picks up from there.
     * @return kExactMatch on hit, -kExactMatch/0 on miss (compare-to/plain).
     */
    template <typename IterType>
    int internal_lower_bound_hint(const key_type &key, IterType &iter) const {
        internal_ascend_until_bound(key, iter);
        return internal_lower_bound(key, iter);
    };

    // =====================================================================
    // Upper bound — shared, descends fully to find first key > search key.
    // Used by emplace (both unique/multi) and the upper_bound() public API.
    // Same index == size() caveat as lower_bound.
    // The node-level upper-bound search inverts the comparator (via
    // WTreeUpperBoundAdapter), so the position it returns never carries an
    // exact-match flag.
    // =====================================================================

    // From root — descend only.
    /**
     * @brief Finds the first key > key, descending to the leaf.
     * @param iter Iterator carrying the search node + index; left holding the
     *        upper-bound position in the leaf (index == size() means the key
     *        sorts past this subtree).
     * @return 0 (plain compare) or -kExactMatch (compare-to); never matches.
     */
    template <typename IterType>
    int internal_upper_bound(const key_type &key, IterType &iter) const;

    // From hint — may ascend.
    /**
     * @brief Finds the first key > key, ascending from the hint first.
     * @param iter Iterator carrying the search node + index; the hint's
     *        ascendants are walked until the search key is bounded by a node,
     *        then descent picks up from there.
     * @return 0 (plain compare) or -kExactMatch (compare-to); never matches.
     */
    template <typename IterType>
    int internal_upper_bound_hint(const key_type &key, IterType &iter) const {
        internal_ascend_until_bound(key, iter);
        return internal_upper_bound(key, iter);
    }

    void visit_path_to_greatest_leaf_value(iterator &iter) noexcept;
    void visit_path_to_smallest_leaf_value(iterator &iter) noexcept;
};
} // namespace WTreeLib
#endif