/*
 * Copyright (c) 2026 Sebastián Pacheco Cáceres
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WTREE_TYPE_ALIASES__H_
#define _WTREE_TYPE_ALIASES__H_

#include "iterator.hpp"
#include "node.hpp"

#include <iterator>

namespace WTreeLib {

/**
 * @brief Shared type aliases for WTree, WTreeLocator, and WTreeNodeManager.
 * @details Inherit (protected) and re-export with `using typename` as needed.
 * @tparam Params A @ref WTree "WTree<Params>" instantiation.
 */
template <typename Params> struct WTreeTypeAliases {
    using params_type = Params;
    using node_type = WTreeNode<params_type>;

    // --- Node layout types ---
    using base_fields_type = node_type::base_fields;
    using leaf_fields_type = node_type::leaf_fields;
    using internal_fields_type = node_type::internal_fields;

    // --- Key / value types ---
    using field_type = params_type::field_type;
    using key_type = params_type::key_type;
    using data_type = params_type::data_type;
    using mapped_type = params_type::mapped_type;
    using value_type = params_type::value_type;
    using size_type = params_type::size_type;
    using difference_type = params_type::difference_type;

    // --- Comparison ---
    using key_compare = params_type::key_compare;
    using is_key_compare_to = params_type::is_key_compare_to;

    // --- Pointer / reference ---
    using pointer = params_type::pointer;
    using const_pointer = params_type::const_pointer;
    using reference = params_type::reference;
    using const_reference = params_type::const_reference;

    // --- Iterators ---
    using iterator = WTreeIterator<node_type, reference, pointer>;
    using const_iterator = iterator::const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = std::reverse_iterator<iterator>;

    // --- Allocator ---
    using allocator_type = params_type::allocator_type;
    using allocator_traits = params_type::allocator_traits;
    using internal_allocator_type = params_type::internal_allocator_type;
    using internal_allocator_traits = params_type::internal_allocator_traits;

    // --- Static constants ---
    static constexpr bool kUnique = params_type::kUnique;
    static constexpr uint kTargetNodeBytes = params_type::kTargetNodeBytes;
    static constexpr field_type kTargetK = params_type::kTargetK;
    static constexpr field_type kLastGrowth = params_type::kLastGrowth;
    static constexpr uint kBasefieldsBytes = node_type::kBasefieldsBytes;
    static constexpr field_type kInitialCapacity = node_type::kInitialCapacity;

    // For result encoding of internal locate methods.
    static constexpr int kMatchMask = node_type::kMatchMask;
    static constexpr int kExactMatch = node_type::kExactMatch;
};

} // namespace WTreeLib

#endif
