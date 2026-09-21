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

#ifndef _WTREE_MANAGER__T_
#define _WTREE_MANAGER__T_

#include "traits.hpp"
#include "node_manager.hpp"

#include <algorithm>
#include <cstring>
#include <new>
#include <utility>

namespace WTreeLib {

// === Node allocation & growth ===

template <class Params>
inline typename WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::new_leaf_node(field_type capacity) {
    assert(capacity >= kInitialCapacity);
    internal_allocator_type &ia = mutable_allocator();
    const int nbytes = kBasefieldsBytes + (sizeof(value_type) * capacity);
    leaf_fields_type *u = reinterpret_cast<leaf_fields_type *>(
        internal_allocator_traits::allocate(ia, nbytes));
    return node_type::init_leaf(u, capacity);
}

template <class Params>
inline typename WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::new_internal_node() {
    internal_allocator_type &ia = mutable_allocator();
    internal_fields_type *u = reinterpret_cast<internal_fields_type *>(
        internal_allocator_traits::allocate(ia, sizeof(internal_fields_type)));
    return node_type::init_internal(u);
}

template <class Params>
inline typename WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::reallocate_leaf(node_type *node, field_type new_cap,
                                          field_type dest_offset) {
    assert(dest_offset + node->size() <= new_cap);
    node_type *new_node = new_leaf_node(new_cap);
    move_values_to_blank_node(node, new_node, node->size(), dest_offset);
    new_node->fields.size = dest_offset + node->size();
    delete_leaf_node(node);
    return new_node;
}

template <class Params>
inline typename WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::grow_leaf(node_type *node) {
    assert(node->capacity() < kTargetK);

    const field_type new_capacity = next_leaf_capacity(node->capacity());
    assert(new_capacity > node->capacity());
    return reallocate_leaf(node, new_capacity);
}

template <class Params>
inline typename WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::grow_leaf_to_size(node_type *node,
                                            field_type min_size) {
    if(min_size <= node->capacity())
        return node;

    const field_type new_cap = next_leaf_capacity(min_size);
    assert(new_cap > node->capacity());
    assert(new_cap >= min_size);
    return reallocate_leaf(node, new_cap);
}

template <class Params>
typename WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::grow_leaf_and_shift_right(node_type *node,
                                                    field_type new_size) {
    const field_type old_size = node->size();

    if(new_size <= node->capacity()) {
        // No reallocation: shift in-place.
        move_values_to_node(node->fields.values,
                            node->fields.values + old_size,
                            node->fields.values + (new_size - old_size));
        return node;
    }

    const field_type new_capacity = next_leaf_capacity(new_size);
    assert(new_capacity > node->capacity());
    assert(new_capacity >= new_size);
    return reallocate_leaf(node, new_capacity, new_size - old_size);
}

// === Leaf/internal conversion ===

template <class Params>
inline typename WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::make_internal(node_type *node) {
    if(node->is_internal())
        return node;

    node_type *new_node = new_internal_node();
    move_values_to_blank_node(node, new_node, node->size());
    new_node->fields.size = node->size();

    delete_leaf_node(node);
    return new_node;
}

template <class Params>
inline typename WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::make_leaf_from_internal(node_type *node) {
    assert(node->is_internal());
    node_type *leaf = new_leaf_node(kTargetK);

    move_values_to_blank_node(node, leaf, node->size());
    leaf->fields.size = node->size();

    delete_internal_node(node);
    return leaf;
}

template <class Params>
inline void
WTreeNodeManager<Params>::make_child_internal_unchecked(node_type *parent,
                                                        field_type i) {
    assert(parent->is_internal());
    assert(parent->child(i)->is_leaf());
    parent->fields.pointers[i] = make_internal(parent->child(i));
}

template <class Params>
inline void
WTreeNodeManager<Params>::make_child_leaf_unchecked(node_type *parent,
                                                    field_type i) {
    assert(parent->is_internal());
    assert(parent->child(i)->is_internal());
    parent->set_child(i, make_leaf_from_internal(parent->child(i)));
}

// === Node deletion ===

template <class Params>
inline void WTreeNodeManager<Params>::destroy_values(node_type *node) {
    if constexpr(!std::is_trivially_destructible_v<value_type>) {
        for(field_type i = 0; i < node->size(); ++i)
            node->destroy_value(i);
    }
}

template <class Params>
inline void WTreeNodeManager<Params>::delete_leaf_node(node_type *node) {
    internal_allocator_type &ia = mutable_allocator();
    destroy_values(node);
    const size_t node_size =
        kBasefieldsBytes + (node->capacity() * sizeof(value_type));
    internal_allocator_traits::deallocate(ia, reinterpret_cast<char *>(node),
                                          node_size);
}

template <class Params>
inline void WTreeNodeManager<Params>::delete_internal_node(node_type *node) {
    internal_allocator_type &ia = mutable_allocator();
    destroy_values(node);
    internal_allocator_traits::deallocate(ia, reinterpret_cast<char *>(node),
                                          sizeof(internal_fields_type));
}

template <class Params>
inline void
WTreeNodeManager<Params>::internal_destroy_child_unchecked(node_type *parent,
                                                           field_type index) {
    assert(parent->is_internal());
    assert(index < parent->size());
    assert(parent->child(index) != nullptr);

    node_type *child_node = parent->child(index);
    if(child_node->is_internal())
        delete_internal_node(child_node);
    else
        delete_leaf_node(child_node);

    parent->set_child(index, nullptr);
}

template <class Params>
void WTreeNodeManager<Params>::internal_recursive_delete(node_type *u) {
    assert(u != nullptr); // Reaching here  with a nullptr may mean a double
                          // free occurred.
    if(u->is_internal()) {
        for(field_type i = 0; i < kTargetK - 1; ++i) {
            if(u->child(i) != nullptr)
                internal_recursive_delete(u->child(i));
        }
        delete_internal_node(u);
    } else {
        delete_leaf_node(u);
    }
}

// === Value relocation ===

template <class Params>
inline void WTreeNodeManager<Params>::move_values_to_blank_node(
    node_type *src, node_type *dest, field_type count, field_type dest_offset) {
    if constexpr(std::is_trivially_copyable_v<value_type>) {
        std::memcpy(static_cast<void *>(dest->fields.values + dest_offset),
                    static_cast<const void *>(src->fields.values),
                    count * sizeof(value_type));
    } else if constexpr(std::is_move_constructible_v<value_type>) {
        std::uninitialized_move_n(src->fields.values, count,
                                  dest->fields.values + dest_offset);
    } else {
        std::uninitialized_copy_n(src->fields.values, count,
                                  dest->fields.values + dest_offset);
    }
}

template <class Params>
inline void WTreeNodeManager<Params>::move_values_to_blank_node(
    value_type *first, value_type *last, value_type *dest) {
    const size_t count = static_cast<size_t>(last - first);
    if(count == 0)
        return;

    // Blank-destination contract: dest is disjoint from the source range
    // (and both live in disjoint node buffers).
    assert(dest >= last || dest + count <= first);

    if constexpr(std::is_trivially_copyable_v<value_type>) {
        std::memcpy(static_cast<void *>(dest), static_cast<const void *>(first),
                    count * sizeof(value_type));
    } else if constexpr(std::is_move_constructible_v<value_type>) {
        for(size_t i = 0; i < count; ++i) {
            new(static_cast<void *>(&dest[i])) value_type(std::move(first[i]));
            first[i].~value_type();
        }
    } else {
        for(size_t i = 0; i < count; ++i) {
            new(static_cast<void *>(&dest[i])) value_type(first[i]);
            first[i].~value_type();
        }
    }
}

template <class Params>
inline void WTreeNodeManager<Params>::move_values_to_node(value_type *first,
                                                         value_type *last,
                                                         value_type *dest) {
    const size_t count = static_cast<size_t>(last - first);
    if(count == 0 || dest == first)
        return;

    if constexpr(std::is_trivially_copyable_v<value_type>) {
        std::memmove(static_cast<void *>(dest), static_cast<const void *>(first),
                     count * sizeof(value_type));
        return;
    }

    if(dest < first) {
        // Left shift: destination start C is not in (A+1, B] (C <= first), so
        // the forward pass never overwrites a not-yet-moved source.
        assert(dest < first);
        for(size_t i = 0; i < count; ++i) {
            if constexpr(std::is_move_constructible_v<value_type>) {
                new(static_cast<void *>(&dest[i]))
                    value_type(std::move(first[i]));
            } else {
                new(static_cast<void *>(&dest[i])) value_type(first[i]);
            }
            first[i].~value_type();
        }
    } else {
        // Right shift: dest > first implies dest_last = dest + count > last,
        // so the destination end D is not in (A, B-1]; the backward pass
        // consumes each source before its slot is re-constructed.
        assert(dest > first);
        for(size_t i = count; i > 0; --i) {
            if constexpr(std::is_move_constructible_v<value_type>) {
                new(static_cast<void *>(&dest[i - 1]))
                    value_type(std::move(first[i - 1]));
            } else {
                new(static_cast<void *>(&dest[i - 1])) value_type(first[i - 1]);
            }
            first[i - 1].~value_type();
        }
    }
}

// === Emplace ===

template <class Params>
template <typename IterType, typename... Args>
inline IterType WTreeNodeManager<Params>::internal_emplace_at(IterType &hint,
                                                              Args &&...args) {
    hint.node->internal_emplace_as_leaf(hint.index,
                                        std::forward<Args>(args)...);
    increment_size();
    return hint;
}

template <class Params>
template <typename... Args>
inline typename WTreeNodeManager<Params>::iterator
WTreeNodeManager<Params>::internal_emplace_at_with_growth(iterator &it,
                                                          Args &&...args) {
    assert(it.node != nullptr);
    assert(it.node->size() < kTargetK);

    if(it.node->size() == it.node->capacity()) {
        // Root never grows.
        assert(it.has_ascendant());

        it.node = grow_leaf(it.node);
        it.ascendant()->set_child(it.position(), it.node);
    }
    return internal_emplace_at(it, std::forward<Args>(args)...);
}

// === Balance: slide/split on a full leaf ===

template <typename Params>
template <typename... Args>
void WTreeNodeManager<Params>::internal_balanced_slide_to_left_unchecked(
    iterator &it, Args &&...args) {
    node_type *p = it.ascendant();
    const field_type pi = it.position();
    node_type *sibling = p->child(pi - 1);
    auto *const node_vals = it.node->fields.values;

    // Node must remain with this size:
    const field_type target_size = (kTargetK >> 1) + (sibling->size() >> 1) + 1;
    // From mid (excluded) to left, all values must be moved outside the
    // node.
    const field_type mid = kTargetK - target_size;
    const field_type sib_old_size = sibling->fields.size;

    // Prepare left sibling: grow, append old parent pivot to end.
    // No shift needed. Existing values stay at [0, sib_old_size).
    sibling = grow_leaf_to_size(sibling, sib_old_size + mid + 1);
    p->fields.pointers[pi - 1] = sibling;
    p->move_value(pi, sibling, sib_old_size);

    // Easy case that keeps it.node untouched.
    // Only one key is moved.
    if(mid == 0 && it.index == 0) {
        p->construct_value(pi, std::forward<Args>(args)...);
        it.ascend();
        ++(sibling->fields.size);
        return;
    }

    if(it.index == mid) {
        // Key becomes the new parent pivot.
        move_values_to_blank_node(node_vals, node_vals + mid,
                                  sibling->fields.values + sib_old_size + 1);
        sibling->fields.size = sib_old_size + mid + 1;

        move_values_to_node(node_vals + mid, node_vals + kTargetK, node_vals);
        it.node->fields.size = kTargetK - mid;

        p->construct_value(pi, std::forward<Args>(args)...);
        it.ascend();
        assert(it.index == pi);
        return;
    }

    if(it.index < mid) {
        // Key goes to left sibling v.
        // Elements for v: u[0..idx-1], key, u[idx..mid-2]. Pivot =
        // u[mid-1].
        move_values_to_blank_node(node_vals, node_vals + it.index,
                                  sibling->fields.values + sib_old_size + 1);
        const field_type newidx = sib_old_size + 1 + it.index;
        move_values_to_blank_node(node_vals + it.index, node_vals + mid - 1,
                                  sibling->fields.values + newidx + 1);
        sibling->construct_value(newidx, std::forward<Args>(args)...);

        it.node->move_value(mid - 1, p, pi);
        move_values_to_node(node_vals + mid, node_vals + kTargetK, node_vals);
        sibling->fields.size = sib_old_size + 1 + mid;
        it.node->fields.size = target_size;
        it.ascend();
        it.descend(pi - 1);
        it.index = newidx;
        return;
    }

    // Key stays in u (it.index > mid).
    // Elements for v: u[0..mid-1]. Pivot = u[mid].
    move_values_to_blank_node(node_vals, node_vals + mid,
                              sibling->fields.values + sib_old_size + 1);
    it.node->move_value(mid, p, pi);

    // Shift elements u[mid+1..kTargetK-1] and create key:
    move_values_to_node(node_vals + mid + 1, node_vals + it.index, node_vals);
    it.node->construct_value(it.index - mid - 1, std::forward<Args>(args)...);
    move_values_to_node(node_vals + it.index, node_vals + kTargetK,
                        node_vals + it.index - mid);
    it.index -= mid + 1;
    it.node->fields.size = target_size;
    sibling->fields.size = sib_old_size + mid + 1;
}

template <typename Params>
template <typename... Args>
void WTreeNodeManager<Params>::internal_balanced_slide_to_right_unchecked(
    iterator &it, Args &&...args) {
    node_type *p = it.ascendant();
    const field_type pi = it.position();
    node_type *sibling = p->child(pi + 1);
    auto *const node_vals = it.node->fields.values;

    const field_type mid = (kTargetK >> 1) + (sibling->size() >> 1) + 1;
    const field_type sib_newsize = kTargetK - (mid - 1) + sibling->size();

    // Prepare right sibling: grow + shift old contents right, place old
    // parent pivot. When reallocation occurs, values land directly at the
    // offset.
    sibling = grow_leaf_and_shift_right(sibling, sib_newsize);
    p->set_child(pi + 1, sibling);
    p->move_value(pi + 1, sibling, kTargetK - mid);
    sibling->fields.size = sib_newsize;
    it.node->fields.size = mid;

    // Easy case that keeps it.node untouched.
    // Only one key is moved.
    if(mid == kTargetK && it.index == kTargetK) {
        p->construct_value(pi + 1, std::forward<Args>(args)...);
        it.ascend();
        ++it.index;
        return;
    }

    if(it.index == mid) {
        // Key becomes the new parent pivot.
        p->construct_value(pi + 1, std::forward<Args>(args)...);
        move_values_to_blank_node(node_vals + mid, node_vals + kTargetK,
                                  sibling->fields.values);
        it.ascend();
        ++it.index;
        assert(it.index == pi + 1);
        return;
    }

    if(it.index > mid) {
        // Key goes to right sibling v.
        // p[pi+1] gets u[right] (insertion past the split, no shift in u).
        it.node->move_value(mid, p, pi + 1);
        move_values_to_blank_node(node_vals + mid + 1, node_vals + it.index,
                                  sibling->fields.values);
        const field_type newpos = it.index - mid - 1;
        sibling->construct_value(newpos, std::forward<Args>(args)...);
        move_values_to_blank_node(node_vals + it.index, node_vals + kTargetK,
                                  sibling->fields.values + newpos + 1);
        it.ascend();
        it.descend(pi + 1);
        it.index = newpos;
        return;
    }

    // Key stays in u (it.index < mid).
    // Pivot = u[mid-1] (insertion shifts logical positions mid).
    move_values_to_blank_node(node_vals + mid, node_vals + kTargetK,
                              sibling->fields.values);
    it.node->move_value(mid - 1, p, pi + 1);
    --it.node->fields.size;
    it.node->internal_emplace_as_leaf(it.index, std::forward<Args>(args)...);
}

template <typename Params>
template <typename... Args>
void WTreeNodeManager<Params>::internal_split_to_left_unchecked(
    iterator &it, Args &&...args) {
    node_type *p = it.ascendant();
    const field_type pi = it.position();
    auto *const node_vals = it.node->fields.values;

    const field_type target_size = (kTargetK >> 1) + 1;
    // From mid (excluded) to left, all values must be moved outside the
    // node.
    const field_type mid = kTargetK - target_size;

    node_type *sibling = new_leaf_node(mid + 1);
    sibling->fields.size = mid + 1;
    p->set_child(pi - 1, sibling);
    p->move_value(pi, sibling, 0);
    it.node->fields.size = target_size;

    // Lateral shift avoids this case.
    assert(mid > 0);

    if(it.index == mid) {
        // New key becomes the pivot.
        move_values_to_blank_node(node_vals, node_vals + mid,
                                  sibling->fields.values + 1);
        move_values_to_node(node_vals + mid, node_vals + kTargetK, node_vals);
        p->construct_value(pi, std::forward<Args>(args)...);
        it.ascend();
        assert(it.index == pi);
        return;
    }

    if(it.index < mid) {
        // New key goes to left sibling.
        // Elements for sibling: u[0..idx-1], new_key, u[idx..mid-2].
        // Pivot = u[mid-1], moved up to parent[pi].
        move_values_to_blank_node(node_vals, node_vals + it.index,
                                  sibling->fields.values + 1);
        const field_type newidx = it.index + 1;
        sibling->construct_value(newidx, std::forward<Args>(args)...);
        move_values_to_blank_node(node_vals + it.index, node_vals + mid - 1,
                                  sibling->fields.values + newidx + 1);

        it.node->move_value(mid - 1, p, pi);
        move_values_to_node(node_vals + mid, node_vals + kTargetK, node_vals);
        it.ascend();
        it.descend(pi - 1);
        it.index = newidx;
        return;
    }

    // it.index > mid: new key stays in u.
    // Elements for sibling: u[0..mid-1]. Pivot = u[mid], moved up to
    // parent[pi].
    move_values_to_blank_node(node_vals, node_vals + mid,
                              sibling->fields.values + 1);
    it.node->move_value(mid, p, pi);

    move_values_to_node(node_vals + mid + 1, node_vals + it.index, node_vals);
    it.node->construct_value(it.index - mid - 1, std::forward<Args>(args)...);
    move_values_to_node(node_vals + it.index, node_vals + kTargetK,
                        node_vals + it.index - mid);
    it.index -= mid + 1;
}

template <typename Params>
template <typename... Args>
void WTreeNodeManager<Params>::internal_split_to_right_unchecked(
    iterator &it, Args &&...args) {
    node_type *p = it.ascendant();
    const field_type pi = it.position();
    auto *const node_vals = it.node->fields.values;

    // Keep this number of keys:
    const field_type mid = (kTargetK >> 1) + 1;
    // Include new key in the sum for sibling.
    const field_type sib_newsize = kTargetK - mid + 1;

    assert(mid < kTargetK);

    node_type *sibling = new_leaf_node(sib_newsize);
    sibling->fields.size = sib_newsize;
    p->set_child(pi + 1, sibling);
    p->move_value(pi + 1, sibling, sib_newsize - 1);
    it.node->fields.size = mid;

    if(it.index == mid) {
        // New key becomes the pivot.
        p->construct_value(pi + 1, std::forward<Args>(args)...);
        move_values_to_blank_node(node_vals + mid, node_vals + kTargetK,
                                  sibling->fields.values);
        it.ascend();
        ++it.index;
        assert(it.index == pi + 1);
        return;
    }

    if(it.index > mid) {
        // New key goes into the right sibling.
        // Pivot = u[mid], moved up to parent[pi+1].
        it.node->move_value(mid, p, pi + 1);
        move_values_to_blank_node(node_vals + mid + 1, node_vals + it.index,
                                  sibling->fields.values);
        const field_type newpos = it.index - mid - 1;
        sibling->construct_value(newpos, std::forward<Args>(args)...);
        move_values_to_blank_node(node_vals + it.index, node_vals + kTargetK,
                                  sibling->fields.values + newpos + 1);
        it.ascend();
        it.descend(pi + 1);
        it.index = newpos;
        return;
    }

    // it.index < mid: new key stays in u.
    // Pivot = u[mid-1], moved up to parent[pi+1].
    it.node->move_value(mid - 1, p, pi + 1);
    move_values_to_blank_node(node_vals + mid, node_vals + kTargetK,
                              sibling->fields.values);
    --it.node->fields.size;
    it.node->internal_emplace_as_leaf(it.index, std::forward<Args>(args)...);
}

template <typename Params>
template <typename... Args>
bool WTreeNodeManager<Params>::attempt_emplace_with_balanced_slide(
    iterator &it, Args &&...args) noexcept {
    assert(it.node != nullptr);
    assert(it.node->is_leaf());
    assert(it.has_ascendant());

    const node_type *p = it.ascendant();
    assert(p->is_internal());

    const field_type pi = it.position();

    if(pi < kTargetK - 2 && p->child(pi + 1) != nullptr &&
       p->child(pi + 1)->size() < kTargetK) {
        internal_balanced_slide_to_right_unchecked(it,
                                                   std::forward<Args>(args)...);
        return true;
    }

    if(pi > 0 && p->child(pi - 1) != nullptr &&
       p->child(pi - 1)->size() < kTargetK) {
        internal_balanced_slide_to_left_unchecked(it,
                                                  std::forward<Args>(args)...);
        return true;
    }

    return false;
}

template <typename Params>
template <typename... Args>
bool WTreeNodeManager<Params>::attempt_emplace_with_balanced_split(
    iterator &it, Args &&...args) noexcept {

    assert(it.node != nullptr);
    assert(it.node->is_leaf());
    assert(it.has_ascendant());

    assert(WTreeRulesAssumptions::split_to_right_first);

    const node_type *p = it.ascendant();
    assert(p->is_internal());

    const field_type pi = it.position();

    if(pi < kTargetK - 2) {
        const node_type *right = p->child(pi + 1);
        if(right == nullptr) {
            internal_split_to_right_unchecked(it, std::forward<Args>(args)...);
            return true;
        }
    }

    if(pi > 0) {
        const node_type *left = p->child(pi - 1);
        if(left == nullptr) {
            internal_split_to_left_unchecked(it, std::forward<Args>(args)...);
            return true;
        }
    }

    return false;
}

template <typename Params>
template <typename... Args>
bool WTreeNodeManager<Params>::attempt_emplace_with_balanced_slide_and_split(
    iterator &it, Args &&...args) noexcept {
    assert(it.node != nullptr);
    assert(it.node->is_leaf());
    assert(it.has_ascendant());

    assert(WTreeRulesAssumptions::slide_over_split);
    assert(WTreeRulesAssumptions::slide_to_right_first);
    assert(WTreeRulesAssumptions::split_to_right_first);

    const node_type *p = it.ascendant();
    assert(p->is_internal());

    const field_type pi = it.position();

    if(pi < kTargetK - 2) {
        const node_type *right = p->child(pi + 1);
        if(right == nullptr) {
            internal_split_to_right_unchecked(it, std::forward<Args>(args)...);
            return true;
        }
        if(right->size() < kTargetK) {
            internal_balanced_slide_to_right_unchecked(
                it, std::forward<Args>(args)...);
            return true;
        }
    }

    if(pi > 0) {
        const node_type *left = p->child(pi - 1);
        if(left == nullptr) {
            internal_split_to_left_unchecked(it, std::forward<Args>(args)...);
            return true;
        }
        if(left->size() < kTargetK) {
            internal_balanced_slide_to_left_unchecked(
                it, std::forward<Args>(args)...);
            return true;
        }
    }

    return false;
}

// === Boundary-value ascent (erase) ===

template <typename Params>
void WTreeNodeManager<Params>::internal_move_greatest_upward(iterator &iter) {
    assert(iter.node->size() > 0);

    if(!iter.has_ascendant()) {
        iter.node->destroy_value(iter.node->size() - 1);
        --iter.node->fields.size;
        // Note that [iter.node] may be empty, and must be destroyed
        // from outside this function.
        return;
    }
    int level = 0;
    for(; level < iter.path_size() - 1; ++level) {
        node_type *node = iter.ascendants[level];
        const field_type pos = iter.move_indexes[level];
        const field_type src_end =
            std::min<field_type>(node->fields.size, kTargetK - 1);
        if(node->fields.size == kTargetK)
            node->destroy_value(kTargetK - 1);
        move_values_to_node(node->fields.values + pos + 1,
                            node->fields.values + src_end,
                            node->fields.values + pos + 2);

        iter.ascendants[level + 1]->move_value(
            iter.ascendants[level + 1]->size() - 1, node, pos + 1);
    }
    assert(iter.node != root());
    assert(iter.node->is_leaf());

    node_type *const asc = iter.ascendant();
    const field_type src_end =
        std::min<field_type>(asc->fields.size, kTargetK - 1);
    if(asc->fields.size == kTargetK)
        asc->destroy_value(kTargetK - 1);
    move_values_to_node(asc->fields.values + iter.position() + 1,
                        asc->fields.values + src_end,
                        asc->fields.values + iter.position() + 2);
    iter.node->move_value(iter.node->size() - 1, iter.ascendant(),
                          iter.position() + 1);
    if(iter.node->size() == 1) {
        internal_destroy_child_unchecked(iter.ascendant(), iter.position());
        iter.ascend();
        return;
    }
    --iter.node->fields.size;
}

template <typename Params>
void WTreeNodeManager<Params>::internal_move_smallest_upward(iterator &iter) {
    assert(iter.node->size() > 0);

    if(!iter.has_ascendant()) {
        // Shift elements left by 1: [1, size) -> [0, size - 1). The erased
        // smallest value lived at index 0.
        iter.node->destroy_value(0);
        move_values_to_node(iter.node->fields.values + 1,
                            iter.node->fields.values + iter.node->size(),
                            iter.node->fields.values);
        --iter.node->fields.size;
        // Note that [iter.node] may be empty, and must be destroyed
        // from outside this functions.
        return;
    }
    int level = 0;
    for(; level < iter.path_size() - 1; ++level) {
        node_type *node = iter.ascendants[level];
        const field_type pos = iter.move_indexes[level];
        // Shift elements left by 1: [1, pos+1) -> [0, pos). The leftmost
        // boundary key of the node is the displaced one.
        node->destroy_value(0);
        move_values_to_node(node->fields.values + 1,
                            node->fields.values + pos + 1,
                            node->fields.values);

        iter.ascendants[level + 1]->move_value(0, node, pos);
    }
    assert(iter.node != root());
    assert(iter.node->is_leaf());

    iter.ascendant()->destroy_value(0);
    move_values_to_node(iter.ascendant()->fields.values + 1,
                        iter.ascendant()->fields.values +
                            iter.position() + 1,
                        iter.ascendant()->fields.values);
    iter.node->move_value(0, iter.ascendant(), iter.position());
    if(iter.node->size() == 1) {
        internal_destroy_child_unchecked(iter.ascendant(), iter.position());
        iter.ascend();
        return;
    }
    iter.node->destroy_value(0);
    move_values_to_node(iter.node->fields.values + 1,
                        iter.node->fields.values + iter.node->size(),
                        iter.node->fields.values);
    --iter.node->fields.size;
}

// === Edge insertion at a full internal node ===

template <class Params>
template <typename... Args>
inline typename WTreeNodeManager<Params>::iterator
WTreeNodeManager<Params>::emplace_edge_swap(iterator &it, Args &&...args) {
    assert(it.node->is_internal());
    assert(it.node->size() == kTargetK);
    assert(it.index == 0 || it.index == kTargetK);

    const field_type swap_idx = (it.index == 0) ? 0 : kTargetK - 1;
    const field_type child_idx = (it.index == 0) ? 0 : kTargetK - 2;

    // Phase 1: Descend while boundary child exists and is full internal.
    int levels = 0;
    while(it.node->child(child_idx) != nullptr) {
        auto *child = it.node->child(child_idx);
        if(child->size() < kTargetK)
            break;
        if(child->is_leaf())
            break;
        it.descend(child_idx);
        ++levels;
    }

    // Phase 2: Handle the bottom — make space for the last displaced key.
    // For the root, first child could be nullptr.
    node_type *child = it.node->child(child_idx);
    if(child != nullptr) {
        if(child->size() < kTargetK) {
            // Leaf child has space — insert directly.
            // it.node->move_value(swap_idx, child, swap_idx);
            if(swap_idx == 0) {
                it.descend(0);
            } else {
                it.descend_to_right_side(child_idx);
                ++it.index;
            }
            internal_emplace_at_with_growth(
                it, std::move(it.ascendant()->fields.values[swap_idx]));
            decrement_size(); // Correction for later addition.
            it.ascend();
            it.node->destroy_value(swap_idx);
        } else {
            // Full leaf child.

            if(swap_idx == 0) {
                it.descend(0);
            } else {
                it.descend_to_right_side(child_idx);
                ++it.index;
            }

            bool correct_attempt = false;
            if constexpr(params_type::use_slide && params_type::use_split) {
                correct_attempt = attempt_emplace_with_balanced_slide_and_split(
                    it, std::move(it.ascendant()->fields.values[swap_idx]));
            } else if constexpr(params_type::use_slide) {
                correct_attempt = attempt_emplace_with_balanced_slide(
                    it, std::move(it.ascendant()->fields.values[swap_idx]));
            } else if constexpr(params_type::use_split) {
                correct_attempt = attempt_emplace_with_balanced_split(
                    it, std::move(it.ascendant()->fields.values[swap_idx]));
            }
            // If not possible, convert to internal and create child below.

            if(correct_attempt) {
                it.ascend();
                it.node->destroy_value(swap_idx);
            } else {
                ++levels;
                make_child_internal_unchecked(it.ascendant(), it.position());
                it.internal_update_position();
                node_type *leaf = new_leaf_node(kInitialCapacity);
                it.node->set_child(child_idx, leaf);
                it.node->move_value(swap_idx, leaf, 0);
                leaf->fields.size = 1;
            }
        }

    } else {
        assert(it.node->is_internal());
        // No child — create new leaf.
        node_type *leaf = new_leaf_node(kInitialCapacity);
        it.node->set_child(child_idx, leaf);
        it.node->move_value(swap_idx, leaf, 0);
        leaf->fields.size = 1;
    }

    // Phase 3: Pull keys down through intermediate levels.
    for(; levels > 0; --levels) {
        node_type *parent = it.ascendant();
        parent->move_value(swap_idx, it.node, swap_idx);
        it.ascend();
    }

    // Phase 4: Construct the original value at the top.
    it.node->construct_value(swap_idx, std::forward<Args>(args)...);
    it.index = swap_idx;
    increment_size();
    return it;
}

} // namespace WTreeLib

#endif