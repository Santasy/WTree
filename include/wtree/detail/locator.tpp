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

#ifndef _WTREE_LOCATOR__T_
#define _WTREE_LOCATOR__T_

#include "locator.hpp"

namespace WTreeLib {

template <typename Params>
template <typename IterType>
int WTreeLocator<Params>::internal_locate_any(const key_type &key,
                                              IterType &iter) const {
    if(iter.node->size() == 0)
        return node_type::miss_code();

    for(;;) {
        assert(iter.node->size() > 0);

        // Bounds check for early stop.
        // For compare_to, use three-way comparison to detect exact matches
        // at the boundaries for free.
        if constexpr(is_key_compare_to::value) {
            int cmp_first = key_comp()(key, iter.node->key(0));
            if(cmp_first < 0) {
                iter.index = 0;
                break;
            }
            if(cmp_first == 0) {
                iter.index = 0;
                return node_type::hit_code();
            }

            int cmp_last =
                key_comp()(iter.node->key(iter.node->size() - 1), key);
            if(cmp_last < 0) {
                iter.index = iter.node->size();
                break;
            }
            if(cmp_last == 0) {
                iter.index = iter.node->size() - 1;
                return node_type::hit_code();
            }
        } else {
            if(key_comp()(key, iter.node->key(0))) {
                iter.index = 0;
                break;
            }
            if(key_comp()(iter.node->key(iter.node->size() - 1), key)) {
                iter.index = iter.node->size();
                break;
            }
        }

        // Bounded search — key is within [key(0), key(size-1)].
        if constexpr(is_key_compare_to::value) {
            int res;
            if(iter.node->is_internal())
                res = iter.node->bounded_lower_bound_internal(key, key_comp());
            else
                res = iter.node->bounded_lower_bound_leaf(key, key_comp());
            iter.index = res & kMatchMask;
            if(res & kExactMatch) {
                return node_type::hit_code();
            }
        } else {
            if(iter.node->is_internal())
                iter.index =
                    iter.node->bounded_lower_bound_internal(key, key_comp());
            else
                iter.index =
                    iter.node->bounded_lower_bound_leaf(key, key_comp());
            if(!key_comp()(key, iter.key())) { // Exact match.
                return node_type::hit_code();
            }
        }

        if(iter.index == 0)
            break;

        if(iter.node->is_leaf() || iter.node->child(iter.index - 1) == nullptr)
            break;

        iter.descend(iter.index - 1);
    }

    return node_type::miss_code();
}

template <typename Params>
template <bool UseLowerBound, typename IterType>
int WTreeLocator<Params>::internal_bound(const key_type &key,
                                         IterType &iter) const {
    if(iter.node->size() == 0)
        return node_type::miss_code();

    for(;;) {
        assert(iter.node->size() > 0);

        // Boundary containment (bool-normalized via compare_keys).
        if(compare_keys(key, iter.node->key(0))) {
            iter.index = 0;
            break;
        }
        if constexpr(UseLowerBound) {
            // Strictly-greater only: key == key(last) must fall through so the
            // bounded search can confirm the exact match (lower bound).
            if(compare_keys(iter.node->key(iter.node->size() - 1), key)) {
                iter.index = iter.node->size();
                break;
            }
        } else {
            // Upper bound excludes equality: key >= key(last) is past-end.
            if(!compare_keys(key, iter.node->key(iter.node->size() - 1))) {
                iter.index = iter.node->size();
                break;
            }
        }

        if(iter.node->is_leaf()) {
            if constexpr(UseLowerBound) {
                int res = iter.node->bounded_lower_bound_leaf(key, key_comp());
                iter.index = res & kMatchMask;
                if(node_type::template is_exact_hit<UseLowerBound>(
                       res, key, iter, key_comp()))
                    return node_type::hit_code();
            } else {
                iter.index =
                    iter.node->bounded_upper_bound_leaf(key, key_comp());
            }
            break;
        }

        // Internal node.
        if constexpr(UseLowerBound) {
            int res = iter.node->bounded_lower_bound_internal(key, key_comp());
            iter.index = res & kMatchMask;
            if(node_type::template is_exact_hit<UseLowerBound>(res, key, iter,
                                                               key_comp()))
                return node_type::hit_code();
        } else {
            iter.index =
                iter.node->bounded_upper_bound_internal(key, key_comp());
        }
        assert(iter.index > 0);
        assert(iter.index < iter.node->size());

        if(iter.node->child(iter.index - 1) == nullptr)
            break;

        iter.descend(iter.index - 1);
    }

    return node_type::miss_code();
}

template <typename Params>
template <typename IterType>
int WTreeLocator<Params>::internal_upper_bound(const key_type &key,
                                               IterType &iter) const {
    return internal_bound<false>(key, iter);
}

template <typename Params>
void WTreeLocator<Params>::visit_path_to_greatest_leaf_value(
    iterator &iter) noexcept {
    assert(iter.node);
    iter.index = iter.node->size() - 1;

    while(iter.node->is_internal()) {
        assert(iter.node->size() > 0);
        iter.index = iter.node->first_left_descendant(iter.index);
        if(iter.index == 0) {
            // u->setAsLeaf(); // Node may not need pointers.
            return;
        }
        iter.descend_to_right_side(iter.index - 1);
    }
}

template <typename Params>
void WTreeLocator<Params>::visit_path_to_smallest_leaf_value(
    iterator &iter) noexcept {
    assert(iter.node);
    iter.index = iter.node->size();

    while(iter.node->is_internal()) {
        assert(iter.node->size() > 0);
        iter.index = iter.node->first_right_descendant(0);
        if(iter.index == iter.node->size() - 1) {
            // u->setAsLeaf(); // Node may not need pointers.
            return;
        }
        iter.descend(iter.index);
    }
}

} // namespace WTreeLib
#endif