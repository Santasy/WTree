#ifndef _WTREE_TYPE_ALIASES__H_
#define _WTREE_TYPE_ALIASES__H_

#include "iterator.hpp"
#include "node.hpp"

#include <iterator>

namespace WTreeLib {

/// Shared type aliases for WTree, WTreeLocator, and WTreeNodeManager.
/// Inherit (protected) and re-export with `using typename` as needed.
template <typename Params> struct WTreeTypeAliases {
  using params_type = Params;
  using node_type = WTreeNode<params_type>;

  // --- Node layout types ---
  using base_fields_type = typename node_type::base_fields;
  using leaf_fields_type = typename node_type::leaf_fields;
  using internal_fields_type = typename node_type::internal_fields;

  // --- Key / value types ---
  using field_type = typename params_type::field_type;
  using key_type = typename Params::key_type;
  using data_type = typename Params::data_type;
  using mapped_type = typename Params::mapped_type;
  using value_type = typename Params::value_type;
  using size_type = typename Params::size_type;
  using difference_type = typename Params::difference_type;

  // --- Comparison ---
  using key_compare = typename Params::key_compare;
  using is_key_compare_to = typename Params::is_key_compare_to;

  // --- Pointer / reference ---
  using pointer = typename Params::pointer;
  using const_pointer = typename Params::const_pointer;
  using reference = typename Params::reference;
  using const_reference = typename Params::const_reference;

  // --- Iterators ---
  using iterator = WTreeIterator<node_type, reference, pointer>;
  using const_iterator = typename iterator::const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  // --- Allocator ---
  using allocator_type = typename Params::allocator_type;
  using allocator_traits = typename Params::allocator_traits;
  using internal_allocator_type = typename Params::internal_allocator_type;
  using internal_allocator_traits = typename Params::internal_allocator_traits;

  // --- Static constants ---
  static constexpr bool kUnique = params_type::kUnique;
  static constexpr uint kTargetNodeBytes = params_type::kTargetNodeBytes;
  static constexpr field_type kTargetK = Params::kTargetK;
  static constexpr field_type kLastGrowth = Params::kLastGrowth;
  static constexpr uint kBasefieldsBytes = node_type::kBasefieldsBytes;
  static constexpr field_type kInitialCapacity = node_type::kInitialCapacity;

  // For result encoding of internal locate methods.
  static constexpr int kMatchMask = node_type::kMatchMask;
  static constexpr int kExactMatch = node_type::kExactMatch;
};

} // namespace WTreeLib

#endif
