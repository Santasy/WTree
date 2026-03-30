#ifndef _WTREE_NODE__H_
#define _WTREE_NODE__H_

#include "traits.hpp"

#include <cassert>
#include <cstring>
#include <sys/types.h>

namespace WTreeLib {
/**
The WTreeNode class receives a Params template parameter, from which
all types and byte-sizes are calculated and setup.
*/
template <typename Params> class WTreeNode {
  using self_type = WTreeNode<Params>;

public:
  using params_type = Params;
  using size_helper = Params::size_helper;

  using key_type = typename params_type::key_type;
  using data_type = typename params_type::data_type;
  using value_type = typename params_type::value_type;
  using field_type = typename params_type::field_type;

  using pointer = typename params_type::pointer;
  using const_pointer = typename params_type::const_pointer;
  using reference = typename params_type::reference;
  using const_reference = typename params_type::const_reference;
  using key_compare = typename params_type::key_compare;
  using size_type = typename params_type::size_type;
  using difference_type = typename params_type::difference_type;

  static constexpr uint kTargetNodeBytes = params_type::kTargetNodeBytes;
  static constexpr field_type kTargetK = params_type::kTargetK;
  static constexpr field_type kLastGrowth = params_type::kLastGrowth;

  // Binary search threshold configuration from params.
  static constexpr bool is_numeric_key = params_type::is_numeric_key;
  static constexpr uint kBinarySearchThreshold =
      params_type::kBinarySearchThreshold;
  static constexpr bool kUseBinarySearchForInternal =
      params_type::kUseBinarySearchForInternal;

  struct base_fields {   // 3 | 5 -> 4 | 6 bytes
    field_type size;     // 1-2 byte (max = [255, 65535])
    field_type capacity; // 1-2 byte (max = [255, 65535])
    bool is_internal;    // 1 byte
  };
  struct leaf_fields : public base_fields {
    value_type values[kTargetK];
  };
  struct internal_fields : public leaf_fields {
    self_type *pointers[kTargetK - 1];
  };

  internal_fields fields;

  // ================================
  // === Compile-time validations ===
  // ================================

  // Compute the byte offset where values array starts,
  // accounting for alignment.
  static constexpr uint kBasefieldsBytes =
      size_helper::template calculate_aligned_base_size<field_type>();

  static_assert(WTREE_TARGET_INITIAL_BYTES > kBasefieldsBytes,
                "Definition of WTREE_TARGET_INITIAL_BYTES has a value that is "
                "smaller than node's BaseFields."
                " Must be bigger.");

  // Calculate initial capacity, ensuring at least 1 element.
  // For large value_types, WTREE_TARGET_INITIAL_BYTES may not be enough,
  // so we fall back to capacity of 1.
  static constexpr field_type kInitialCapacity_computed =
      static_cast<field_type>((WTREE_TARGET_INITIAL_BYTES - kBasefieldsBytes) /
                              sizeof(value_type));
  static constexpr field_type kInitialCapacity =
      (kInitialCapacity_computed > 3) ? kInitialCapacity_computed : 3;

  static_assert(kInitialCapacity <= kTargetK,
                "kInitialCapacity cannot exceed kTargetK");

  // Wraps a comparator for upper_bound semantics (inverts the predicate).
  // For compare_to (int-returning), first converts to bool via
  // WTreeKeyComparer, then inverts. This ensures the unified search
  // functions take the plain compare path for upper_bound.
  template <typename Compare> static auto make_upper_comp(const Compare &comp) {
    if constexpr (is_compare_to<Compare>()) {
      using bool_comp = WTreeKeyComparer<key_type, Compare, true>;
      return WTreeUpperBoundAdapter<key_type, bool_comp>(bool_comp(comp));
    } else {
      return WTreeUpperBoundAdapter<key_type, Compare>(comp);
    }
  }

  // Search masks for exact matches.
  enum {
    kExactMatch = 1 << 30,
    kMatchMask = kExactMatch - 1,
  };

  // === Helper functions to generalize std::move and std::move_backward ===

  // Safe move function that works with both assignable and non-assignable
  // types.
  static void safe_move_range(value_type *first, value_type *last,
                              value_type *dest);

  // Safe move_backward function.
  static void safe_move_backward_range(value_type *first, value_type *last,
                                       value_type *dest_last);

  // Helper function to reverse a range for non-move-assignable types.
  void reverse_range(value_type *first, value_type *last);

  // Safe rotate implementation that works with non-move-assignable types.
  void safe_rotate(value_type *first, value_type *middle, value_type *last);

  // template <typename... Args>
  // inline auto insert_value_impl(int i, Args &&...args)
  //     -> std::enable_if_t<std::is_move_assignable_v<value_type>, void> {
  //   auto cnt = size();
  //   assert(i <= cnt);
  //   construct_value(cnt, std::forward<Args>(args)...);
  //   std::rotate(fields.values + i, fields.values + cnt,
  //               fields.values + cnt + 1);
  //   ++fields.size;
  // }

  // template <typename... Args>
  // inline auto insert_value_impl(int i, Args &&...args)
  //     -> std::enable_if_t<!std::is_trivially_destructible_v<value_type>,
  //     void> {
  //   auto cnt = size();
  //   assert(i <= cnt);

  //   if (i == cnt) {
  //     construct_value(cnt, std::forward<Args>(args)...);
  //   } else {
  //     // Manual shifting approach
  //     construct_value(cnt, std::move(fields.values[cnt - 1]));
  //     for (int j = cnt - 1; j > i; --j) {
  //       fields.values[j].~value_type();
  //       new (&fields.values[j]) value_type(std::move(fields.values[j -
  //       1]));
  //     }
  //     fields.values[i].~value_type();
  //     construct_value(i, std::forward<Args>(args)...);
  //   }
  //   ++fields.size;
  // }

  // === Node Functions ===

  bool insert(value_type val);

  // std::vector<bool> insert(const std::vector<value_type> &values);
  // std::vector<bool> insert(const value_type *&values, size_t n);

  // bool contains(value_type val);
  // typename self_type::field_type getPosition(value_type val);

  // void print() const;
  // int getHeight() const;

private:
  /* WTreeNode<T>(const self_type &);
  void operator=(const self_type &); */

public:
  // Getter/setter for whether this is a leaf node or not.
  // This value doesn't change after the node is allocated.

  inline bool is_internal() const { return fields.is_internal; }
  inline bool is_leaf() const { return !fields.is_internal; }

  // Getter/setter for the number of values stored in this node.

  inline field_type size() const { return fields.size; }
  inline field_type capacity() const { return fields.capacity; }
  inline bool is_empty() const { return fields.size == 0; }

  // Getters for the key/value at position i in the node.

  inline const key_type &key(field_type i) const {
    return params_type::get_key(fields.values[i]);
  }
  inline reference value(field_type i) {
    return reinterpret_cast<reference>(fields.values[i]);
  }
  inline const_reference value(field_type i) const {
    return reinterpret_cast<const_reference>(fields.values[i]);
  }

  inline reference last_value() {
    return reinterpret_cast<reference>(fields.values[size() - 1]);
  }
  inline const_reference last_value() const {
    return reinterpret_cast<const_reference>(fields.values[size() - 1]);
  }
  template <typename Reference> inline Reference safe_last_value() {
    return reinterpret_cast<Reference>(
        fields.values[fields.size ? fields.size - 1 : 0]);
  }

  template <typename Compare>
  inline bool bounds_key(const key_type &search_key,
                         const Compare &comp) const {
    if (is_empty())
      return false;

    return !comp(search_key, key(0)) && !comp(key(size() - 1), search_key);
  }

  // Swap value i in this node with value j in node x.
  void swap_value(int i, WTreeNode *x, int j) {
    assert(x != this || i != j);
    params_type::swap(fields.values[i], x->fields.values[j]);
  }

  // Swap value i in this node with value i in node x.
  void swap_value(int i, WTreeNode *x) {
    assert(x != this);
    params_type::swap(fields.values[i], x->fields.values[i]);
  }

  // Swap value i with value j.
  void swap_value(int i, int j) {
    assert(i != j);
    params_type::swap(fields.values[i], fields.values[j]);
  }

  // Move value i in this node to value j in node x.
  void move_value(int i, WTreeNode *x, int j) {
    assert(x != this || i != j);
    x->construct_value(j, std::move(fields.values[i]));
    destroy_value(i);
  }

  // Move value i in this node to value i in node x.
  void move_value(int i, WTreeNode *x) {
    assert(x != this);
    x->construct_value(i, std::move(fields.values[i]));
    destroy_value(i);
  }

  // Move value i to value j.
  void move_value(int i, int j) {
    assert(i != j);
    construct_value(j, std::move(fields.values[i]));
    destroy_value(i);
  }

  // Getters/setter for the child at position i in the node.
  WTreeNode *child(field_type i) const {
    assert(!is_leaf());
    return fields.pointers[i];
  }

  void set_child(field_type i, WTreeNode *c) {
    assert(is_internal());
    fields.pointers[i] = c;
  }

  // Swap child i in this node with child j in node x.
  // void swap_child(int i, WTreeNode *x, int j) {
  //   assert(x != this || i != j);
  //   assert(!is_leaf());
  //   assert(!x->is_leaf());
  //   assert(_fields.pointers[i] != nullptr);
  //   assert(x->_fields.pointers[j] != nullptr);
  //   auto &a = _fields.pointers[i];
  //   auto &b = x->_fields.pointers[j];
  //   wtree_swap_helper(a, b);
  //   a->_fields.parent = this;
  //   a->_fields.position = i;
  //   b->_fields.parent = x;
  //   b->_fields.position = j;
  // }

  // Move child i in this node to child j in node x.
  //   void move_child(int from_index, self_type *node, int to_index) {
  //     assert(node != this || from_index != to_index);
  //     assert(!is_leaf());
  //     assert(!node->is_leaf());
  //     assert(fields.pointers[from_index] != nullptr);
  //     self_type *c = fields.pointers[from_index];
  //     node->fields.pointers[to_index] = c;
  //     c->fields.position = to_index;
  //     c->fields.parent = node;
  // #ifndef NDEBUG
  //     fields.pointers[from_index] = nullptr;
  // #endif
  //   }

  // Move child i in this node to child i in node x.
  //   void move_child(int index, WTreeNode *node) {
  //     assert(node != this);
  //     assert(!is_leaf());
  //     assert(!node->is_leaf());
  //     assert(fields.pointers[index] != nullptr);
  //     auto c = fields.pointers[index];
  //     node->fields.pointers[index] = c;
  //     c->_fields.parent = node;
  // #ifndef NDEBUG
  //     fields.pointers[index] = nullptr;
  // #endif
  //   }

  // Move child i to child j.
  //   void move_child(int from_index, int to_index) {
  //     assert(from_index != to_index);
  //     assert(!is_leaf());
  //     assert(fields.pointers[from_index] != nullptr);
  //     auto c = fields.pointers[from_index];
  //     fields.pointers[to_index] = c;
  //     c->_fields.position = to_index;
  // #ifndef NDEBUG
  //     fields.pointers[from_index] = nullptr;
  // #endif
  //   }

  // #region WTREE_NODE_SEARCH

  // Helper: true when comp(a, b) returns int (three-way / compare_to).
  // When the upper_bound adapter wraps compare_to → bool, this correctly
  // returns false, routing through the plain compare path.
  template <typename Compare> static constexpr bool is_compare_to();

  // Unified node search — uses if constexpr on comp return type (int vs bool)
  // to handle both plain compare and compare_to. For compare_to binary search,
  // unique containers early-stop on exact match; multi containers recurse left.
  template <typename Compare>
  int linear_lower_bound_search(const key_type &query_key, int s, int e,
                                const Compare &comp) const;

  template <typename Compare>
  int reversed_linear_upper_bound_search(const key_type &query_key, int s,
                                         int e, const Compare &comp) const;

  template <typename Compare>
  int binary_lower_bound_search(const key_type &k, int s, int e,
                                const Compare &comp) const;

  template <typename Compare>
  int binary_search_any(const key_type &k, int s, int e,
                        const Compare &comp) const;

  // Bounded variants — assumes key is within [key(0), key(size-1)].
  template <typename Compare>
  int closed_linear_lower_bound_search(const key_type &query_key,
                                       const Compare &comp) const;

  template <typename Compare>
  int closed_binary_lower_bound_search(const key_type &k,
                                       const Compare &comp) const;

  // =====================================================================
  // Public node search API — picks linear vs binary, calls directly.
  // Internal nodes: compile-time decision (always full at kTargetK keys).
  // Leaf nodes: runtime hybrid check (variable fill).
  // Upper bound: wraps comparator via make_upper_comp.
  // =====================================================================

  template <typename Compare>
  int lower_bound_internal(const key_type &key, const Compare &comp) const {
    if constexpr (kUseBinarySearchForInternal)
      return binary_lower_bound_search(key, 0, size(), comp);
    else
      return linear_lower_bound_search(key, 0, size(), comp);
  }

  template <typename Compare>
  int lower_bound_leaf(const key_type &key, const Compare &comp) const {
    if (size() >= kBinarySearchThreshold)
      return binary_lower_bound_search(key, 0, size(), comp);
    return linear_lower_bound_search(key, 0, size(), comp);
  }

  template <typename Compare>
  int upper_bound_internal(const key_type &key, const Compare &comp) const {
    auto uc = make_upper_comp(comp);
    if constexpr (kUseBinarySearchForInternal)
      return binary_lower_bound_search(key, 0, size(), uc);
    else
      return linear_lower_bound_search(key, 0, size(), uc);
  }

  template <typename Compare>
  int upper_bound_leaf(const key_type &key, const Compare &comp) const {
    auto uc = make_upper_comp(comp);
    if (size() >= kBinarySearchThreshold)
      return binary_lower_bound_search(key, 0, size(), uc);
    return linear_lower_bound_search(key, 0, size(), uc);
  }

  // Bounded lower bound — key within [key(0), key(size-1)].
  template <typename Compare>
  int bounded_lower_bound_internal(const key_type &key,
                                   const Compare &comp) const {
    if constexpr (kUseBinarySearchForInternal)
      return closed_binary_lower_bound_search(key, comp);
    else
      return closed_linear_lower_bound_search(key, comp);
  }

  template <typename Compare>
  int bounded_lower_bound_leaf(const key_type &key, const Compare &comp) const {
    if (size() >= kBinarySearchThreshold)
      return closed_binary_lower_bound_search(key, comp);
    return closed_linear_lower_bound_search(key, comp);
  }

  // Bounded upper bound — key within [key(0), key(size-1)].
  template <typename Compare>
  int bounded_upper_bound_internal(const key_type &key,
                                   const Compare &comp) const {
    auto uc = make_upper_comp(comp);
    if constexpr (kUseBinarySearchForInternal)
      return closed_binary_lower_bound_search(key, uc);
    else
      return closed_linear_lower_bound_search(key, uc);
  }

  template <typename Compare>
  int bounded_upper_bound_leaf(const key_type &key, const Compare &comp) const {
    auto uc = make_upper_comp(comp);
    if (size() >= kBinarySearchThreshold)
      return closed_binary_lower_bound_search(key, uc);
    return closed_linear_lower_bound_search(key, uc);
  }

  // Returns at index (kTargetK - 1) when no descendant was found,
  // which is the last key index (with no right descendant).
  template <typename T> T first_right_descendant(T from) {
    assert(is_internal());
    for (; from < kTargetK - 1 && child(from) == nullptr; ++from)
      ;
    return from;
  }

  // Returns 0 if no child exist at the left of the [from] value index.
  // This is correct because for a value at index [from]=0 has no left child,
  // and is also efficient as we can use an unsigned [field_type].
  template <typename T> T first_left_descendant(T from) {
    assert(is_internal());
    for (; from > 0 && child(from - 1) == nullptr; --from)
      ;
    return from;
  }

  // #endregion

  // Inserts the value exactly at index i, shifting all existing values
  // at index >= i to the right by 1.
  // Also updates node size.
  template <typename... Args>
  void internal_emplace_as_leaf(field_type i, Args &&...args);

  // Removes the value at position i, shifting all existing values
  // at index > i to the left by 1.
  void internal_remove_as_leaf(field_type i);

  // Rebalances a node with its right sibling.
  void rebalance_right_to_left(WTreeNode *sibling, int to_move);
  void rebalance_left_to_right(WTreeNode *sibling, int to_move);

  // Splits a node, moving a portion of the node's values to its right
  // sibling.
  void split(WTreeNode *sibling, int insert_position);

  // Merges a node with its right sibling, moving all of the values
  // and the delimiting key in the parent node onto itself.
  void merge(WTreeNode *sibling);

  // Swap the contents of "this" and "src".
  void swap(WTreeNode *src);

  // === Node allocation/deletion routines ===

  static WTreeNode *init_leaf(self_type::leaf_fields *u,
                              self_type::field_type init_capacity) {
    u->fields.size = 0;
    u->fields.capacity = init_capacity;
    u->fields.is_internal = false;

#ifndef NDEBUG
    void *res = memset(&(u->values), 0, init_capacity * sizeof(value_type));
    assert(res != nullptr);
#endif
    return reinterpret_cast<WTreeNode *>(u);
  }

  static WTreeNode *init_internal(self_type::internal_fields *u) {
    self_type *node = reinterpret_cast<self_type *>(u);
    node->fields.size = 0;
    node->fields.capacity = kTargetK;
    node->fields.is_internal = true;
    void *res;

#ifndef NDEBUG
    res = memset(&(node->fields.values), 0, kTargetK * sizeof(value_type));
    assert(res != nullptr);
#endif

    res = memset(node->fields.pointers, 0, sizeof(node->fields.pointers));
    assert(res != nullptr);
    return node;
  }

private:
  static constexpr const char zero_value[sizeof(value_type)] = {};

public:
  template <typename... Args>
  void construct_value(value_type *v, Args &&...args) {
    new (static_cast<void *>(v)) value_type(std::forward<Args>(args)...);
  }

  template <typename... Args> void construct_value(int i, Args &&...args) {
    assert(i >= 0);
    assert(i < fields.capacity);
    construct_value(&fields.values[i], std::forward<Args>(args)...);
  }

  void destroy_value(value_type *v) {
    v->~value_type();
#ifndef NDEBUG
    memcpy(static_cast<void *>(v), zero_value, sizeof(value_type));
#endif
  }

  void destroy_value(int i) {
    assert(i >= 0);
    if (i < 0)
      return;
    assert(i < kTargetK);
    destroy_value(&fields.values[i]);
  }
};

} // namespace WTreeLib
#endif