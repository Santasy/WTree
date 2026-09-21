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

#ifndef _WTREE_MANAGER__H_
#define _WTREE_MANAGER__H_

#include "traits.hpp"
#include "type_aliases.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/types.h>
#include <type_traits>

namespace WTreeLib {

/**
 * @brief Owns node storage and the balance operations (slide/split),
 * coordinating node growth, shifts, and rewrites.
 * @tparam Params A @ref WTree "WTree<Params>" instantiation.
 */
template <class Params>
class WTreeNodeManager : protected WTreeTypeAliases<Params> {
    typedef WTreeNodeManager<Params> self_type;
    using Aliases = WTreeTypeAliases<Params>;

  public:
    using typename Aliases::allocator_traits;
    using typename Aliases::allocator_type;
    using typename Aliases::base_fields_type;
    using typename Aliases::const_iterator;
    using typename Aliases::const_pointer;
    using typename Aliases::const_reference;
    using typename Aliases::const_reverse_iterator;
    using typename Aliases::data_type;
    using typename Aliases::field_type;
    using typename Aliases::internal_allocator_traits;
    using typename Aliases::internal_allocator_type;
    using typename Aliases::internal_fields_type;
    using typename Aliases::iterator;
    using typename Aliases::key_compare;
    using typename Aliases::key_type;
    using typename Aliases::leaf_fields_type;
    using typename Aliases::mapped_type;
    using typename Aliases::node_type;
    using typename Aliases::params_type;
    using typename Aliases::pointer;
    using typename Aliases::reference;
    using typename Aliases::reverse_iterator;
    using typename Aliases::size_type;
    using typename Aliases::value_type;

    using Aliases::kBasefieldsBytes;
    using Aliases::kInitialCapacity;
    using Aliases::kLastGrowth;
    using Aliases::kTargetK;
    using Aliases::kTargetNodeBytes;

    template <typename> friend class WTree;
    template <typename> friend class ModifiableWTree;

    // A helper class to get the empty base class optimization for 0-size
    // allocators. Base is internal_allocator_type.
    // (e.g. empty_base_handle<internal_allocator_type, node_type*>). If Base is
    // 0-size, the compiler doesn't have to reserve any space for it and
    // sizeof(empty_base_handle) will simply be sizeof(Data). Google [empty base
    // class optimization] for more details.
    template <typename Base, typename Data>
    struct EmptyBaseNodeHandle : public Base {
        EmptyBaseNodeHandle(const Base &b, const Data &d) : Base(b), data(d) {}
        Data data;
    };

    WTreeNodeManager(const internal_allocator_type &alloc)
        : m_root(alloc, nullptr), m_size(0) {};

  protected:
    EmptyBaseNodeHandle<internal_allocator_type, node_type *> m_root;
    size_type m_size = 0;

    // Internal accessor routines.
    node_type *root() noexcept { return m_root.data; };
    const node_type *croot() const noexcept { return m_root.data; };
    node_type *&mutable_root() noexcept { return m_root.data; };

    size_type size() const noexcept { return m_size; };
    void increment_size() noexcept { ++m_size; };
    void increase_size_by(size_type amount) noexcept { m_size += amount; };
    void decrement_size() noexcept { --m_size; };
    void decrease_size_by(size_type amount) noexcept {
        assert(amount <= size());
        m_size -= amount;
    };

    void set_size(size_t s) noexcept { m_size = s; };
    void set_root(node_type *node) noexcept { m_root.data = node; };

    // Next capacity for a leaf node: SAFE_NEW_SIZE for uint8_t field_type
    // avoids overflow when computing the new capacity.
    static constexpr field_type next_leaf_capacity(field_type current_size) {
        if constexpr(std::is_same_v<field_type, uint8_t>) {
            return SAFE_NEW_SIZE<field_type, kLastGrowth, kTargetK>(
                current_size);
        } else if constexpr(std::is_same_v<field_type, ushort>) {
            return std::min<field_type>(NEW_SIZE<field_type>(current_size),
                                        kTargetK);
        } else {
            static_assert(std::is_same_v<field_type, uint16_t> ||
                              std::is_same_v<field_type, uint8_t>,
                          "Unsupported field_type");
        }
    }

  public:
    internal_allocator_type &mutable_allocator() noexcept {
        return *static_cast<internal_allocator_type *>(&m_root);
    };
    const internal_allocator_type &internal_allocator() const noexcept {
        return *static_cast<const internal_allocator_type *>(&m_root);
    };

    node_type *new_leaf_node(field_type capacity);
    node_type *new_internal_node();

    node_type *grow_leaf(node_type *node);

    /**
     * @brief Grows a leaf and shifts existing values to the right end.
     * @details When reallocation is needed, values land directly at the
     * target offset, avoiding a separate backward-shift pass.
     *
     * @param new_size Target layout size; existing values end up at
     *                 [new_size - old_size, new_size).
     * @param node     The leaf node being grown.
     * @return The possibly reallocated node — callers must reset the parent
     *         child pointer.
     */
    node_type *grow_leaf_and_shift_right(node_type *node, field_type new_size);

    node_type *grow_leaf_to_size(node_type *node, field_type min_size);

    /** @brief Also deletes the replaced leaf node.
     */
    node_type *make_internal(node_type *node);

    /** @brief Also deletes the replaced leaf node.
     */
    node_type *make_leaf_from_internal(node_type *node);

    void make_child_internal_unchecked(node_type *parent, field_type i);
    void make_child_leaf_unchecked(node_type *parent, field_type i);

    void delete_leaf_node(node_type *node);
    void delete_internal_node(node_type *node);

    void internal_destroy_child_unchecked(node_type *parent, field_type index);

    // Erase the tree visiting childrens recursively.
    void internal_recursive_delete(node_type *u);

    // === Value relocation ===
    //
    // Two mover families with complementary destination contracts:
    //  - move_values_to_blank_node: destination bytes are non-valid
    //    (never-constructed or already destroyed), so values are simply
    //    placement-moved in — there is never a destroy at the destination.
    //  - move_values_to_node: relocation inside live node storage that may
    //    overlap. The traversal direction derives from dest vs first, so no
    //    not-yet-moved source is ever overwritten (no-erase-data rules).

    // Move count values from src into dest at dest_offset. The destination
    // region must be blank; source values end up moved-from (the caller
    // destroys the source node afterward).
    void move_values_to_blank_node(node_type *src, node_type *dest,
                                   field_type count,
                                   field_type dest_offset = 0);

    // Pointer-range relocation into a blank region disjoint from the source
    // range. Constructs at dest and destroys every source element.
    void move_values_to_blank_node(value_type *first, value_type *last,
                                   value_type *dest);

    // Pointer-range relocation inside live node storage that may overlap.
    // Constructs at dest and destroys every source element.
    void move_values_to_node(value_type *first, value_type *last,
                             value_type *dest);

    /** @brief Also destroys an empty leaf when moving the greatest leaf
     * key-value.
     */
    void internal_move_greatest_upward(iterator &iter);

    /** @brief Also destroys an empty leaf when moving the greatest leaf
     * key-value.
     */
    void internal_move_smallest_upward(iterator &iter);

    template <typename IterType, typename... Args>
    inline IterType internal_emplace_at(IterType &hint, Args &&...args);

    /**
     * @brief Emplaces exactly at the iterator's position, using the node as
     * a leaf without further validation and growing it if necessary.
     *
     * @return Iterator to the emplaced element.
     */
    template <typename... Args>
    iterator internal_emplace_at_with_growth(iterator &it, Args &&...args);

    /**
     * @brief Slides elements from full leaf node to its right sibling v.
     *
     * The logical (K+1)-element sorted array is:
     *   L[] = [u0, ..., u_{idx-1}, new_key, u_{idx}, ..., u_{K-1}]
     * Split at position `mid`:
     *   node keeps    L[0..mid-1]       (mid elements)
     *   pivot    =    L[mid]            (1 element → parent at pi+1)
     *   sibling gets  L[mid+1..K]       (K - mid elements, prepended to v)
     *
     * @param it  Iterator at the full leaf. Updated to the inserted position.
     * @param args  Arguments forwarded to construct the new element.
     */
    template <typename... Args>
    void internal_balanced_slide_to_right_unchecked(iterator &it,
                                                    Args &&...args);

    /**
     * @brief Slides elements from full leaf node to its left sibling.
     *
     * The logical (K+1)-element sorted array is:
     *   L[] = [u0, ..., u_{idx-1}, new_key, u_{idx}, ..., u_{K-1}]
     * Split at position `mid`:
     *   sibling gets  L[0..mid-1]       (mid elements, appended to v)
     *   pivot      =  L[mid]            (1 element → parent at pi)
     *   node keeps    L[mid+1..K]       (K - mid elements)
     *
     * @param it  Iterator at the full leaf. Updated to the inserted position.
     * @param args  Arguments forwarded to construct the new element.
     */
    template <typename... Args>
    void internal_balanced_slide_to_left_unchecked(iterator &it,
                                                   Args &&...args);

    /**
     * @brief Splits a full leaf by creating a new right sibling that does
     * not yet exist.
     *
     * The logical (K+1)-element sorted array is:
     *   L[] = [u0, ..., u_{idx-1}, new_key, u_{idx}, ..., u_{K-1}]
     * Split at position `mid`:
     *   node keeps    L[0..mid-1]       (mid elements)
     *   pivot    =    L[mid]            (1 element → parent at pi+1)
     *   sibling gets  L[mid+1..K]       (K - mid elements)
     *
     * Unlike the slide variant, the right sibling is freshly created (size 0),
     * so no existing sibling elements are moved and the parent gains a new
     * child/key slot at pi+1.
     *
     * @param it  Iterator at the full leaf. Updated to the inserted position.
     * @param args  Arguments forwarded to construct the new element.
     */
    template <typename... Args>
    void internal_split_to_right_unchecked(iterator &it, Args &&...args);

    /**
     * @brief Splits a full leaf by creating a new left sibling that does
     * not yet exist.
     *
     * The logical (K+1)-element sorted array is:
     *   L[] = [u0, ..., u_{idx-1}, new_key, u_{idx}, ..., u_{K-1}]
     * Split at position `mid`:
     *   sibling gets  L[0..mid-1]       (mid elements)
     *   pivot      =  L[mid]            (1 element → parent at pi)
     *   node keeps    L[mid+1..K]       (K - mid elements)
     *
     * Unlike the slide variant, the left sibling is freshly created (size 0),
     * so no existing sibling elements are moved and the parent gains a new
     * child/key slot at pi (current node shifts to pi+1).
     *
     * @param it  Iterator at the full leaf. Updated to the inserted position.
     * @param args  Arguments forwarded to construct the new element.
     */
    template <typename... Args>
    void internal_split_to_left_unchecked(iterator &it, Args &&...args);

    /**
     * @brief Balanced slide into a sibling node.
     * @details Given a full leaf u with insertion position it.index
     * (0..kTargetK inclusive), tries to slide elements to a non-full
     * adjacent sibling v, distributing the (kTargetK + 1) logical elements
     * (u's keys + new key) across u, a parent pivot, and v.
     */
    template <typename... Args>
    bool attempt_emplace_with_balanced_slide(iterator &it,
                                             Args &&...args) noexcept;

    template <typename... Args>
    bool attempt_emplace_with_balanced_split(iterator &it,
                                             Args &&...args) noexcept;

    /**
     * @brief Attempts to apply a balanced slide or split into a sibling node.
     * @details See the implementation for the specific sequence of attempts.
     */
    template <typename... Args>
    bool attempt_emplace_with_balanced_slide_and_split(iterator &it,
                                                       Args &&...args) noexcept;

    // Handles insertion at the edge of a full internal node (index == 0 or
    // index == kTargetK). Displaces boundary keys downward following the
    // boundary child chain, then constructs the new value at the top.
    template <typename... Args>
    iterator emplace_edge_swap(iterator &it, Args &&...args);

  private:
    // Destroys the live values of [0, size()) for non-trivially-destructible
    // value types; a no-op otherwise.
    void destroy_values(node_type *node);

    // Allocates a new leaf of capacity new_cap and moves all current values so
    // they land at [dest_offset, dest_offset + size()); the old node is
    // destroyed. dest_offset == 0 keeps the values in ascending storage order.
    node_type *reallocate_leaf(node_type *node, field_type new_cap,
                               field_type dest_offset = 0);
};

} // namespace WTreeLib
#endif