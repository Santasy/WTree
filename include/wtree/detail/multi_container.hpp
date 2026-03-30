#ifndef _WTREE_MULTI_CONTAINER__H_
#define _WTREE_MULTI_CONTAINER__H_

#include "containers.hpp"

#include <cassert>

namespace WTreeLib {

/**
 * A common base class for wtree::multiset and wtree::multimap.
 * @tparam Tree A @ref WTree "WTree<Params>" instantiation, where Params is
 *         @ref WTreeSetParams or @ref WTreeMapParams with Unique == false.
 */
template <typename Tree>
class WTreeMultiContainer : public WTreeContainer<Tree> {
  using super_type = WTreeContainer<Tree>;
  using params_type = typename Tree::params_type;

public:
  using typename super_type::allocator_type;
  using typename super_type::const_iterator;
  using typename super_type::iterator;
  using typename super_type::key_compare;
  using typename super_type::key_type;
  using typename super_type::size_type;
  using typename super_type::value_type;

  using super_type::kExactMatch;
  using super_type::kMatchMask;

  // Inherit default, copy, move constructors and assignment from base.
  using super_type::super_type;

  // Range constructor (multi: inserts all, duplicates allowed).
  template <class InputIterator>
  WTreeMultiContainer(InputIterator b, InputIterator e,
                      const key_compare &comp = key_compare(),
                      const allocator_type &alloc = allocator_type())
      : super_type(comp, alloc) {
    insert(b, e);
  }

  // === Lookup routines (multi-specific) ===

  size_type count(const key_type &key) const {
    // TODO: Implement count for multi container (distance(lb, ub)).
    assert(0);
    return 0;
  }

  // === Insertion routines ===
  // Multi containers always succeed — return iterator (not pair).

  iterator insert(const value_type &x) { return this->m_tree.insert_multi(x); }
  template <typename P> iterator insert(P &&x) {
    return this->m_tree.insert_multi(std::forward<P>(x));
  }
  iterator insert(value_type &&x) {
    return this->m_tree.insert_multi(std::move(x));
  }

  iterator insert(const_iterator hint, const value_type &x) {
    return this->m_tree.insert_multi(hint, x);
  }
  template <typename P> iterator insert(const_iterator hint, P &&x) {
    return this->m_tree.insert_multi(hint, std::forward<P>(x));
  }
  iterator insert(const_iterator hint, value_type &&x) {
    return this->m_tree.insert_multi(hint, std::move(x));
  }

  void insert(std::initializer_list<value_type> il) {
    insert(il.begin(), il.end());
  }
  template <typename InputIterator>
  void insert(InputIterator f, InputIterator l) {
    for (; f != l; ++f) {
      insert(*f);
    }
  }

  template <typename... Args> iterator emplace(Args &&...args) {
    return this->m_tree.emplace_multi(std::forward<Args>(args)...);
  }

  template <typename... Args>
  iterator emplace_hint(const_iterator hint, Args &&...args) {
    return this->m_tree.emplace_hint_multi(hint, std::forward<Args>(args)...);
  }

  // === Deletion routines (multi-specific) ===

  using super_type::erase; // Inherit erase(iter) and erase(range) from base.

  // Erases all elements matching key. Returns number of erased elements.
  size_type erase(const key_type &key) {
    // TODO: Implement erase-by-key for multi container (lb + ub + erase range).
    assert(0);
    return 0;
  }
};

} // namespace WTreeLib

#endif
