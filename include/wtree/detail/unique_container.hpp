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

#ifndef _WTREE_UNIQUE_CONTAINER__H_
#define _WTREE_UNIQUE_CONTAINER__H_

#include "traits.hpp"
#include "node.hpp"
#include "containers.hpp"

#include <cassert>

namespace WTreeLib {

/**
 * A common base class for wtree::map and wtree::set.
 * @tparam Tree A @ref WTree "WTree<Params>" instantiation, where Params is
 *         @ref WTreeLib::WTreeSetParams "WTreeSetParams" or @ref
 * WTreeLib::WTreeMapParams "WTreeMapParams" with Unique == true.
 */
template <typename Tree>
class WTreeUniqueContainer : public WTreeContainer<Tree> {
    using super_type = WTreeContainer<Tree>;
    using params_type = typename Tree::params_type;

  public:
    using typename super_type::key_type;
    using typename super_type::value_type;

    using typename super_type::allocator_type;
    using typename super_type::size_type;

    using typename super_type::iterator;
    using typename super_type::const_iterator;

    using super_type::kExactMatch;
    using super_type::kMatchMask;

    using typename super_type::key_compare;
    typedef std::pair<iterator, bool> InsertResult;

  public:
    // Inherit default, copy, move constructors and assignment from base.
    using super_type::super_type;

    // Range constructor (unique: calls insert which checks duplicates).
    template <class InputIterator>
    WTreeUniqueContainer(InputIterator begin, InputIterator end,
                         const key_compare &comp = key_compare(),
                         const allocator_type &alloc = allocator_type())
        : super_type(comp, alloc) {
        insert(begin, end);
    }

    WTreeUniqueContainer(std::initializer_list<value_type> list,
                         const key_compare &comp = key_compare(),
                         const allocator_type &alloc = allocator_type())
        : super_type(comp, alloc) {
        insert(list.begin(), list.end());
    }

    // === Lookup routines (unique-specific) ===

    iterator find(const key_type &key) {
        iterator iter(this->m_tree.root(), 0);
        auto [it, _] = this->m_tree.lower_bound(key, iter);
        if(it != this->end()) {
            if(!this->m_tree.locator()->compare_keys(key,
                                                     params_type::get_key(*it)))
                return it;
        }
        return this->end();
    }

    const_iterator find(const key_type &key) const {
        const_iterator iter(this->m_tree.croot(), 0);
        auto [it, _] = this->m_tree.lower_bound(key, iter);
        if(it != this->cend()) {
            if(!this->m_tree.clocator()->compare_keys(
                   key, params_type::get_key(*it)))
                return it;
        }
        return this->cend();
    }

    iterator find_hint(const key_type &key, iterator it) {
        auto [_, res] = this->m_tree.locate_hint(key, it);
        return res == kExactMatch ? it : this->end();
    }

    const_iterator find_hint(const key_type &key, const_iterator it) const {
        auto [_, res] = this->m_tree.locate_hint(key, it);
        return res == kExactMatch ? it : this->cend();
    }

    size_type count(const key_type &key) const {
        const_iterator iter(this->m_tree.croot(), 0);
        auto res = this->m_tree.locate(key, iter);
        return (res.second == Tree::kExactMatch) ? 1 : 0;
    }

    // === Insertion routines ===

    InsertResult insert(const value_type &x) {
        return this->m_tree.insert_unique(x);
    }
    template <typename P> InsertResult insert(P &&x) {
        return this->m_tree.insert_unique(std::forward<P>(x));
    }
    InsertResult insert(value_type &&x) {
        return this->m_tree.insert_unique(std::move(x));
    }

    // Using iterators:

    iterator insert(const_iterator hint, const value_type &x) {
        return this->m_tree.insert_unique(hint, x);
    }

    template <typename P> iterator insert(const_iterator hint, P &&x) {
        return this->m_tree.insert_unique(hint, std::forward<P>(x));
    }
    iterator insert(const_iterator hint, value_type &&x) {
        return this->m_tree.insert_unique(hint, std::move(x));
    }

    void insert(std::initializer_list<value_type> il) {
        insert(il.begin(), il.end());
    }
    template <typename InputIterator>
    void insert(InputIterator f, InputIterator l) {
        for(const_iterator end = this->cend(); f != l; ++f) {
            insert(end, *f);
        }
    }

    template <typename... Args> InsertResult emplace(Args &&...args) {
        return this->m_tree.emplace_unique(std::forward<Args>(args)...);
    }

    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args &&...args) {
        return this->m_tree.emplace_hint_unique(hint,
                                                std::forward<Args>(args)...);
    }

    // === Deletion routines (unique-specific) ===

    using super_type::erase; // Inherit erase(iter) and erase(range) from base.

    int erase(const key_type &key) {
        iterator iter(this->m_tree.root(), 0);
        auto [_, i_result] = this->m_tree.locate(key, iter);
        if(i_result != kExactMatch)
            return 0;
        this->m_tree.erase(iter);
        return 1;
    }
};

} // namespace WTreeLib

#endif
