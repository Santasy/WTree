#ifndef _WTREE_LOCATOR__H_
#define _WTREE_LOCATOR__H_

#include "traits.hpp"
#include "type_aliases.hpp"

#include <cassert>
#include <utility>

namespace WTreeLib {
template <typename Params>
class WTreeLocator : protected WTreeTypeAliases<Params> {
  typedef WTreeLocator<Params> self_type;
  using Aliases = WTreeTypeAliases<Params>;

public:
  using typename Aliases::const_iterator;
  using typename Aliases::const_pointer;
  using typename Aliases::const_reference;
  using typename Aliases::const_reverse_iterator;
  using typename Aliases::field_type;
  using typename Aliases::is_key_compare_to;
  using typename Aliases::iterator;
  using typename Aliases::key_compare;
  using typename Aliases::key_type;
  using typename Aliases::node_type;
  using typename Aliases::params_type;
  using typename Aliases::pointer;
  using typename Aliases::reference;
  using typename Aliases::reverse_iterator;
  using typename Aliases::value_type;

  using Aliases::kExactMatch;
  using Aliases::kMatchMask;
  using Aliases::kTargetK;
  using Aliases::kUnique;

  const key_compare &m_comp;

  explicit WTreeLocator(const key_compare &comp) : m_comp(comp) {}

  const key_compare &key_comp() const { return m_comp; }

  bool compare_keys(const key_type &x, const key_type &y) const {
    return wtree_compare_keys(key_comp(), x, y);
  }

  /**
   * @brief Locate with early-stop descent. Can stop at any level on first exact
   * match. Does not guarantee finding the first or last occurence.
   *
   * @return int Uses inner dispatching to use a
   * plain_compare or compare_to style:
   * - compare_to:    kExactMatch on hit, -kExactMatch on miss.
   * - plain_compare: kExactMatch on hit, 0 on miss.
   */
  template <typename IterType>
  int internal_locate_any(const key_type &key, IterType &iter) const;

  template <typename IterType>
  int internal_locate_any_hint(const key_type &key, IterType &iter) const {
    internal_ascend_until_bound(key, iter);
    return internal_locate_any(key, iter);
  }

  template <typename IterType>
  void internal_ascend_until_bound(const key_type &key, IterType &iter) const {
    // First look upwards.
    auto s = iter.ascendants.size();
    while (s-- > 0) {
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
  }

  // =====================================================================
  // Lower bound — shared, descends fully to find first key >= search key.
  // Used by find (both unique/multi) and the lower_bound() public API.
  // May return index == size() at a leaf (valid insertion slot, not a
  // valid key position — caller converts via candidate tracking).
  // Uses if constexpr on is_key_compare_to to handle both compare styles.
  // =====================================================================

  // From root — descend only.
  template <typename IterType>
  int internal_lower_bound_unique(const key_type &key, IterType &iter) const;

  template <typename IterType>
  int internal_lower_bound_multi(const key_type &key, IterType &iter) const {
    // TODO: Implement internal_lower_bound_unique_hint.
    assert(0);
  };

  template <typename IterType>
  int internal_lower_bound(const key_type &key, IterType &iter) const {
    if constexpr (kUnique) {
      return internal_lower_bound_unique(key, iter);
    } else {
      return internal_lower_bound_multi(key, iter);
    }
  };

  // From hint — may ascend.
  template <typename IterType>
  int internal_lower_bound_unique_hint(const key_type &key,
                                       IterType &iter) const;
  template <typename IterType>
  int internal_lower_bound_multi_hint(const key_type &key,
                                      IterType &iter) const {
    // TODO: Implement internal_lower_bound_unique_hint.
    assert(0);
  };

  template <typename IterType>
  int internal_lower_bound_hint(const key_type &key, IterType &iter) const {
    if constexpr (kUnique) {
      return internal_lower_bound_unique_hint(key, iter);
    } else {
      return internal_lower_bound_multi_hint(key, iter);
    }
  };

  // =====================================================================
  // Upper bound — shared, descends fully to find first key > search key.
  // Used by emplace (both unique/multi) and the upper_bound() public API.
  // Same index == size() caveat as lower_bound.
  // Uses if constexpr on is_key_compare_to to handle both compare styles.
  // =====================================================================

  // From root — descend only.
  template <typename IterType>
  int internal_upper_bound(const key_type &key, IterType &iter) const {
    // TODO: Implement upper_bound descent (from root).
    assert(0);
  }

  // From hint — may ascend.
  template <typename IterType>
  int internal_upper_bound_hint(const key_type &key, IterType &iter) const {
    // TODO: Implement upper_bound descent (from root).
    assert(0);
  }

  void visit_path_to_greatest_leaf_value(iterator &iter) noexcept;
  void visit_path_to_smallest_leaf_value(iterator &iter) noexcept;
};
} // namespace WTreeLib
#endif