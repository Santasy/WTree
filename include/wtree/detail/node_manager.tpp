#ifndef _WTREE_MANAGER__T_
#define _WTREE_MANAGER__T_

#include "node_manager.hpp"

namespace WTreeLib {
template <typename Params>
void WTreeNodeManager<Params>::internal_move_greatest_upward(iterator &iter) {
  assert(iter.node->size() > 0);

  if (!iter.has_ascendant()) {
    --iter.node->fields.size;
    // Note that [iter.node] may be empty, and must be destroyed
    // from outside this function.
    return;
  }
  int level = 0;
  for (; level < iter.path_size() - 1; ++level) {
    node_type *node = iter.ascendants[level];
    const field_type pos = iter.move_indexes[level];
    node_type::safe_move_backward_range(node->fields.values + pos + 1,
                                        node->fields.values + kTargetK - 1,
                                        node->fields.values + kTargetK);

    iter.ascendants[level + 1]->move_value(
        iter.ascendants[level + 1]->size() - 1, node, pos + 1);
  }
  assert(iter.node != root());
  assert(iter.node->is_leaf());

  node_type::safe_move_backward_range(
      iter.ascendant()->fields.values + iter.position() + 1,
      iter.ascendant()->fields.values + kTargetK - 1,
      iter.ascendant()->fields.values + kTargetK);
  iter.node->move_value(iter.node->size() - 1, iter.ascendant(),
                        iter.position() + 1);
  if (iter.node->size() == 1) {
    internal_destroy_child_unchecked(iter.ascendant(), iter.position());
    iter.ascend();
    return;
  }
  --iter.node->fields.size;
}

template <typename Params>
void WTreeNodeManager<Params>::internal_move_smallest_upward(iterator &iter) {
  assert(iter.node->size() > 0);

  if (!iter.has_ascendant()) {
    // Shift elements left by 1: [1, size+1) -> [0, size)
    node_type::safe_move_range(iter.node->fields.values + 1,
                               iter.node->fields.values + iter.node->size(),
                               iter.node->fields.values);
    --iter.node->fields.size;
    // Note that [iter.node] may be empty, and must be destroyed
    // from outside this functions.
    return;
  }
  int level = 0;
  for (; level < iter.path_size() - 1; ++level) {
    node_type *node = iter.ascendants[level];
    const field_type pos = iter.move_indexes[level];
    // Shift elements left by 1: [1, pos+1) -> [0, pos)
    node_type::safe_move_range(node->fields.values + 1,
                               node->fields.values + pos + 1,
                               node->fields.values);

    iter.ascendants[level + 1]->move_value(0, node, pos);
  }
  assert(iter.node != root());
  assert(iter.node->is_leaf());

  node_type::safe_move_range(iter.ascendant()->fields.values + 1,
                             iter.ascendant()->fields.values + iter.position() +
                                 1,
                             iter.ascendant()->fields.values);
  iter.node->move_value(0, iter.ascendant(), iter.position());
  if (iter.node->size() == 1) {
    internal_destroy_child_unchecked(iter.ascendant(), iter.position());
    iter.ascend();
    return;
  }
  node_type::safe_move_range(iter.node->fields.values + 1,
                             iter.node->fields.values + iter.node->size(),
                             iter.node->fields.values);
  --iter.node->fields.size;
}

template <class Params>
WTreeNodeManager<Params>::node_type *
WTreeNodeManager<Params>::grow_leaf_and_shift_right(node_type *node,
                                                    field_type new_size) {
  const field_type old_size = node->size();

  if (new_size <= node->capacity()) {
    // No reallocation: shift in-place.
    node_type::safe_move_backward_range(node->fields.values,
                                        node->fields.values + old_size,
                                        node->fields.values + new_size);
    return node;
  }

  field_type new_cap;
  if constexpr (std::is_same_v<field_type, uint8_t>) {
    new_cap = SAFE_NEW_SIZE(new_size, kLastGrowth, kTargetK);
  } else if constexpr (std::is_same_v<field_type, ushort>) {
    new_cap = std::min<field_type>(NEW_SIZE(new_size), kTargetK);
  } else {
    static_assert(std::is_same_v<field_type, uint16_t> ||
                      std::is_same_v<field_type, uint8_t>,
                  "Unsupported field_type");
  }

  assert(new_cap > node->capacity());
  assert(new_cap >= new_size);
  node_type *new_node = new_leaf_node(new_cap);

  // Copy values directly to the right-shifted position.
  const field_type offset = new_size - old_size;
  move_values_to_node(node, new_node, old_size, offset);
  new_node->fields.size = new_size;

  delete_leaf_node(node);
  return new_node;
}

} // namespace WTreeLib

#endif