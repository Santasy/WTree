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

#ifndef _WTREE_ITERATOR__H_
#define _WTREE_ITERATOR__H_

#include "node.hpp"
#include "traits.hpp"

#include <cassert>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace WTreeLib {
/**
 * @brief Bidirectional iterator over a WTree, carrying its ascendant path.
 * @tparam Node The node it points at (const or non-const).
 * @tparam Reference The reference type it yields.
 * @tparam Pointer The pointer type it yields.
 */
template <typename Node, typename Reference, typename Pointer>
class WTreeIterator {
  public:
    typedef typename Node::key_type key_type;
    typedef typename Node::size_type size_type;
    typedef typename Node::difference_type difference_type;
    typedef typename Node::params_type params_type;

    typedef typename Node::field_type field_type;

    typedef Node node_type;
    typedef typename std::remove_const<Node>::type normal_node;
    typedef const Node const_node;
    typedef typename params_type::value_type value_type;
    typedef typename params_type::pointer normal_pointer;
    typedef typename params_type::reference normal_reference;
    typedef typename params_type::const_pointer const_pointer;
    typedef typename params_type::const_reference const_reference;

    typedef typename std::pair<field_type, normal_node *> path_vertex_type;

    typedef Pointer pointer;
    typedef Reference reference;
    typedef std::bidirectional_iterator_tag iterator_category;

    typedef WTreeIterator<normal_node, normal_reference, normal_pointer>
        iterator;
    typedef WTreeIterator<const_node, const_reference, const_pointer>
        const_iterator;
    typedef WTreeIterator<Node, Reference, Pointer> self_type;

    static_assert(
        std::numeric_limits<int>::max() >= params_type::kTargetK,
        "Index has no enough size for the last valid node's values index.");

    // The node in the tree the iterator is pointing at.
    normal_node *node;
    // The index within the node of the tree the iterator is pointing at.
    int index;
    // Ascendant path for search/iteration/remove routines. The first
    // kInlinePath levels live in the iterator itself so the common tree is
    // allocation-free; any deeper level spills into overflow_path. Depth is
    // unbounded in the worst case (in-order insertion degenerates the W-tree
    // into a near-linear chain, height ~ n/k), and the overflow vector keeps
    // that correct.
    static constexpr int kInlinePath = 4;
    path_vertex_type inline_path[kInlinePath];
    std::vector<path_vertex_type> overflow_path;
    int depth;

    template <typename> friend class WTree;
    template <typename> friend class WTreeNodeManager;
    template <typename> friend class WTreeNodeLocator;
    friend iterator;
    friend const_iterator;

    WTreeIterator() : node(nullptr), index(-1), overflow_path(), depth(0) {
#ifdef WTREE_DBG_ITERATORS
        std::cout << "Constructing iterator with no arguments.\n";
#endif
    }

    WTreeIterator(Node *n, int p)
        : node(const_cast<normal_node *>(n)), index(p), overflow_path(),
          depth(0) {
#ifdef WTREE_DBG_ITERATORS
        std::cout << "Constructing iterator with initial node and index.\n";
#endif
    }

    WTreeIterator(const WTreeIterator &) = default;
    WTreeIterator &operator=(const WTreeIterator &) = default;
    WTreeIterator(WTreeIterator &&) noexcept = default;
    WTreeIterator &operator=(WTreeIterator &&) noexcept = default;

    // This is the normal-to-const iterator conversion.
    template <typename OtherNode, typename OtherRef, typename OtherPtr>
    WTreeIterator(const WTreeIterator<OtherNode, OtherRef, OtherPtr> &other)
        : node(other.node), index(other.index),
          overflow_path(other.overflow_path), depth(other.depth) {
        for(int i = 0; i < depth && i < kInlinePath; ++i)
            inline_path[i] = other.inline_path[i];
        assert(stack_sizes_match());
#ifdef WTREE_DBG_ITERATORS
        std::cout << "Constructing iterator from another const iterator.\n";
#endif
    }

    int path_size() const { return depth; }

    // Ancestor at -level- (0 = root side of the recorded path) and the index
    // of the node below it.
    normal_node *ascendant_at(int level) const {
        assert(level >= 0 && level < depth);
        return level < kInlinePath ? inline_path[level].second
                                   : overflow_path[level - kInlinePath].second;
    }
    field_type move_index_at(int level) const {
        assert(level >= 0 && level < depth);
        return level < kInlinePath ? inline_path[level].first
                                   : overflow_path[level - kInlinePath].first;
    }

    bool is_valid() { return node != nullptr; }

    int position() const noexcept {
        return has_ascendant() ? static_cast<int>(move_index_at(depth - 1))
                               : -1;
    }
    bool has_ascendant() const noexcept { return depth > 0; }
    normal_node *ascendant() const noexcept {
        assert(has_ascendant());
        return ascendant_at(depth - 1);
    }

    normal_node *safe_ascendant() const noexcept {
        return has_ascendant() ? ascendant_at(depth - 1) : nullptr;
    }

    bool operator==(const const_iterator &x) const {
        return node == x.node && index == x.index;
    }
    bool operator!=(const const_iterator &x) const {
        return node != x.node || index != x.index;
    }
    bool operator==(const iterator &x) const {
        return node == x.node && index == x.index;
    }
    bool operator!=(const iterator &x) const {
        return node != x.node || index != x.index;
    }

    // Accessors for the key/value the iterator is pointing at.
    const key_type &key() const { return node->key(index); }
    reference operator*() const { return node->value(index); }
    pointer operator->() const { return &node->value(index); }

    self_type &operator++() {
        increment();
        return *this;
    }
    self_type &operator--() {
        decrement();
        return *this;
    }
    self_type operator++(int) {
        self_type tmp = *this;
        increment();
        return tmp;
    }
    self_type operator--(int) {
        self_type tmp = *this;
        decrement();
        return tmp;
    }

    inline void ascend() {
        assert(has_ascendant());
        node = ascendant();
        index = position();
        pop_stacks_unchecked();
    }

    void safe_ascend() noexcept {
        if(has_ascendant()) {
            ascend();
            return;
        }
    }

    // Descent occurs into the child at position [from].
    // See [descend_to_right_side(field_type from)] for the version that is used
    // finding the path to the greatest key in leaves.
    void descend(field_type from) {
        assert(node->child(from) != nullptr);

        push_path(from);
        node = node->child(from);

        index = 0;
    }

    void descend_to_right_side(field_type from) {
        assert(node->child(from) != nullptr);

        push_path(from);
        node = node->child(from);

        assert(node->size() > 0);
        index = node->size() - 1;
    }

    // Moves one position forward, as operator++ does.
    void step_forward() { ++(*this); }

  private:
    // Increment/decrement the iterator.
    void increment();
    void increment_by(size_t count);

    void decrement();
    void decrement_by(size_t count);

    void push_path(field_type from) {
        if(depth < kInlinePath)
            inline_path[depth] = path_vertex_type(from, node);
        else
            overflow_path.emplace_back(from, node);
        ++depth;
        assert(stack_sizes_match());
    }

    void pop_stacks_unchecked() {
        if(depth > kInlinePath)
            overflow_path.pop_back();
        --depth;
        assert(stack_sizes_match());
    }

    // Invariant: everything past the inline stack lives in the overflow stack.
    bool stack_sizes_match() const {
        return static_cast<int>(overflow_path.size()) ==
               (depth > kInlinePath ? depth - kInlinePath : 0);
    }

    void internal_update_position() { node = ascendant()->child(position()); }

    template <typename> friend class WTree;
};
} // namespace WTreeLib

#endif