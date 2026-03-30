#ifndef _WTREE_ITERATOR__T_
#define _WTREE_ITERATOR__T_

#include "iterator.hpp"

#include <cassert>

namespace WTreeLib {

template <typename Node, typename Reference, typename Pointer>
void WTreeIterator<Node, Reference, Pointer>::increment() {
  const field_type c = node->size();

  // Check if we are at the end (in the root node):
  if (index == c) {
    // We must be at the root.
    assert(!has_ascendant());
    return;
  }

  if (node->is_leaf()) {
    assert(has_ascendant());
    assert(c > 0);

    if (index == c - 1) {
      ascend();
    }
    ++index;
    return;
  }

  // For internals:

  if (index < c - 1 && node->child(index) != nullptr) {
    descend(index);
    return;
  }

  if (index == c - 1 && has_ascendant()) {
    ascend();
  }
  ++index;
}

template <typename Node, typename Reference, typename Pointer>
void WTreeIterator<Node, Reference, Pointer>::increment_by(size_t count) {
  while (count > 0) {
    if (node->is_leaf()) {
      // A leaf always has an ascendant,
      // i.e. there is a succesor key.
      assert(node->size() > 0);
      assert(has_ascendant());

      // The rest inside the node is one less, but one is added
      // to include the successor in the move.
      size_t rest = node->size() - index;
      size_t take = std::min(rest, count);
      count -= take;
      index += take;
      if (index < node->size()) {
        return;
      }
      ascend();
      ++index;
      continue;
    }

    if (index == node->size()) {
      // We only get here at the root node,
      // which is the end of the container.
      // Also gets the case for empty root node.
      assert(!has_ascendant());
      return;
    }

    if (index == node->size() - 1) {
      // Last key has no right descendant.

      if (!has_ascendant()) {
        ++index;
        return;
      }

      ascend();
      ++index;
      --count;
      continue;
    }

    // A descendant could exists:

    field_type desc_index = node->first_right_descendant(index);
    const field_type c = node->size();
    if (desc_index == c - 1) {
      // No descendant to right.
      size_t to_node_end = c - index;
      if (count < to_node_end) {
        // Ending before node's end.
        index += count;
        return;
      }

      assert(count >= to_node_end);
      if (!has_ascendant()) {
        index = c;
        return;
      }

      // It is guaranteed that there must be a key in the ascendant.
      count -= to_node_end;
      ascend();
      ++index;
    } else {
      // A descendant exists:
      field_type gap = desc_index - index;
      if (gap >= count) {
        // No need to descend.
        index += count;
        return;
      }

      count -= gap + 1;
      descend(desc_index);
    }
  }
}

template <typename Node, typename Reference, typename Pointer>
void WTreeIterator<Node, Reference, Pointer>::decrement() {
  if (index == 0) {
    if (has_ascendant())
      ascend();
    return;
  }

  --index;
  if (node->is_leaf() || index == (node->size() - 1)) {
    return;
  }

  if (node->child(index) != nullptr) {
    descend_to_right_side(index);
  }
}

template <typename Node, typename Reference, typename Pointer>
void WTreeIterator<Node, Reference, Pointer>::decrement_by(size_t count) {
  while (count > 0) {
    if (node->is_leaf()) {
      // A leaf always has an ascendant,
      // i.e. there is a predeccesor key.
      assert(node->size() > 0);
      assert(has_ascendant());

      if (count <= index) {
        // Here we never ascend from the leaf.
        index -= count;
        return;
      }

      // Plus one to count the index at the ascendant.
      count -= index + 1;
      ascend();
      continue;
    }

    if (index > 0) {
      // A descendant (at index - 1) could exists:
      field_type desc_index = node->first_left_descendant(index);
      if (desc_index == 0) {
        // No descendant to left.
        if (count <= index) {
          // Here we never ascend from the node.
          index -= count;
          return;
        }

        // Plus one to count the index at the ascendant.
        count -= index + 1;
        ascend();
        continue;
      }

      // A descendant exists:
      field_type gap = index - desc_index;
      if (gap >= count) {
        // No need to descend.
        index -= count;
        return;
      }

      count -= gap + 1;
      descend_to_right_side(desc_index - 1);
      continue;
    }

    assert(index == 0);
    if (!has_ascendant()) {
      // We cannot ascend from the root.
      return;
    }
    --count;
    ascend();
  }
}

} // namespace WTreeLib

#endif