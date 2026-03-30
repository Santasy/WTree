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

    // Iterators typically should be copyable (shallow copy is fine)
    EmptyBaseNodeHandle(const EmptyBaseNodeHandle &) = default;
    EmptyBaseNodeHandle &operator=(const EmptyBaseNodeHandle &) = default;
    EmptyBaseNodeHandle(EmptyBaseNodeHandle &&) noexcept = default;
    EmptyBaseNodeHandle &operator=(EmptyBaseNodeHandle &&) noexcept = default;
    Data data;
  };

  WTreeNodeManager(const internal_allocator_type &alloc)
      : m_root(alloc, nullptr), m_size(0) {}

protected:
  EmptyBaseNodeHandle<internal_allocator_type, node_type *> m_root;
  size_type m_size = 0;

  // Internal accessor routines.
  node_type *root() { return m_root.data; }
  const node_type *croot() const { return m_root.data; }
  node_type *&mutable_root() { return m_root.data; }

  size_type size() const { return m_size; }
  void increment_size() { ++m_size; }
  void increase_size_by(size_type amout) { m_size += amout; }
  void decrement_size() { --m_size; }
  void decrease_size_by(size_type amout) {
    assert(amout <= size());
    m_size -= amout;
  }

  field_type get_initial_capacity() { return kInitialCapacity; }
  field_type get_maximum_capacity() { return kTargetK; }

  void set_size(size_t s) { m_size = s; }

  void set_root(node_type *node) { m_root.data = node; }

public:
  internal_allocator_type &mutable_allocator() noexcept {
    return *static_cast<internal_allocator_type *>(&m_root);
  }
  const internal_allocator_type &internal_allocator() const noexcept {
    return *static_cast<const internal_allocator_type *>(&m_root);
  }

  node_type *new_leaf_node(field_type capacity) {
    internal_allocator_type &ia = mutable_allocator();
    leaf_fields_type *u;
    const int nbytes = kBasefieldsBytes + (sizeof(value_type) * capacity);
    u = reinterpret_cast<leaf_fields_type *>(
        internal_allocator_traits::allocate(ia, nbytes));
    return init_leaf(u, capacity);
  }

  node_type *new_internal_node() {
    internal_allocator_type &ia = mutable_allocator();
    internal_fields_type *u = reinterpret_cast<internal_fields_type *>(
        internal_allocator_traits::allocate(ia, sizeof(internal_fields_type)));
    return init_internal(u);
  }

  node_type *init_leaf(leaf_fields_type *u,
                       field_type init_capacity = kInitialCapacity) {
    node_type *node = reinterpret_cast<node_type *>(u);
    node->fields.size = 0;
    node->fields.capacity = init_capacity;
    node->fields.is_internal = false;

    // We could lazy init values:
#ifndef NDEBUG
    void *res =
        memset(&node->fields.values, 0, init_capacity * sizeof(value_type));
    assert(res != nullptr);
#endif
    return node;
  }

  node_type *init_internal(internal_fields_type *u) {
    node_type *node = reinterpret_cast<node_type *>(u);
    node->fields.size = 0;
    node->fields.capacity = kTargetK;
    node->fields.is_internal = true;

    void *res;
    // We could lazy init values:
#ifndef NDEBUG
    res = memset(&node->fields.values, 0, kTargetK * sizeof(value_type));
    assert(res != nullptr);
#endif

    res = memset(node->fields.pointers, 0, sizeof(node->fields.pointers));
    assert(res != nullptr);
    return node;
  }

  // The function grow_leaf uses SAFE_NEW_SIZE for uint8_t field_type
  // to avoid overflow when computing the new capacity.
  node_type *grow_leaf(node_type *node) {
    assert(node->capacity() < kTargetK);

    field_type new_capacity;
    if (node->capacity() == 1) {
      new_capacity = 2;
    } else if constexpr (std::is_same_v<field_type, uint8_t>) {
      new_capacity = SAFE_NEW_SIZE(node->capacity(), kLastGrowth, kTargetK);
    } else {
      new_capacity = std::min<field_type>(NEW_SIZE(node->capacity()), kTargetK);
    }
    assert(new_capacity > node->capacity());

    node_type *new_node = new_leaf_node(new_capacity);
    if constexpr (std::is_trivially_copyable_v<value_type>) {
      std::memcpy(static_cast<void *>(new_node->fields.values),
                  static_cast<const void *>(node->fields.values),
                  node->size() * sizeof(value_type));
    } else {
      move_values_to_node(node, new_node, node->size());
    }
    new_node->fields.size = node->size();
    delete_leaf_node(node);
    return new_node;
  }

  /** Grows a leaf and shifts existing values to the right end of the node.
   * When reallocation is needed, values are placed directly at the offset
   * position, avoiding a separate backward-shift pass.
   *
   * @param node     The leaf node to grow and shift.
   * @param new_size Target layout size.
   *                 Existing values end up at [new_size - old_size, new_size).
   * @return         The (possibly reallocated) node.
   */
  node_type *grow_leaf_and_shift_right(node_type *node, field_type new_size);

  node_type *grow_leaf_to_size(node_type *node, field_type min_size) {
    if (min_size <= node->capacity())
      return node;

    field_type new_cap;
    if constexpr (std::is_same_v<field_type, uint8_t>) {
      new_cap = SAFE_NEW_SIZE(min_size, kLastGrowth, kTargetK);
    } else if constexpr (std::is_same_v<field_type, ushort>) {
      new_cap = std::min<field_type>(NEW_SIZE(min_size), kTargetK);
    } else {
      static_assert(std::is_same_v<field_type, uint16_t> ||
                        std::is_same_v<field_type, uint8_t>,
                    "Unsupported field_type");
    }

    assert(new_cap > node->capacity());
    assert(new_cap >= min_size);
    node_type *new_node = new_leaf_node(new_cap);

    // Copy values using the appropriate method
    move_values_to_node(node, new_node, node->size());
    new_node->fields.size = node->size();

    delete_leaf_node(node);
    return new_node;
  }

  /**
  Also deletes the replaced leaf node.
   */
  inline node_type *make_internal(node_type *node) {
    if (node->is_internal())
      return node;

    node_type *new_node = new_internal_node();

    move_values_to_node(node, new_node, node->size());
    new_node->fields.size = node->size();

    delete_leaf_node(node);
    return new_node;
  }

  /**
  Also deletes the replaced leaf node.
   */
  inline node_type *make_leaf_from_internal(node_type *node) {
    assert(node->is_internal());
    node_type *leaf = new_leaf_node(kTargetK);

    // Copy values using the appropriate method
    move_values_to_node(node, leaf, node->size());
    leaf->fields.size = node->size();

    delete_internal_node(node);
    return leaf;
  }

  inline void make_child_internal_unchecked(node_type *parent, field_type i) {
    assert(parent->is_internal());
    assert(parent->child(i)->is_leaf());
    parent->fields.pointers[i] = make_internal(parent->child(i));
  }

  inline void make_child_leaf_unchecked(node_type *parent, field_type i) {
    assert(parent->is_internal());
    assert(parent->child(i)->is_internal());
    parent->set_child(i, make_leaf_from_internal(parent->child(i)));
  }

  inline void delete_leaf_node(node_type *&node) {
    internal_allocator_type &ia = mutable_allocator();
    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      for (field_type i = 0; i < node->size(); ++i) {
        node->destroy_value(i);
      }
    }
    const size_t node_size =
        kBasefieldsBytes + (node->capacity() * sizeof(value_type));
    internal_allocator_traits::deallocate(ia, reinterpret_cast<char *>(node),
                                          node_size);
  }

  inline void delete_internal_node(node_type *&node) {
    internal_allocator_type &ia = mutable_allocator();
    if constexpr (!std::is_trivially_destructible_v<value_type>) {
      for (field_type i = 0; i < node->size(); ++i) {
        node->destroy_value(i);
      }
    }
    internal_allocator_traits::deallocate(ia, reinterpret_cast<char *>(node),
                                          sizeof(internal_fields_type));
  }

  void internal_destroy_child_unchecked(node_type *parent, field_type index) {
    assert(parent->is_internal());
    assert(index < parent->size());
    assert(parent->child(index) != nullptr);

    node_type *child_node = parent->child(index);
    if (child_node->is_internal())
      delete_internal_node(child_node);
    else
      delete_leaf_node(child_node);

    parent->set_child(index, nullptr);
  }

  // Erase the tree visiting childrens recursively.
  void internal_recursive_delete(node_type *u) {
    if (u == nullptr) {
      // WARNING: Reaching here may mean a double free ocurred.
      return;
    }
    if (u->is_internal()) {
      for (field_type i = 0; i < kTargetK - 1; ++i) {
        if (u->child(i) != nullptr)
          internal_recursive_delete(u->child(i));
      }
      delete_internal_node(u);
    } else {
      delete_leaf_node(u);
    }
  }

  // Move values from source node to destination node.
  // Note: This function moves/destroys source values.
  void move_values_to_node(node_type *src, node_type *dest, field_type count,
                           field_type dest_offset = 0) {
    if constexpr (std::is_trivially_copyable_v<value_type>) {
      std::memcpy(static_cast<void *>(dest->fields.values + dest_offset),
                  static_cast<void *>(src->fields.values),
                  count * sizeof(value_type));
    } else if constexpr (std::is_move_constructible_v<value_type>) {
      std::uninitialized_move_n(src->fields.values, count,
                                dest->fields.values + dest_offset);
    } else {
      std::uninitialized_copy_n(src->fields.values, count,
                                dest->fields.values + dest_offset);
    }
  }

  /**
  Also destroys an empty leaf when moving the greatest leaf key-value.
   */
  void internal_move_greatest_upward(iterator &iter);

  /**
  Also destroys an empty leaf when moving the greatest leaf key-value.
   */
  void internal_move_smallest_upward(iterator &iter);

  template <typename IterType, typename... Args>
  inline IterType internal_emplace_at(IterType &hint, Args &&...args) {
    hint.node->internal_emplace_as_leaf(hint.index,
                                        std::forward<Args>(args)...);
    increment_size();
    return hint;
  }

  /**
   * @brief Use exactly the location of the iterator.
   * Uses the node as a leaf without any additional validation, growing its size
   * if necessary.
   *
   * @return std::pair<typename WTree<Params>::iterator, bool>
   */

  template <typename... Args>
  iterator internal_emplace_at_with_growht(iterator &it, Args &&...args) {
    assert(it.node != nullptr);
    assert(it.node->size() < kTargetK);

    if (it.node->size() == it.node->capacity()) {
      // Root never grows.
      assert(it.has_ascendant());

      it.node = grow_leaf(it.node);
      it.ascendant()->set_child(it.position(), it.node);
    }
    return internal_emplace_at(it, std::forward<Args>(args)...);
  }

  /** Does not update tree size.
   */
  /** Slides elements from full leaf u to its right sibling v.
   *
   * The logical (K+1)-element sorted array is:
   *   L[] = [u0, ..., u_{idx-1}, new_key, u_{idx}, ..., u_{K-1}]
   * Split at position `mid`:
   *   node keeps    L[0..mid-1]       (mid elements)
   *   pivot    =    L[mid]            (1 element → parent at pi+1)
   *   sibling gets  L[mid+1..K]       (K - mid elements, prepended to v)
   *
   * @param it  Iterator at the full leaf. Updated to the inserted position.
   * @param v   Right sibling (must have space).
   * @param pi  Position of u under its parent (child index).
   */
  template <typename... Args>
  bool internal_balanced_slide_to_right(iterator &it, Args &&...args) {
    node_type *p = it.ascendant();
    const field_type pi = it.position();
    node_type *sibling = p->child(pi + 1);
    auto *const node_vals = it.node->fields.values;

    const field_type mid = (kTargetK >> 1) + (sibling->size() >> 1) + 1;
    const field_type sib_newsize = kTargetK - (mid - 1) + sibling->size();

    // Prepare right sibling: grow + shift old contents right, place old parent
    // pivot. When reallocation occurs, values land directly at the offset.
    sibling = grow_leaf_and_shift_right(sibling, sib_newsize);
    p->set_child(pi + 1, sibling);
    p->move_value(pi + 1, sibling, kTargetK - mid);
    sibling->fields.size = sib_newsize;
    it.node->fields.size = mid;

    // Easy case that keeps it.node untouch.
    // Only one key is moved.
    if (mid == kTargetK && it.index == kTargetK) {
      p->construct_value(pi + 1, std::forward<Args>(args)...);
      it.ascend();
      ++it.index;
      return true;
    }

    if (it.index == mid) {
      // Key becomes the new parent pivot.
      p->construct_value(pi + 1, std::forward<Args>(args)...);
      node_type::safe_move_range(node_vals + mid, node_vals + kTargetK,
                                 sibling->fields.values);
      it.ascend();
      ++it.index;
      assert(it.index == pi + 1);
      return true;
    }

    if (it.index > mid) {
      // Key goes to right sibling v.
      // p[pi+1] gets u[right] (insertion past the split, no shift in u).
      it.node->move_value(mid, p, pi + 1);
      node_type::safe_move_range(node_vals + mid + 1, node_vals + it.index,
                                 sibling->fields.values);
      const field_type newpos = it.index - mid - 1;
      sibling->construct_value(newpos, std::forward<Args>(args)...);
      node_type::safe_move_range(node_vals + it.index, node_vals + kTargetK,
                                 sibling->fields.values + newpos + 1);
      it.ascend();
      it.descend(pi + 1);
      it.index = newpos;
      return true;
    }

    // Key stays in u (it.index < mid).
    // Pivot = u[mid-1] (insertion shifts logical positions mid).
    node_type::safe_move_range(node_vals + mid, node_vals + kTargetK,
                               sibling->fields.values);
    it.node->move_value(mid - 1, p, pi + 1);
    --it.node->fields.size;
    it.node->internal_emplace_as_leaf(it.index, std::forward<Args>(args)...);
    return true;
  }

  /** Slides elements from full leaf node to its left sibling.
   *
   * The logical (K+1)-element sorted array is:
   *   L[] = [u0, ..., u_{idx-1}, new_key, u_{idx}, ..., u_{K-1}]
   * Split at position `mid`:
   *   sibling gets  L[0..mid-1]       (mid elements, appended to v)
   *   pivot      =  L[mid]            (1 element → parent at pi)
   *   node keeps    L[mid+1..K]       (K - mid elements)
   *
   * @param it  Iterator at the full leaf. Updated to the inserted position.
   * @param v   Left sibling (must have space).
   * @param pi  Position of u under its parent (child index).
   */
  template <typename... Args>
  bool internal_balanced_slide_to_left(iterator &it, Args &&...args) {
    node_type *p = it.ascendant();
    const field_type pi = it.position();
    node_type *sibling = p->child(pi - 1);
    auto *const node_vals = it.node->fields.values;

    // Node must remain with this size:
    const field_type target_size = (kTargetK >> 1) + (sibling->size() >> 1) + 1;
    // From mid (excluded) to left, all values must be moved outside the node.
    const field_type mid = kTargetK - target_size;
    const field_type sib_old_size = sibling->fields.size;

    // Prepare left sibling: grow, append old parent pivot to end.
    // No shift needed — existing values stay at [0, sib_old_size).
    sibling = grow_leaf_to_size(sibling, sib_old_size + mid + 1);
    p->fields.pointers[pi - 1] = sibling;
    p->move_value(pi, sibling, sib_old_size);

    // Easy case that keeps it.node untouch.
    // Only one key is moved.
    if (mid == 0 && it.index == 0) {
      p->construct_value(pi, std::forward<Args>(args)...);
      it.ascend();
      ++(sibling->fields.size);
      return true;
    }

    if (it.index == mid) {
      // Key becomes the new parent pivot.
      node_type::safe_move_range(node_vals, node_vals + mid,
                                 sibling->fields.values + sib_old_size + 1);
      sibling->fields.size = sib_old_size + mid + 1;

      node_type::safe_move_range(node_vals + mid, node_vals + kTargetK,
                                 node_vals);
      it.node->fields.size = kTargetK - mid;

      p->construct_value(pi, std::forward<Args>(args)...);
      it.ascend();
      assert(it.index == pi);
      return true;
    }

    if (it.index < mid) {
      // Key goes to left sibling v.
      // Elements for v: u[0..idx-1], key, u[idx..mid-2]. Pivot = u[mid-1].
      node_type::safe_move_range(node_vals, node_vals + it.index,
                                 sibling->fields.values + sib_old_size + 1);
      const field_type newidx = sib_old_size + 1 + it.index;
      node_type::safe_move_range(node_vals + it.index, node_vals + mid - 1,
                                 sibling->fields.values + newidx + 1);
      sibling->construct_value(newidx, std::forward<Args>(args)...);

      it.node->move_value(mid - 1, p, pi);
      node_type::safe_move_range(node_vals + mid, node_vals + kTargetK,
                                 node_vals);
      sibling->fields.size = sib_old_size + 1 + mid;
      it.node->fields.size = target_size;
      it.ascend();
      it.descend(pi - 1);
      it.index = newidx;
      return true;
    }

    // Key stays in u (it.index > mid).
    // Elements for v: u[0..mid-1]. Pivot = u[mid].
    node_type::safe_move_range(node_vals, node_vals + mid,
                               sibling->fields.values + sib_old_size + 1);
    it.node->move_value(mid, p, pi);

    // Shift elements u[mid+1..kTargetK-1] and create key:
    node_type::safe_move_range(node_vals + mid + 1, node_vals + it.index,
                               node_vals);
    it.node->construct_value(it.index - mid - 1, std::forward<Args>(args)...);
    node_type::safe_move_range(node_vals + it.index, node_vals + kTargetK,
                               node_vals + it.index - mid);
    it.index -= mid + 1;
    it.node->fields.size = target_size;
    sibling->fields.size = sib_old_size + mid + 1;
    return true;
  }

  /** Balanced slide into a sibling node.
   *
   * Given a full leaf u with insertion position it.index (0..kTargetK
   * inclusive), tries to slide elements to a non-full adjacent sibling v,
   * distributing the (kTargetK + 1) logical elements (u's keys + new key)
   * across u, a parent pivot, and v.
   */
  template <typename... Args>
  bool attempt_emplace_with_balanced_slide(iterator &it,
                                           Args &&...args) noexcept {
    assert(it.node != nullptr);
    assert(it.node->is_leaf());
    assert(it.has_ascendant());

    const node_type *p = it.ascendant();
    assert(p->is_internal());

    const field_type pi = it.position();

    if (pi < kTargetK - 2 && p->child(pi + 1) != nullptr &&
        p->child(pi + 1)->size() < kTargetK)
      return internal_balanced_slide_to_right(it, std::forward<Args>(args)...);

    if (pi > 0 && p->child(pi - 1) != nullptr &&
        p->child(pi - 1)->size() < kTargetK)
      return internal_balanced_slide_to_left(it, std::forward<Args>(args)...);

    return false;
  }

  // Handles insertion at the edge of a full internal node (index == 0 or
  // index == kTargetK). Displaces boundary keys downward following the
  // boundary child chain, then constructs the new value at the top.
  template <typename... Args>
  iterator emplace_edge_swap(iterator &it, Args &&...args) {
    assert(it.node->is_internal());
    assert(it.node->size() == kTargetK);
    assert(it.index == 0 || it.index == kTargetK);

    const field_type swap_idx = (it.index == 0) ? 0 : kTargetK - 1;
    const field_type child_idx = (it.index == 0) ? 0 : kTargetK - 2;

    // Phase 1: Descend while boundary child exists and is full internal.
    int levels = 0;
    while (it.node->child(child_idx) != nullptr) {
      auto *child = it.node->child(child_idx);
      if (child->size() < kTargetK)
        break;
      if (child->is_leaf())
        break;
      it.descend(child_idx);
      ++levels;
    }

    // Phase 2: Handle the bottom — make space for the last displaced key.
    // For the root, first child could be nullptr.
    node_type *child = it.node->child(child_idx);
    if (child != nullptr) {
      if (child->size() < kTargetK) {
        // Leaf child has space — insert directly.
        // it.node->move_value(swap_idx, child, swap_idx);
        if (swap_idx == 0) {
          it.descend(0);
        } else {
          it.descend_to_right_side(child_idx);
          ++it.index;
        }
        internal_emplace_at_with_growht(
            it, std::move(it.ascendant()->fields.values[swap_idx]));
        decrement_size(); // Correction for later addition.
        it.ascend();
        it.node->destroy_value(swap_idx);
      } else {
        // Full leaf child.
        // First descend to try horizontal balance.
        // If not possible, convert to internal and create child below.
        assert(WTreeRulesAssumptions::use_slide);
        assert(!WTreeRulesAssumptions::use_split);

        if (swap_idx == 0) {
          it.descend(0);
        } else {
          it.descend_to_right_side(child_idx);
          ++it.index;
        }

        if (attempt_emplace_with_balanced_slide(
                it, std::move(it.ascendant()->fields.values[swap_idx]))) {
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
    for (; levels > 0; --levels) {
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
};
} // namespace WTreeLib
#endif