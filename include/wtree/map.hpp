#ifndef _WTREE_MAP__H_
#define _WTREE_MAP__H_

#include "detail/unique_container.hpp"

#include <memory>

namespace WTreeLib {

// Map params: value_type == pair<const Key, Data>, stored in the node's values
// array. Node size calculations use this pair size for slot layout.
template <typename Key, typename Data, typename Compare, typename Alloc,
          int TargetNodeSize, bool Unique = true>
struct WTreeMapParams
    : public WTreeCommonParams<Key, Compare, Alloc, TargetNodeSize,
                               std::pair<const Key, Data>, Unique> {
  using base = WTreeCommonParams<Key, Compare, Alloc, TargetNodeSize,
                                 std::pair<const Key, Data>, Unique>;
  using typename base::value_type;

  using data_type = Data;
  using mapped_type = Data;

  static void swap(value_type &a, value_type &b) {
    wtree_swap_helper(const_cast<Key &>(a.first), const_cast<Key &>(b.first));
    wtree_swap_helper(a.second, b.second);
  }
  static const Key &get_key(const value_type &x) { return x.first; }
};

/**
 * A common base class for map and safe_map.
 * @tparam WTree A @ref WTree "WTree<WTreeMapParams<...>>" instantiation.
 */
template <typename WTree>
class WTreeMapContainer : public WTreeUniqueContainer<WTree> {
  using super_type = WTreeUniqueContainer<WTree>;

public:
  using typename super_type::const_iterator;
  using typename super_type::iterator;
  using typename super_type::key_type;
  using typename super_type::value_type;
  using data_type = typename WTree::data_type;
  using mapped_type = typename WTree::mapped_type;

  // Inherit all constructors from WTreeUniqueContainer.
  using super_type::super_type;

  template <typename... Args>
  std::pair<iterator, bool> try_emplace(const key_type &key, Args &&...args) {
    return this->tree()->emplace_unique_key_args(
        key, std::piecewise_construct, std::forward_as_tuple(key),
        std::forward_as_tuple(std::forward<Args>(args)...));
  }

  template <typename... Args>
  std::pair<iterator, bool> try_emplace(key_type &&key, Args &&...args) {
    return this->tree()->emplace_unique_key_args(
        key, std::piecewise_construct, std::forward_as_tuple(std::move(key)),
        std::forward_as_tuple(std::forward<Args>(args)...));
  }

  template <typename... Args>
  iterator try_emplace(const_iterator hint, const key_type &key,
                       Args &&...args) {
    return this->tree()->emplace_hint_unique_key_args(
        hint, key, std::piecewise_construct, std::forward_as_tuple(key),
        std::forward_as_tuple(std::forward<Args>(args)...));
  }

  template <typename... Args>
  iterator try_emplace(const_iterator hint, key_type &&key, Args &&...args) {
    return this->tree()->emplace_hint_unique_key_args(
        hint, key, std::piecewise_construct,
        std::forward_as_tuple(std::move(key)),
        std::forward_as_tuple(std::forward<Args>(args)...));
  }

  // Access specified element with bounds checking.
  mapped_type &at(const key_type &key) {
    auto it = this->find(key);
    if (it == this->end()) {
      throw std::out_of_range("map::at:  key not found");
    }
    return it->second;
  }
  const mapped_type &at(const key_type &key) const {
    auto it = this->find(key);
    if (it == this->end()) {
      throw std::out_of_range("map::at:  key not found");
    }
    return it->second;
  }

  // Insertion routines.
  data_type &operator[](const key_type &key) {
    return this->try_emplace(key).first->second;
  }

  data_type &operator[](key_type &&key) {
    return this->try_emplace(std::move(key)).first->second;
  }
};

template <
    typename Key, typename Value, int TargetNodeSize = WTREE_TARGET_NODE_BYTES,
    typename Compare = std::less<Key>, typename Alloc = std::allocator<Key>>
class map
    : public WTreeMapContainer<
          WTree<WTreeMapParams<Key, Value, Compare, Alloc, TargetNodeSize>>> {
  using self_type = map<Key, Value, TargetNodeSize, Compare, Alloc>;
  using super_type = WTreeMapContainer<
      WTree<WTreeMapParams<Key, Value, Compare, Alloc, TargetNodeSize>>>;

public:
  // Inherit all constructors from WTreeMapContainer.
  using super_type::super_type;

  bool operator==(const self_type &rhs) const {
    return this->size() == rhs.size() &&
           std::equal(this->cbegin(), this->cend(), rhs.cbegin());
  }

  bool operator<(const self_type &rhs) const {
    return std::lexicographical_compare(this->cbegin(), this->cend(),
                                        rhs.cbegin(), rhs.cend());
  }

  bool operator!=(const self_type &rhs) const { return !(*this == rhs); }

  bool operator>(const self_type &rhs) const { return rhs < this; }

  bool operator>=(const self_type &rhs) const { return !(*this < rhs); }

  bool operator<=(const self_type &rhs) const { return !(rhs < *this); }
};

} // namespace WTreeLib

#endif