#ifndef _WTREE_SET__H_
#define _WTREE_SET__H_

#include "detail/unique_container.hpp"

#include <memory>

namespace WTreeLib {

// Set params: value_type == Key, stored directly in the node's values array.
template <typename Key, typename Compare, typename Alloc, int TargetNodeSize,
          bool Unique = true>
struct WTreeSetParams : public WTreeCommonParams<Key, Compare, Alloc,
                                                 TargetNodeSize, Key, Unique> {
  using base =
      WTreeCommonParams<Key, Compare, Alloc, TargetNodeSize, Key, Unique>;
  using typename base::value_type;

  // No mapped data for sets.
  using data_type = std::false_type;
  using mapped_type = std::false_type;

  static void swap(value_type &a, value_type &b) { wtree_swap_helper(a, b); }
  static const Key &get_key(const value_type &x) { return x; }
};

template <typename Key, int TargetNodeSize = WTREE_TARGET_NODE_BYTES,
          typename Compare = std::less<Key>,
          typename Alloc = std::allocator<Key>>
class set : public WTreeUniqueContainer<
                WTree<WTreeSetParams<Key, Compare, Alloc, TargetNodeSize>>> {
  using super_type = WTreeUniqueContainer<
      WTree<WTreeSetParams<Key, Compare, Alloc, TargetNodeSize>>>;

public:
  // Inherit all constructors from WTreeUniqueContainer.
  using super_type::super_type;
};

} // namespace WTreeLib

#endif