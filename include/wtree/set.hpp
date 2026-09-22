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

#ifndef _WTREE_SET__H_
#define _WTREE_SET__H_

#include "detail/index.hpp"
#include "detail/unique_container.hpp"

#include <memory>

namespace WTreeLib {

// Set params: value_type == Key, stored directly in the node's values array.
/**
 * @brief Params definition for @ref set instantiations.
 * @details Value type equals Key and is stored directly in the node's
 * values array.
 */
template <typename Key, typename Compare, typename Alloc, int TargetNodeSize,
          bool Unique = true, typename BalanceOptions = WTreeBalanceOptions<>>
struct WTreeSetParams
    : public WTreeCommonParams<Key, Compare, Alloc, TargetNodeSize, Key, Unique,
                               BalanceOptions> {
    using base = WTreeCommonParams<Key, Compare, Alloc, TargetNodeSize, Key,
                                   Unique, BalanceOptions>;
    using typename base::value_type;

    // No mapped data for sets.
    using data_type = std::false_type;
    using mapped_type = std::false_type;

    static void swap(value_type &a, value_type &b) { wtree_swap_helper(a, b); }
    static const Key &get_key(const value_type &x) { return x; }
};

/**
 * @brief A @ref WTree "WTree"-based set of unique keys.
 * @tparam Key The key type stored.
 * @tparam Compare Comparator that orders keys; defaults to std::less<Key>.
 * @tparam Alloc Element allocator.
 * @tparam TargetNodeSize Target node byte size (cache-size inspired).
 * @tparam BalanceOptions A @ref WTreeBalanceOptions policy.
 */
template <typename Key, typename Compare = std::less<Key>,
          typename Alloc = std::allocator<Key>,
          int TargetNodeSize = WTREE_TARGET_NODE_BYTES,
          typename BalanceOptions = WTreeBalanceOptions<>>
class set : public WTreeUniqueContainer<WTree<WTreeSetParams<
                Key, Compare, Alloc, TargetNodeSize, true, BalanceOptions>>> {
    using self_type = set<Key, Compare, Alloc, TargetNodeSize, BalanceOptions>;
    using super_type = WTreeUniqueContainer<WTree<WTreeSetParams<
        Key, Compare, Alloc, TargetNodeSize, true, BalanceOptions>>>;

  public:
    // Inherit all constructors from WTreeUniqueContainer.
    using super_type::super_type;

    bool operator==(const self_type &other) const {
        return this->size() == other.size() &&
               std::equal(this->cbegin(), this->cend(), other.cbegin());
    }

    bool operator<(const self_type &other) const {
        return std::lexicographical_compare(this->cbegin(), this->cend(),
                                            other.cbegin(), other.cend());
    }

    bool operator!=(const self_type &other) const { return !(*this == other); }

    bool operator>(const self_type &other) const { return other < *this; }

    bool operator>=(const self_type &other) const { return !(*this < other); }

    bool operator<=(const self_type &other) const { return !(other < *this); }
};

template <typename Key, typename Compare, typename Alloc, int TargetNodeSize,
          typename BalanceOptions>
inline void swap(set<Key, Compare, Alloc, TargetNodeSize, BalanceOptions> &a,
                 set<Key, Compare, Alloc, TargetNodeSize, BalanceOptions>
                     &b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

} // namespace WTreeLib

#endif