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

#ifndef _WTREE_MAP__H_
#define _WTREE_MAP__H_

#include "detail/index.hpp"
#include "detail/unique_container.hpp"

#include <memory>

namespace WTreeLib {

// Map params: value_type == pair<const Key, Value>, stored in the node's values
// array. Node size calculations use this pair size for slot layout.
/**
 * @brief Params definition for @ref map instantiations.
 * @details Value type is pair<const Key, Value>, stored in the node's values
 * array; node size calculations use the pair size for slot layout.
 */
template <typename Key, typename Value, typename Compare = std::less<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>,
          int TargetNodeSize = WTREE_TARGET_NODE_BYTES, bool Unique = true,
          typename BalanceOptions = WTreeBalanceOptions<>>
struct WTreeMapParams
    : public WTreeCommonParams<Key, Compare, Alloc, TargetNodeSize,
                               std::pair<const Key, Value>, Unique,
                               BalanceOptions> {
  public:
    using base_type =
        WTreeCommonParams<Key, Compare, Alloc, TargetNodeSize,
                          std::pair<const Key, Value>, Unique, BalanceOptions>;

    using data_type = Value;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using mutable_value_type = std::pair<Key, data_type>;

    static const Key &get_key(const value_type &x) { return x.first; }
    static const Key &key(const mutable_value_type &x) { return x.first; }

    static void swap(mutable_value_type &a, mutable_value_type &b) {
        wtree_swap_helper(a.first, b.first);
        wtree_swap_helper(a.second, b.second);
    }
    static void swap(mutable_value_type *a, mutable_value_type *b) {
        wtree_swap_helper(a->first, b->first);
        wtree_swap_helper(a->second, b->second);
    }
};

/**
 * A common base class for map and safe_map.
 * @tparam WTree A @ref WTree "WTree<WTreeMapParams<...>>" instantiation.
 */
template <typename WTree>
class WTreeMapContainer : public WTreeUniqueContainer<WTree> {
    using super_type = WTreeUniqueContainer<WTree>;

  public:
    using typename super_type::const_iterator;
    using typename super_type::iterator;
    using typename super_type::key_type;
    using typename super_type::value_type;
    using data_type = typename WTree::data_type;
    using mapped_type = typename WTree::mapped_type;

    // Inherit all constructors from WTreeUniqueContainer.
    using super_type::super_type;

    // Access specified element with bounds checking.
    mapped_type &at(const key_type &key) {
        auto it = this->find(key);
        if(it == this->end()) {
            throw std::out_of_range("map::at:  key not found");
        }
        return it->second;
    }
    const mapped_type &at(const key_type &key) const {
        auto it = this->find(key);
        if(it == this->end()) {
            throw std::out_of_range("map::at:  key not found");
        }
        return it->second;
    }

    // Insertion routines.
    data_type &operator[](const key_type &key) {
        return this->try_emplace(key).first->second;
    }

    data_type &operator[](key_type &&key) {
        return this->try_emplace(std::move(key)).first->second;
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(const key_type &key, Args &&...args) {
        return this->tree()->emplace_unique_key_args(
            key, std::piecewise_construct, std::forward_as_tuple(key),
            std::forward_as_tuple(std::forward<Args>(args)...));
    }

    template <typename... Args>
    std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args) {
        return this->tree()->emplace_unique_key_args(
            key, std::piecewise_construct,
            std::forward_as_tuple(std::move(key)),
            std::forward_as_tuple(std::forward<Args>(args)...));
    }

    template <typename... Args>
    iterator try_emplace(const_iterator hint, const key_type &key,
                         Args &&...args) {
        return this->tree()->emplace_hint_unique_key_args(
            hint, key, std::piecewise_construct, std::forward_as_tuple(key),
            std::forward_as_tuple(std::forward<Args>(args)...));
    }

    template <typename... Args>
    iterator try_emplace(const_iterator hint, key_type &&key, Args &&...args) {
        return this->tree()->emplace_hint_unique_key_args(
            hint, key, std::piecewise_construct,
            std::forward_as_tuple(std::move(key)),
            std::forward_as_tuple(std::forward<Args>(args)...));
    }

    // Insert a new element, or assign if the key already exists (C++17).
    template <typename M>
    mapped_type &insert_or_assign(const key_type &key, M &&obj) {
        std::pair<iterator, bool> res = this->tree()->emplace_unique_key_args(
            key, std::piecewise_construct, std::forward_as_tuple(key),
            std::forward_as_tuple(std::forward<M>(obj)));
        if(!res.second)
            res.first->second = std::forward<M>(obj);
        return res.first->second;
    }

    template <typename M>
    mapped_type &insert_or_assign(key_type &&key, M &&obj) {
        std::pair<iterator, bool> res = this->tree()->emplace_unique_key_args(
            key, std::piecewise_construct,
            std::forward_as_tuple(std::move(key)),
            std::forward_as_tuple(std::forward<M>(obj)));
        if(!res.second)
            res.first->second = std::forward<M>(obj);
        return res.first->second;
    }
};

/**
 * @brief A @ref WTree "WTree"-based map of unique keys, each mapped to a value.
 * @tparam Key The key type.
 * @tparam Value The mapped value type.
 * @tparam Compare Comparator that orders keys; defaults to std::less<Key>.
 * @tparam Alloc Element allocator (defaults to an allocator of
 *         std::pair<const Key, Value>).
 * @tparam TargetNodeSize Target node byte size (cache-size inspired).
 * @tparam BalanceOptions A @ref WTreeBalanceOptions policy.
 */
template <typename Key, typename Value, typename Compare = std::less<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value>>,
          int TargetNodeSize = WTREE_TARGET_NODE_BYTES, bool Unique = true,
          typename BalanceOptions = WTreeBalanceOptions<>>
class map : public WTreeMapContainer<
                WTree<WTreeMapParams<Key, Value, Compare, Alloc, TargetNodeSize,
                                     Unique, BalanceOptions>>> {
    using self_type =
        map<Key, Value, Compare, Alloc, TargetNodeSize, Unique, BalanceOptions>;
    using super_type = WTreeMapContainer<WTree<WTreeMapParams<
        Key, Value, Compare, Alloc, TargetNodeSize, Unique, BalanceOptions>>>;

  public:
    // Inherit all constructors from WTreeMapContainer.
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

template <typename Key, typename Value, typename Compare, typename Alloc,
          int TargetNodeSize, bool Unique, typename BalanceOptions>
inline void
swap(map<Key, Value, Compare, Alloc, TargetNodeSize, Unique, BalanceOptions> &a,
     map<Key, Value, Compare, Alloc, TargetNodeSize, Unique, BalanceOptions>
         &b) noexcept(noexcept(a.swap(b))) {
    a.swap(b);
}

} // namespace WTreeLib

#endif