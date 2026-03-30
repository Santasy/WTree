#ifndef _WTREE_ITERATOR__H_
#define _WTREE_ITERATOR__H_

#include "node.hpp"
#include "traits.hpp"

#include <cassert>
#include <limits>
#include <type_traits>
#include <utility>

namespace WTreeLib {
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

  typedef WTreeIterator<normal_node, normal_reference, normal_pointer> iterator;
  typedef WTreeIterator<const_node, const_reference, const_pointer>
      const_iterator;
  typedef WTreeIterator<Node, Reference, Pointer> self_type;

  static_assert(
      std::numeric_limits<int>::max() >= params_type::kTargetK,
      "Index has no enought size for the last valid node's values index.");

  // The node in the tree the iterator is pointing at.
  normal_node *node;
  // The index within the node of the tree the iterator is pointing at.
  int index;
  // Vectors for ascendant path in search and remove routines.
  std::vector<normal_node *> ascendants;
  std::vector<field_type> move_indexes;

  template <typename> friend class WTree;
  template <typename> friend class WTreeNodeManager;
  template <typename> friend class WTreeNodeLocator;
  friend iterator;
  friend const_iterator;

  WTreeIterator() : node(nullptr), index(-1), ascendants(), move_indexes() {
#ifdef WTREE_DBG_ITERATORS
    std::cout << "Constructing iterator with no arguments.\n";
#endif
  }

  WTreeIterator(Node *n, int p)
      : node(const_cast<normal_node *>(n)), index(p), ascendants(),
        move_indexes() {
#ifdef WTREE_DBG_ITERATORS
    std::cout << "Constructing iterator with initial node and index.\n";
#endif
  }

  WTreeIterator(const WTreeIterator &) = default;
  WTreeIterator &operator=(const WTreeIterator &) = default;
  WTreeIterator(WTreeIterator &&) noexcept = default;
  WTreeIterator &operator=(WTreeIterator &&) noexcept = default;

  // This is the normal-to-const iterator convertion.
  template <typename OtherNode, typename OtherRef, typename OtherPtr>
  WTreeIterator(const WTreeIterator<OtherNode, OtherRef, OtherPtr> &other)
      : node(other.node), index(other.index), ascendants(other.ascendants),
        move_indexes(other.move_indexes) {
#ifdef WTREE_DBG_ITERATORS
    std::cout << "Constructing iterator from another const iterator.\n";
#endif
  }

  int path_size() const { return ascendants.size(); }

  bool is_valid() { return node != nullptr; }

  int position() const noexcept {
    return has_ascendant() ? move_indexes[path_size() - 1] : -1;
  }
  bool has_ascendant() const noexcept { return !ascendants.empty(); }
  normal_node *ascendant() const noexcept {
    assert(has_ascendant());
    return ascendants[path_size() - 1];
  }

  normal_node *safe_ascendant() const noexcept {
    return has_ascendant() ? ascendants[path_size() - 1] : nullptr;
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
    if (has_ascendant()) {
      ascend();
      return;
    }
  }

  // Descend ocurrs in the from[index] to better advance the iterator.
  // See [descend_from_right(field_type from)] for the version that is used
  // finding the path to the greatest key in leaves.
  void descend(field_type from) {
    assert(node->child(from) != nullptr);

    ascendants.push_back(node);
    move_indexes.push_back(from);
    node = node->child(from);

    index = 0;
  }

  void descend_to_right_side(field_type from) {
    assert(node->child(from) != nullptr);

    ascendants.push_back(node);
    move_indexes.push_back(from);
    node = node->child(from);

    assert(node->size() > 0);
    index = node->size() - 1;
  }

private:
  // Increment/decrement the iterator.
  void increment();
  void increment_by(size_t count);

  void decrement();
  void decrement_by(size_t count);

  void pop_stacks_unchecked() {
    move_indexes.pop_back();
    ascendants.pop_back();
  }

  void internal_update_position() { node = ascendant()->child(position()); }

  template <typename> friend class WTree;
};
} // namespace WTreeLib

#endif