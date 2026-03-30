#ifndef _WTREE_TREE__H_
#define _WTREE_TREE__H_

#include "traits.hpp"
#include "type_aliases.hpp"

#include "locator.hpp"
#include "node_manager.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <sys/types.h>
#include <type_traits>
#include <utility>

namespace WTreeLib {

template <typename Params>
class WTree : public Params::key_compare, protected WTreeTypeAliases<Params> {
  /** Used Empty Base Optimization (EBO) for the key_compare struct.
   */
  typedef WTree<Params> self_type;
  using Aliases = WTreeTypeAliases<Params>;

public:
  // Re-export shared type aliases.
  using typename Aliases::params_type;

  using typename Aliases::node_type;

  using typename Aliases::base_fields_type;
  using typename Aliases::field_type;
  using typename Aliases::internal_fields_type;
  using typename Aliases::leaf_fields_type;

  using typename Aliases::data_type;
  using typename Aliases::key_type;
  using typename Aliases::mapped_type;
  using typename Aliases::size_type;
  using typename Aliases::value_type;

  using typename Aliases::is_key_compare_to;
  using typename Aliases::key_compare;

  using typename Aliases::const_iterator;
  using typename Aliases::const_pointer;
  using typename Aliases::const_reference;
  using typename Aliases::const_reverse_iterator;
  using typename Aliases::difference_type;
  using typename Aliases::iterator;
  using typename Aliases::pointer;
  using typename Aliases::reference;
  using typename Aliases::reverse_iterator;

  using typename Aliases::allocator_traits;
  using typename Aliases::allocator_type;
  using typename Aliases::internal_allocator_traits;
  using typename Aliases::internal_allocator_type;

  using Aliases::kBasefieldsBytes;
  using Aliases::kInitialCapacity;
  using Aliases::kLastGrowth;
  using Aliases::kTargetK;
  using Aliases::kTargetNodeBytes;
  using Aliases::kUnique;

  using Aliases::kExactMatch;
  using Aliases::kMatchMask;

  using manager_type = WTreeNodeManager<params_type>;
  using locator_type = WTreeLocator<params_type>;

  template <typename> friend class WTreeContainer;

protected:
  manager_type m_manager;
  locator_type m_locator;

public:
  // Default constructor.
  WTree(const key_compare &comp, const allocator_type &alloc);

  // Copy constructor.
  WTree(const self_type &tree)
      : key_compare(tree.key_comp()), m_manager(tree.internal_allocator()),
        m_locator(key_comp()) {
    assign(tree);
  }

  // Move constructor.
  WTree(self_type &&other) noexcept
      : key_compare(std::move(other.key_comp())),
        m_manager(std::move(other.m_manager)), m_locator(key_comp()) {
    other.m_manager.set_root(nullptr);
    other.m_manager.set_size(0);
  }

  // Destructor.
  ~WTree() {
    m_manager.internal_recursive_delete(root());
    mutable_root() = nullptr;
  }

  node_type *root() { return m_manager.root(); }
  const node_type *croot() const { return m_manager.croot(); }
  node_type *&mutable_root() { return m_manager.mutable_root(); }

  size_type size() const { return m_manager.size(); }

  manager_type *manager() { return &m_manager; }
  const manager_type *cmanager() const { return &m_manager; }

  locator_type *locator() { return &m_locator; }
  const locator_type *clocator() const { return &m_locator; }

  key_compare &mutable_key_comp() { return *this; }
  const key_compare &key_comp() const { return *this; }

  // Assign the contents of x to* this.
  self_type &operator=(const self_type &x) {
    if (&x == this)
      return *this;

    assign(x);
    return *this;
  }

  /**
   * @brief Move assignment operator.
   * @details To avoid allocating a new WTree (with a new root node), is better
   * to swap them.
   */
  self_type &operator=(self_type &&other) noexcept {
    if (&other == this)
      return *this;

    swap(other);

    return *this;
  }

  // // MEMO: Old version of the operator:
  // self_type &operator=(self_type &&other) noexcept {
  //   if (&other == this)
  //     return *this;
  //   // Clean up current resources
  //   _recursive_delete(root());

  //   // Move from other
  //   mutable_key_comp() = std::move(other.mutable_key_comp());
  //   if constexpr (internal_allocator_traits::
  //                     propagate_on_container_move_assignment::value) {
  //     mutable_allocator() = std::move(other.mutable_allocator());
  //   }
  //   m_manager.m_size = other.size();
  //   mutable_root() = other.root();

  //   // Leave other in valid empty state
  //   other.mutable_root() = nullptr;
  //   other.m_manager.m_size = 0;

  //   return *this;
  // }

  // Swap two trees.
  void swap(self_type &other) noexcept {
    using std::swap;
    swap(static_cast<key_compare &>(*this), static_cast<key_compare &>(other));
    swap(m_manager.m_size, other.m_manager.m_size);
    swap(m_manager.m_root, other.m_manager.m_root);
    if constexpr (internal_allocator_traits::propagate_on_container_swap::
                      value) {
      swap(mutable_allocator(), other.mutable_allocator());
    } else {
      assert(allocator() == other.allocator());
    }
  }

  // Assign the contents of -other- to -this- tree (copy semantics).
  // Does not invalidate the source tree.
  void assign(const self_type &other) {
    // Clean up existing tree
    m_manager.internal_recursive_delete(root());

    // Copy comparator and allocator
    mutable_key_comp() = other.key_comp();
    if constexpr (internal_allocator_traits::
                      propagate_on_container_copy_assignment::value) {
      mutable_allocator() = other.internal_allocator();
    } else {

      assert(allocator() == other.allocator());
    }

    // Deep clone the tree structure
    if (other.croot()) {
      mutable_root() = clone_subtree(other.croot());
      m_manager.set_size(other.size());
    } else {
      mutable_root() = m_manager.new_internal_node();
      m_manager.set_size(0);
    }
  }

  // Create a deep copy of this tree.
  // Returns a new tree that is independent of this one.
  self_type clone() const {
    self_type result(key_comp(), allocator());
    result.assign(*this);
    return result;
  }

private:
  // Helper function to copy-construct values from source to destination node.
  // Uses optimal method based on type traits.
  static void copy_construct_values(node_type *dest, const node_type *src,
                                    size_t count) {
    if constexpr (std::is_trivially_copyable_v<value_type>) {
      std::memcpy(static_cast<void *>(dest->fields.values),
                  static_cast<const void *>(src->fields.values),
                  count * sizeof(value_type));
    } else {
      std::uninitialized_copy_n(src->fields.values, count, dest->fields.values);
    }
  }

  /**
   * @brief Recursively clone a subtree, preserving exact structure.
   *
   * @param src existing root node for a new tree.
   * @return node_type* a new node that is a deep copy of src.
   */
  node_type *clone_subtree(const node_type *src) {
    if (src == nullptr)
      return nullptr;

    node_type *dest;
    if (src->is_internal()) {
      dest = m_manager.new_internal_node();

      // Recursively clone children
      for (field_type i = 0; i < kTargetK - 1; ++i) {
        if (src->child(i)) {
          dest->fields.pointers[i] = clone_subtree(src->child(i));
        }
      }
    } else {
      // Clone leaf node with same capacity
      dest = m_manager.new_leaf_node(src->capacity());
    }

    // Copy values
    copy_construct_values(dest, src, src->size());
    dest->fields.size = src->size();

    return dest;
  }

public:
  // Allocator routines.
  internal_allocator_type &mutable_allocator() noexcept {
    return m_manager.mutable_allocator();
  }
  const internal_allocator_type &internal_allocator() const noexcept {
    return m_manager.internal_allocator();
  }

  allocator_type allocator() const noexcept {
    return allocator_type(internal_allocator());
  }

  /**
   * @brief Leaves the tree in a valid empty state
   * (with an existing root internal node).
   */
  void clear() {
    assert(root()->is_internal());
    node_type *node = root();
    for (field_type i = 0; i < kTargetK - 1; ++i) {
      if (node->child(i) != nullptr) {
        m_manager.internal_recursive_delete(node->child(i));
        node->set_child(i, nullptr);
      }
    }
    // Lazy remove.
    node->fields.size = 0;
#ifndef NDEBUG
    memset(static_cast<void *>(node->fields.values), 0,
           sizeof(node->fields.values));
#endif
    m_manager.set_size(0);
  }

  // Iterator routines.
  iterator begin() { return iterator(root(), 0); }
  const_iterator begin() const { return const_iterator(croot(), 0); }
  const_iterator cbegin() const { return const_iterator(croot(), 0); }
  iterator end() { return iterator(root(), root()->size()); }
  const_iterator cend() const {
    return const_iterator(croot(), croot() ? croot()->size() : 0);
  }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  bool is_empty() const { return size() == 0; }

  bool node_bounds_key(const key_type &key, const node_type *node) const {
    assert(node != nullptr);
    return node->bounds_key(key, key_comp());
  }

  // #region WTREE_INSERT_AND_EMPLACE_ROUTINES

  // -- Unique container operations --

  /**
   * @brief Inserts a value into the wtree only if it does not already exist,
   * prioritizing horizontal expansion.
   * The boolean return value indicates whether insertion succeeded or failed.
   *
   * @details For a WTree, is better to not use internal_locate because the
   * algorithm makes corrections going down the tree.
   *
   * @return {iterator, true} on insertion, {iterator, false} if key exists.
   */
  template <typename... Args>
  std::pair<iterator, bool> emplace_unique_key_args(const key_type &key,
                                                    Args &&...args) noexcept;

  // Single-argument emplace: extract key directly when possible.
  template <typename P> std::pair<iterator, bool> emplace_unique(P &&x) {
    using extract_tag = WTreeCanExtractKey<P, key_type>;
    if constexpr (std::is_base_of_v<WTreeExtractKeySelfTag, extract_tag>) {
      return emplace_unique_key_args(x, std::forward<P>(x));
    } else if constexpr (std::is_base_of_v<WTreeExtractKeyFirstTag,
                                           extract_tag>) {
      return emplace_unique_key_args(x.first, std::forward<P>(x));
    } else {
      value_type v(std::forward<P>(x));
      return emplace_unique_key_args(params_type::get_key(v), std::move(v));
    }
  }

  // Two-argument emplace for maps: first arg is the key.
  template <typename First, typename Second>
  typename std::enable_if<
      WTreeCanExtractMapKey<First, key_type, value_type>::value,
      std::pair<iterator, bool>>::type
  emplace_unique(First &&f, Second &&s) {
    return emplace_unique_key_args(f, std::forward<First>(f),
                                   std::forward<Second>(s));
  }

  // Variadic emplace fallback: construct value, then extract key.
  template <typename... Args>
  std::pair<iterator, bool> emplace_unique(Args &&...args) {
    value_type v(std::forward<Args>(args)...);
    return emplace_unique_key_args(params_type::get_key(v), std::move(v));
  }

  // Insert with hint. Check to see if the value should be placed immediately
  // before position in the tree. If it does, then the insertion will take
  // amortized constant time. If not, the insertion will take amortized
  // logarithmic time as if a call to emplace_unique(v) were made.
  template <typename... Args>
  iterator emplace_hint_unique_key_args(const_iterator hint,
                                        const key_type &key, Args &&...args) {
    // TODO: Implement emplace_hint_unique_key_args.
    assert(0);
  }

  // Single-argument emplace with hint: extract key directly when possible.
  template <typename P>
  iterator emplace_hint_unique(const_iterator hint, P &&x) {
    using extract_tag = WTreeCanExtractKey<P, key_type>;
    if constexpr (std::is_base_of_v<WTreeExtractKeySelfTag, extract_tag>) {
      return emplace_hint_unique_key_args(hint, x, std::forward<P>(x));
    } else if constexpr (std::is_base_of_v<WTreeExtractKeyFirstTag,
                                           extract_tag>) {
      return emplace_hint_unique_key_args(hint, x.first, std::forward<P>(x));
    } else {
      value_type v(std::forward<P>(x));
      return emplace_hint_unique_key_args(hint, params_type::get_key(v),
                                          std::move(v));
    }
  }

  // Two-argument emplace with hint for maps: first arg is the key.
  template <typename First, typename Second>
  typename std::enable_if<
      WTreeCanExtractMapKey<First, key_type, value_type>::value, iterator>::type
  emplace_hint_unique(const_iterator hint, First &&f, Second &&s) {
    return emplace_hint_unique_key_args(hint, f, std::forward<First>(f),
                                        std::forward<Second>(s));
  }

  // Variadic emplace with hint fallback: construct value, then extract key.
  template <typename... Args>
  iterator emplace_hint_unique(const_iterator hint, Args &&...args) {
    value_type v(std::forward<Args>(args)...);
    return emplace_hint_unique_key_args(hint, params_type::get_key(v),
                                        std::move(v));
  }

  std::pair<iterator, bool> insert_unique(value_type &&v) {
    return emplace_unique_key_args(params_type::get_key(v), std::move(v));
  }
  std::pair<iterator, bool> insert_unique(const value_type &v) {
    return emplace_unique_key_args(params_type::get_key(v), v);
  }

  iterator insert_unique(const_iterator hint, value_type &&v) {
    return emplace_hint_unique_key_args(hint, params_type::get_key(v),
                                        std::move(v));
  }
  iterator insert_unique(const_iterator hint, const value_type &v) {
    return emplace_hint_unique_key_args(hint, params_type::get_key(v), v);
  }

  template <typename P, typename = typename std::enable_if<!std::is_same<
                            typename std::remove_const<
                                typename std::remove_reference<P>::type>::type,
                            value_type>::value>::type>
  std::pair<iterator, bool> insert_unique(P &&x) {
    return emplace_unique(std::forward<P>(x));
  }
  template <typename P, typename = typename std::enable_if<!std::is_same<
                            typename std::remove_const<
                                typename std::remove_reference<P>::type>::type,
                            value_type>::value>::type>
  iterator insert_unique(const_iterator hint, P &&x) {
    return emplace_hint_unique(hint, std::forward<P>(x));
  }

  // === Multi container operations (future extension) ===
  // TODO: Migrate unique/multi dispatch to a Params flag.

  template <typename... Args>
  iterator emplace_multi_key_args(const key_type &key, Args &&...args);

  template <typename... Args> iterator emplace_multi(Args &&...args) {
    value_type v(std::forward<Args>(args)...);
    return emplace_multi_key_args(params_type::get_key(v), std::move(v));
  }

  template <typename... Args>
  iterator emplace_hint_multi_key_args(const_iterator hint, const key_type &key,
                                       Args &&...args);

  template <typename... Args>
  iterator emplace_hint_multi(const_iterator hint, Args &&...args) {
    value_type v(std::forward<Args>(args)...);
    return emplace_hint_multi_key_args(hint, params_type::get_key(v),
                                       std::move(v));
  }

  iterator insert_multi(const value_type &v) {
    return emplace_multi_key_args(params_type::get_key(v), v);
  }
  iterator insert_multi(value_type &&v) {
    return emplace_multi_key_args(params_type::get_key(v), std::move(v));
  }
  iterator insert_multi(const_iterator hint, const value_type &v) {
    return emplace_hint_multi_key_args(hint, params_type::get_key(v), v);
  }
  iterator insert_multi(const_iterator hint, value_type &&v) {
    return emplace_hint_multi_key_args(hint, params_type::get_key(v),
                                       std::move(v));
  }
  template <typename P, typename = typename std::enable_if<!std::is_same<
                            typename std::remove_const<
                                typename std::remove_reference<P>::type>::type,
                            value_type>::value>::type>
  iterator insert_multi(P &&x) {
    return emplace_multi(std::forward<P>(x));
  }
  template <typename P, typename = typename std::enable_if<!std::is_same<
                            typename std::remove_const<
                                typename std::remove_reference<P>::type>::type,
                            value_type>::value>::type>
  iterator insert_multi(const_iterator hint, P &&x) {
    return emplace_hint_multi(hint, std::forward<P>(x));
  }

  // #endregion

  // #region WTREE_SEARCH_ROUTINES

  // === Lower/Upper-Bound methods ===
  // lower_bound and upper_bound are the same for unique and multi containers.
  // They wrap the internal descent methods.

  template <typename IterType>
  std::pair<IterType, int> lower_bound(const key_type &key, IterType &iter) {
    const int result = m_locator.internal_lower_bound(key, iter);
    return {internal_end(iter), result};
  }

  template <typename IterType>
  std::pair<IterType, int> lower_bound(const key_type &key,
                                       IterType &iter) const {
    const int result = m_locator.internal_lower_bound(key, iter);
    return {internal_end(iter), result};
  }

  template <typename IterType>
  std::pair<IterType, int> upper_bound(const key_type &key, IterType &iter) {
    const int result = m_locator.internal_upper_bound(key, iter);
    return {internal_end(iter), result};
  }

  template <typename IterType>
  std::pair<IterType, int> upper_bound(const key_type &key,
                                       IterType &iter) const {
    const int result = m_locator.internal_upper_bound(key, iter);
    return {internal_end(iter), result};
  }

  template <typename IterType>
  std::pair<IterType, int> lower_bound_hint(const key_type &key,
                                            IterType &iter) {
    const int result = m_locator.internal_lower_bound_hint(key, iter);
    return {internal_end(iter), result};
  }

  template <typename IterType>
  std::pair<IterType, int> upper_bound_hint(const key_type &key,
                                            IterType &iter) {
    const int result = m_locator.internal_upper_bound_hint(key, iter);
    return {internal_end(iter), result};
  }
  // === Shared locate (early-stop descent) ===
  // Used by containers for count/contains. Returns (iterator, match_flag).
  //   compare_to:    kExactMatch on hit, -kExactMatch on miss.
  //   plain_compare: kExactMatch on hit, 0 on miss.
  // Uses if constexpr on is_key_compare_to for both compare styles.

  // From root, descend only.

  template <typename IterType>
  std::pair<IterType, int> locate(const key_type &key, IterType &iter) const {
    const int result = m_locator.internal_locate_any(key, iter);
    return std::make_pair(iter, result);
  }

  template <typename IterType>
  std::pair<IterType, int> locate_hint(const key_type &key,
                                       IterType &iter) const {
    const int result = m_locator.internal_locate_any_hint(key, iter);
    return std::make_pair(iter, result);
  }

private:
  // #endregion

  // #region WTREE_ERASE_ROUTINES
public:
  // Erase the specified iterator from the wtree. The iterator must be valid
  // (i.e. not equal to end()).  Return an iterator pointing to the node after
  // the one that was erased (or end() if none exists).
  template <typename Iterator> Iterator erase(Iterator &iter);

  // Erases range. Returns the number of keys erased.
  template <typename Iterator> int erase(Iterator begin, Iterator end);

  // #endregion

private:
  // #region WTREE_PRIVATE_INTERNAL_FUNCTIONS

  /**
   * @brief Adjusts an iterator that has moved past the last element of a child
   * node by ascending to its parent.
   *
   * If the iterator's index equals its node's size and the node has a parent,
   * the iterator ascends to the parent node and advances one position.
   * Otherwise, the iterator is left unchanged.
   *
   * @param iter The iterator to adjust (modified in place).
   * @return A copy of the adjusted iterator.
   */
  template <typename Iterator> Iterator internal_end(Iterator &iter) {
    if (iter.index == iter.node->size() && iter.has_ascendant()) {
      iter.ascend();
      ++iter.index;
    }
    return iter;
  }

  /**
   * @brief A const version of internal_end. Adjusts an iterator that has moved
   * past the last element of a child node by ascending to its parent.
   *
   * If the iterator's index equals its node's size and the node has a parent,
   * the iterator ascends to the parent node and advances one position.
   * Otherwise, the iterator is left unchanged.
   *
   * @param iter The iterator to adjust (modified in place).
   * @return A copy of the adjusted iterator.
   */
  template <typename Iterator> Iterator internal_end(Iterator &iter) const {
    if (iter.index == iter.node->size() && iter.has_ascendant()) {
      iter.ascend();
      ++iter.index;
    }
    return iter;
  }

  // Inserts a value into the WTree immediately before iter. Requires that
  // key(v) <= iter.key() and (--iter).key() <= key(v).
  template <typename IterType, typename... Args>
  IterType internal_emplace_at(IterType &hint, Args &&...args);

  // =====================================================================
  // Find — lower_bound + equality check. Shared logic, works for both
  // unique and multi (lower_bound always finds the first occurrence).
  // =====================================================================

  // template <typename IterType>
  // IterType internal_find(const key_type &key, IterType iter) const;
  // template <typename IterType>
  // IterType internal_find_hint(const key_type &key, IterType iter) const;

  // #endregion
};

} // namespace WTreeLib
#endif