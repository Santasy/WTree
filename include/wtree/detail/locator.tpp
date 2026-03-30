#ifndef _WTREE_LOCATOR__T_
#define _WTREE_LOCATOR__T_

#include "locator.hpp"

namespace WTreeLib {

template <typename Params>
template <typename IterType>
int WTreeLocator<Params>::internal_locate_any(const key_type &key,
                                              IterType &iter) const {
  if (iter.node->size() == 0)
    return 0;

  for (;;) {
    assert(iter.node->size() > 0);

    // Bounds check for early stop.
    // For compare_to, use three-way comparison to detect exact matches
    // at the boundaries for free.
    if constexpr (is_key_compare_to::value) {
      int cmp_first = key_comp()(key, iter.node->key(0));
      if (cmp_first < 0) {
        iter.index = 0;
        break;
      }
      if (cmp_first == 0) {
        iter.index = 0;
        return kExactMatch;
      }

      int cmp_last = key_comp()(iter.node->key(iter.node->size() - 1), key);
      if (cmp_last < 0) {
        iter.index = iter.node->size();
        break;
      }
      if (cmp_last == 0) {
        iter.index = iter.node->size() - 1;
        return kExactMatch;
      }
    } else {
      if (key_comp()(key, iter.node->key(0))) {
        iter.index = 0;
        break;
      }
      if (key_comp()(iter.node->key(iter.node->size() - 1), key)) {
        iter.index = iter.node->size();
        break;
      }
    }

    // Bounded search — key is within [key(0), key(size-1)].
    if constexpr (is_key_compare_to::value) {
      int res;
      if (iter.node->is_internal())
        res = iter.node->bounded_lower_bound_internal(key, key_comp());
      else
        res = iter.node->bounded_lower_bound_leaf(key, key_comp());
      iter.index = res & kMatchMask;
      if (res & kExactMatch) {
        return kExactMatch;
      }
    } else {
      if (iter.node->is_internal())
        iter.index = iter.node->bounded_lower_bound_internal(key, key_comp());
      else
        iter.index = iter.node->bounded_lower_bound_leaf(key, key_comp());
      if (!key_comp()(key, iter.key())) { // Exact match.
        return kExactMatch;
      }
    }

    if (iter.index == 0)
      break;

    if (iter.node->is_leaf() || iter.node->child(iter.index - 1) == nullptr)
      break;

    iter.descend(iter.index - 1);
  }

  if constexpr (is_key_compare_to::value)
    return -kExactMatch;
  else
    return 0;
}

template <typename Params>
template <typename IterType>
int WTreeLocator<Params>::internal_lower_bound_unique(const key_type &key,
                                                      IterType &iter) const {
  if (iter.node->size() == 0) {
    if constexpr (is_key_compare_to::value)
      return static_cast<int>(-kExactMatch);
    else
      return 0;
  }

  for (;;) {
    assert(iter.node->size() > 0);

    // Bounds check for early stop.
    if constexpr (is_key_compare_to::value) {
      // For compare_to, use three-way comparison to detect exact matches
      // at the boundaries for free.
      int cmp_first = key_comp()(key, iter.node->key(0));
      if (cmp_first < 0) {
        iter.index = 0;
        break;
      }
      if (cmp_first == 0) {
        iter.index = 0;
        return static_cast<int>(kExactMatch);
      }

      int cmp_last = key_comp()(iter.node->key(iter.node->size() - 1), key);
      if (cmp_last < 0) {
        iter.index = iter.node->size();
        break;
      }
      if (cmp_last == 0) {
        iter.index = iter.node->size() - 1;
        return static_cast<int>(kExactMatch);
      }
    } else {
      if (key_comp()(key, iter.node->key(0))) {
        iter.index = 0;
        break;
      }
      if (key_comp()(iter.node->key(iter.node->size() - 1), key)) {
        iter.index = iter.node->size();
        break;
      }
    }

    // Bounded search — lower bound is within [key(0), key(size-1)].
    if (iter.node->is_leaf()) {
      if constexpr (is_key_compare_to::value) {
        int res = iter.node->bounded_lower_bound_leaf(key, key_comp());
        iter.index = res & kMatchMask;
        if (res & kExactMatch)
          return static_cast<int>(kExactMatch);
      } else {
        iter.index = iter.node->bounded_lower_bound_leaf(key, key_comp());
        if (!key_comp()(key, iter.key()))
          return static_cast<int>(kExactMatch);
      }
      break;
    }

    // Internal node.
    if constexpr (is_key_compare_to::value) {
      int res = iter.node->bounded_lower_bound_internal(key, key_comp());
      iter.index = res & kMatchMask;
      if (res & kExactMatch)
        return static_cast<int>(kExactMatch);
    } else {
      iter.index = iter.node->bounded_lower_bound_internal(key, key_comp());
      if (!key_comp()(key, iter.key()))
        return static_cast<int>(kExactMatch);
    }

    assert(iter.index > 0);
    assert(iter.index < iter.node->size());

    if (iter.node->child(iter.index - 1) == nullptr)
      break;

    iter.descend(iter.index - 1);
  }

  if constexpr (is_key_compare_to::value)
    return -kExactMatch;
  else
    return 0;
}

template <typename Params>
template <typename IterType>
int WTreeLocator<Params>::internal_lower_bound_unique_hint(
    const key_type &key, IterType &iter) const {
  // First look upwards.
  while (iter.ascendants.size() > 0) {
    assert(iter.node->size() > 0);
    if (key_comp()(key, iter.node->key(0))) {
      iter.ascend();
      continue;
    }

    if (key_comp()(iter.node->key(iter.node->size() - 1), key)) {
      iter.ascend();
      ++iter.index;
      continue;
    }

    // Break when key is surely bounded by the node.
    break;
  }
  return internal_lower_bound_unique(key, iter);
}

template <typename Params>
void WTreeLocator<Params>::visit_path_to_greatest_leaf_value(
    iterator &iter) noexcept {
  assert(iter.node);
  iter.index = iter.node->size() - 1;

  while (iter.node->is_internal()) {
    assert(iter.node->size() > 0);
    iter.index = iter.node->first_left_descendant(iter.index);
    if (iter.index == 0) {
      // u->setAsLeaf(); // Node may not need pointers.
      return;
    }
    iter.descend_to_right_side(iter.index - 1);
  }
}

template <typename Params>
void WTreeLocator<Params>::visit_path_to_smallest_leaf_value(
    iterator &iter) noexcept {
  assert(iter.node);
  iter.index = iter.node->size();

  while (iter.node->is_internal()) {
    assert(iter.node->size() > 0);
    iter.index = iter.node->first_right_descendant(0);
    if (iter.index == iter.node->size() - 1) {
      // u->setAsLeaf(); // Node may not need pointers.
      return;
    }
    iter.descend(iter.index);
  }
}

} // namespace WTreeLib
#endif