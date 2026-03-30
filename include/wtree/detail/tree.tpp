#ifndef _WTREE_TREE__T_
#define _WTREE_TREE__T_

#include "tree.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

// #define DBGVERBOSE
#ifdef DBGVERBOSE
#include <cstddef>
#include <iostream>
#endif

namespace WTreeLib {

template <typename Params>
WTree<Params>::WTree(const key_compare &comp, const allocator_type &alloc)
    : key_compare(comp), m_manager(internal_allocator_type(alloc)),
      m_locator(key_comp()) {
  m_manager.set_root(m_manager.new_internal_node());

#ifdef DBGVERBOSE
  std::printf("Creating node<value=%lu> of %lu bytes: "
              "\tfrom %lu to %lu keys.\n",
              sizeof(value_type), (unsigned long)kTargetNodeBytes,
              (unsigned long)kInitialCapacity, (unsigned long)kTargetK);

  size_t real_bytes = sizeof(leaf_fields_type);
  if (real_bytes > kTargetNodeBytes) {
    if (kInitialCapacity == 3) {
      std::printf("\t[WARNING] Nodes are %lu bytes instead of %u bytes to "
                  "have 3 values.\n",
                  real_bytes, kTargetNodeBytes);
    } else {
      std::printf("\t[ERROR] Nodes are %lu bytes, exceeding target of %u "
                  "bytes!\n",
                  real_bytes, kTargetNodeBytes);
    }
  } else if (real_bytes < kTargetNodeBytes) {
    std::printf(
        "\t[INFO] Nodes are %lu bytes (%u bytes unused of %u target).\n",
        real_bytes, kTargetNodeBytes - (uint)real_bytes, kTargetNodeBytes);
  }
  std::cout << "=====" << std::endl;
#endif
}

// #region WTREE_SEARCH_ROUTINES

// #endregion

// #region WTREE_INSERT_ROUTINES

/**
 * @details Uses internal_lower_bound_unique to find the insertion point:
 * - On exact match: returns (iter, false) — key already exists.
 * - On miss: the iterator lands at the lower bound position, which may be
 *   at a leaf, at an internal node with a null child, or at the edges
 *   (index == 0 or index == size()).
 *
 * Insertion rules, in order:
 * 1. If the node is not full, emplace directly.
 * 2. If the node is full and internal, displace the boundary key into a
 *    descendant via edge swap, or create a new child for mid-range.
 * 3. If the node is full and a leaf, try balanced slide (R3). On failure,
 *    convert to internal and apply rule 2.
 *
 * @return {iterator, true} on insertion, {iterator, false} if key exists.
 */
template <typename Params>
template <typename... Args>
std::pair<typename WTree<Params>::iterator, bool>
WTree<Params>::emplace_unique_key_args(const key_type &key,
                                       Args &&...args) noexcept {
  assert(root() != nullptr);

  iterator it(root(), 0);

  if (m_locator.internal_locate_any(key, it) & kExactMatch)
    return std::make_pair(it, false);

  // R2: Node not full — emplace directly.
  if (it.node->size() < kTargetK)
    return std::make_pair(m_manager.internal_emplace_at_with_growht(
                              it, std::forward<Args>(args)...),
                          true);

  // Full leaf — try balanced slide (R3), then convert to internal.
  if (it.node->is_leaf()) {
    assert(it.has_ascendant());
    assert(WTreeRulesAssumptions::use_slide);
    assert(!WTreeRulesAssumptions::use_split);

    if (m_manager.attempt_emplace_with_balanced_slide(
            it, std::forward<Args>(args)...)) {
      m_manager.increment_size();
      return std::make_pair(it, true);
    }
    m_manager.make_child_internal_unchecked(it.ascendant(), it.position());
    it.internal_update_position();
  }

  // Full internal node from here.
  assert(it.node->is_internal());
  assert(it.node->size() == kTargetK);

  // Edge cases. Displace boundary key via descendant chain.
  if (it.index == 0)
    return std::make_pair(
        m_manager.emplace_edge_swap(it, std::forward<Args>(args)...), true);

  if (it.index == kTargetK)
    return std::make_pair(
        m_manager.emplace_edge_swap(it, std::forward<Args>(args)...), true);

  // Mid-range: child(index-1) must be null (lower_bound would have
  // descended).
  assert(it.node->child(it.index - 1) == nullptr);
  it.node->set_child(it.index - 1, m_manager.new_leaf_node(kInitialCapacity));
  it.descend(it.index - 1);
  it.node->construct_value(0, std::forward<Args>(args)...);
  ++it.node->fields.size;
  m_manager.increment_size();
  return std::make_pair(it, true);
};

template <typename Params>
template <typename... Args>
typename WTree<Params>::iterator
WTree<Params>::emplace_multi_key_args(const key_type &key, Args &&...args) {
  iterator iter = iterator(root(), 0);
  // TODO:
  assert(0);
  return internal_emplace_at(iter, std::forward<Args>(args)...);
}

template <typename Params>
template <typename... Args>
typename WTree<Params>::iterator WTree<Params>::emplace_hint_multi_key_args(
    const_iterator hint, const key_type &key, Args &&...args) {
  // TODO:
  assert(0);
  return emplace_multi(std::forward<Args>(args)...);
}

// #endregion

// #region WTREE_ERASE_ROUTINES

template <typename P>
template <typename Iterator>
Iterator WTree<P>::erase(Iterator &iter) {
  assert(iter != end());

  if (iter.node->size() == 1) {
    if (iter.has_ascendant()) {
      m_manager.internal_destroy_child_unchecked(iter.ascendant(),
                                                 iter.position());
      iter.ascend();
      ++iter;
    } else {
      assert(iter.node == root());
      if constexpr (!std::is_move_assignable_v<value_type>) {
        iter.node->destroy_value(0);
      }
      --iter.node->fields.size;
    }
    m_manager.decrement_size();
    return iter;
  }

  if (iter.node->is_internal()) {
    field_type child_lower_bound = iter.node->first_left_descendant(iter.index);
    // A zero value means that there was no child because [iter.index]=0 has
    // no left child.
    if (child_lower_bound > 0) {
      node_type::safe_move_backward_range(
          iter.node->fields.values + child_lower_bound,
          iter.node->fields.values + iter.index,
          iter.node->fields.values + iter.index + 1);

      // Cheap copy to use for descending.
      iterator child_node(iter.node->child(child_lower_bound - 1),
                          child_lower_bound - 1);

      child_node.node->move_value(child_node.node->size() - 1, iter.node,
                                  child_lower_bound);

      m_locator.visit_path_to_greatest_leaf_value(child_node);
      if (child_node.node->is_internal()) {
        // Node should be transform to a leaf.
        node_type *ancestor =
            child_node.has_ascendant() ? child_node.ascendant() : iter.node;
        const field_type pos = child_node.has_ascendant()
                                   ? child_node.position()
                                   : child_lower_bound - 1;
        m_manager.make_child_leaf_unchecked(ancestor, pos);
        child_node.node = ancestor->child(pos);
      }

      m_manager.internal_move_greatest_upward(child_node);
      if (child_node.node->size() == 0) {
        m_manager.internal_destroy_child_unchecked(iter.node,
                                                   child_lower_bound - 1);
      }

      m_manager.decrement_size();
      return ++iter;
    }

    child_lower_bound = iter.node->first_right_descendant(iter.index);
    // [iter.index] == kTarget-1 means no child was found, because
    // value at indes kTarget-1 has no right descendant.
    if (child_lower_bound < kTargetK - 1) {
      node_type::safe_move_range(iter.node->fields.values + iter.index + 1,
                                 iter.node->fields.values + child_lower_bound +
                                     1,
                                 iter.node->fields.values + iter.index);

      // Cheap copy to use for descending.
      iterator child_iter(iter.node->child(child_lower_bound),
                          child_lower_bound);
      child_iter.node->move_value(0, iter.node, child_lower_bound);

      m_locator.visit_path_to_smallest_leaf_value(child_iter);
      if (child_iter.node->is_internal()) {
        // Node should be transform to a leaf.
        node_type *ancestor =
            child_iter.has_ascendant() ? child_iter.ascendant() : iter.node;
        const field_type pos = child_iter.has_ascendant()
                                   ? child_iter.position()
                                   : child_lower_bound;
        m_manager.make_child_leaf_unchecked(ancestor, pos);
        child_iter.node = ancestor->child(pos);
      }

      m_manager.internal_move_smallest_upward(child_iter);
      if (child_iter.node->size() == 0) {
        m_manager.internal_destroy_child_unchecked(iter.node,
                                                   child_lower_bound);
      }

      m_manager.decrement_size();
      return iter;
    }

    // At this point, it is certain that this node could be a leaf.
    if (iter.node != root()) {
      iter.node = m_manager.make_leaf_from_internal(iter.node);
      iter.ascendant()->set_child(iter.position(), iter.node);
    }
  }

  node_type::safe_move_range(iter.node->fields.values + iter.index + 1,
                             iter.node->fields.values + iter.node->size(),
                             iter.node->fields.values + iter.index);
  --iter.node->fields.size;
  m_manager.decrement_size();
  return iter;
}

template <typename P>
template <typename Iterator>
int WTree<P>::erase(Iterator begin, Iterator end) {
  int count = distance(begin, end);
  for (int i = 0; i < count; i++) {
    begin = erase(begin);
  }
  return count;
}

// #endregion

} // namespace WTreeLib
#endif
