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

#ifndef _WTREE_NODE__T_
#define _WTREE_NODE__T_

#include "node.hpp"

#include <algorithm>
#include <cassert>

namespace WTreeLib {

template <typename Params>
template <typename... Args>
inline void WTreeNode<Params>::internal_emplace_as_leaf(field_type i,
                                                        Args &&...args) {
    const field_type cnt = size();
    assert(i <= cnt);

    if constexpr(std::is_trivially_move_assignable_v<value_type>) {
        // Trivially move-assignable: rotate compiles to memmove.
        construct_value(cnt, std::forward<Args>(args)...);
        std::rotate(fields.values + i, fields.values + cnt,
                    fields.values + cnt + 1);
    } else {
        // Non-trivial types: shift right and construct in-place at i.
        if(i == cnt) {
            construct_value(cnt, std::forward<Args>(args)...);
        } else {
            for(field_type j = cnt; j > i; --j) {
                move_value(j - 1, j);
            }
            // destroy_value(i);
            construct_value(i, std::forward<Args>(args)...);
        }
    }
    ++fields.size;
}

// === WTREE_NODE_SEARCH ===

// Helper: true when comp(a, b) returns int (three-way / compare_to).
// When the upper_bound adapter wraps (compare_to as bool), this correctly
// returns false, routing through the plain compare path.
template <typename Params>
template <typename Compare>
constexpr bool WTreeNode<Params>::is_compare_to() {
    using result_t = std::invoke_result_t<const Compare &, const key_type &,
                                          const key_type &>;
    return std::is_same_v<result_t, int>;
}

// Return-code decoder: shared by the locator over all node searches.
template <typename Params>
template <bool UseLowerBound, typename Compare, typename IterType>
bool WTreeNode<Params>::is_exact_hit(int res, const key_type &key,
                                     IterType &iter, const Compare &comp) {
    if constexpr(!UseLowerBound)
        return false;
    if constexpr(params_type::is_key_compare_to::value)
        return (res & kExactMatch) != 0;
    return !wtree_compare_keys(comp, key, iter.key());
}

// --- Unbounded search ---

template <typename Params>
template <typename Compare>
int WTreeNode<Params>::linear_lower_bound_search(const key_type &query_key,
                                                 int s, int e,
                                                 const Compare &comp) const {
    while(s < e) {
        if constexpr(is_compare_to<Compare>()) {
            int c = comp(key(s), query_key);
            if(c == 0)
                return s | kExactMatch;
            if(c > 0)
                break;
        } else {
            if(!comp(key(s), query_key))
                break;
        }
        ++s;
    }
    return s;
}

template <typename Params>
template <typename Compare>
int WTreeNode<Params>::reversed_linear_upper_bound_search(
    const key_type &query_key, int s, int e, const Compare &comp) const {
    while(e > s) {
        --e;
        if constexpr(is_compare_to<Compare>()) {
            int c = comp(key(e), query_key);
            if(c == 0) {
                if constexpr(params_type::kUnique) {
                    // Unique: e is the only occurrence.
                    return e | kExactMatch;
                } else {
                    // Multi: continue searching left for the first occurrence.
                    while(e > s && comp(key(e - 1), query_key) == 0)
                        --e;
                    return e | kExactMatch;
                }
            }
            if(c < 0)
                return e + 1;
        } else {
            if(comp(key(e), query_key))
                return e + 1;
        }
    }
    return s;
}

template <typename Params>
template <typename Compare>
int WTreeNode<Params>::binary_lower_bound_search(const key_type &k, int s,
                                                 int e,
                                                 const Compare &comp) const {
    while(s != e) {
        int mid = (s + e) / 2;
        if constexpr(is_compare_to<Compare>()) {
            int c = comp(key(mid), k);
            if(c < 0) {
                s = mid + 1;
            } else if(c > 0) {
                e = mid;
            } else {
                // Exact match found at mid.
                if constexpr(params_type::kUnique) {
                    // Unique: mid is the only occurrence.
                    return mid | kExactMatch;
                } else {
                    // Multi: continue searching left for the first occurrence.
                    e = mid;
                    continue;
                }
            }
        } else {
            if(comp(key(mid), k)) {
                s = mid + 1;
            } else {
                e = mid;
            }
        }
    }
    return s;
}

template <typename Params>
template <typename Compare>
int WTreeNode<Params>::binary_search_any(const key_type &k, int s, int e,
                                         const Compare &comp) const {
    while(s != e) {
        int mid = (s + e) / 2;
        if constexpr(is_compare_to<Compare>()) {
            int c = comp(key(mid), k);
            if(c < 0) {
                s = mid + 1;
            } else if(c > 0) {
                e = mid;
            } else {
                return mid | kExactMatch;
            }
        } else {
            if(comp(key(mid), k)) {
                s = mid + 1;
            } else {
                e = mid;
            }
        }
    }
    return s;
}

// --- Bounded search (key within [key(0), key(size-1)]) ---

template <typename Params>
template <typename Compare>
int WTreeNode<Params>::closed_linear_lower_bound_search(
    const key_type &query_key, const Compare &comp) const {
    field_type index = 0;
    if constexpr(is_compare_to<Compare>()) {
        for(;; ++index) {
            int result = comp(key(index), query_key);
            if(result == 0)
                return index | kExactMatch;
            if(result > 0)
                break;
        }
    } else {
        for(; comp(key(index), query_key); ++index) {
        }
    }
    return index;
}

template <typename Params>
template <typename Compare>
int WTreeNode<Params>::closed_binary_lower_bound_search(
    const key_type &k, const Compare &comp) const {
    // Bounded guarantee: key >= values[0] && key <= values[size-1],
    // so the answer is in [0, size()-1].
    field_type left = 0, right = size() - 1, mid;

    while(left != right) {
        mid = (left + right) / 2;
        if constexpr(is_compare_to<Compare>()) {
            int result = comp(key(mid), k);
            if(result < 0) {
                left = mid + 1;
            } else if(result > 0) {
                right = mid;
            } else {
                // Exact match found at mid.
                if constexpr(params_type::kUnique) {
                    return mid | kExactMatch;
                } else {
                    // Multi: continue searching left for the first occurrence.
                    // Note: recursive call stays unbounded — the match
                    // guarantees the result will also be an exact match.
                    left = binary_search(k, left, mid, comp);
                    return left | kExactMatch;
                }
            }
        } else {
            if(comp(key(mid), k)) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
    }
    return left;
}
// ================================

// In-node / cross-node value relocation lives in WTreeNodeManager as
// move_values_to_node / move_values_to_blank_node.

template <typename Params>
void WTreeNode<Params>::reverse_range(value_type *first, value_type *last) {
    if constexpr(std::is_move_assignable_v<value_type>) {
        std::reverse(first, last);
    } else {
        // Manual reverse using move construction
        while(first != last && first != --last) {
            // Swap *first and *last using move construction
            alignas(value_type) char temp_storage[sizeof(value_type)];
            value_type *temp = reinterpret_cast<value_type *>(temp_storage);

            new(temp) value_type(std::move(*first));
            first->~value_type();

            new(first) value_type(std::move(*last));
            last->~value_type();

            new(last) value_type(std::move(*temp));
            temp->~value_type();

            ++first;
        }
    }
}

template <typename Params>
void WTreeNode<Params>::safe_rotate(value_type *first, value_type *middle,
                                    value_type *last) {
    if constexpr(std::is_move_assignable_v<value_type>) {
        // Use standard rotate for move-assignable types
        std::rotate(first, middle, last);
    } else {
        // Manual rotate implementation for non-move-assignable types
        // This implements the same logic as std::rotate but uses
        // construction/destruction

        if(first == middle || middle == last) {
            return; // Nothing to rotate
        }

        const size_t first_part_size = middle - first;
        const size_t second_part_size = last - middle;

        if(first_part_size == 1 && second_part_size == 1) {
            // Simple case: swap two elements
            // Create temporary storage
            alignas(value_type) char temp_storage[sizeof(value_type)];
            value_type *temp = reinterpret_cast<value_type *>(temp_storage);

            // Move first element to temp
            new(temp) value_type(std::move(*first));
            first->~value_type();

            // Move second element to first position
            new(first) value_type(std::move(*middle));
            middle->~value_type();

            // Move temp to second position
            new(middle) value_type(std::move(*temp));
            temp->~value_type();

        } else if(first_part_size == 1) {
            // Rotating one element to the right
            // Save the first element
            alignas(value_type) char temp_storage[sizeof(value_type)];
            value_type *temp = reinterpret_cast<value_type *>(temp_storage);
            new(temp) value_type(std::move(*first));
            first->~value_type();

            // Shift all elements in [middle, last) one position left
            for(value_type *it = first; it != last - 1; ++it) {
                new(it) value_type(std::move(*(it + 1)));
                (it + 1)->~value_type();
            }

            // Place the saved element at the end
            new(last - 1) value_type(std::move(*temp));
            temp->~value_type();

        } else if(second_part_size == 1) {
            // Rotating one element to the left
            // Save the middle element
            alignas(value_type) char temp_storage[sizeof(value_type)];
            value_type *temp = reinterpret_cast<value_type *>(temp_storage);
            new(temp) value_type(std::move(*middle));
            middle->~value_type();

            // Shift all elements in [first, middle) one position right
            for(value_type *it = middle - 1; it != first - 1; --it) {
                new(it + 1) value_type(std::move(*it));
                it->~value_type();
            }

            // Place the saved element at the beginning
            new(first) value_type(std::move(*temp));
            temp->~value_type();

        } else {
            reverse_range(first, middle);
            reverse_range(middle, last);
            reverse_range(first, last);
        }
    }
}

} // namespace WTreeLib
#endif