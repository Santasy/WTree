#ifndef _WTREE_TRAITS__H_
#define _WTREE_TRAITS__H_

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>

// === User setup ===
// These parameters may be configured by compilation flags.

// * [FEATURE] Use this flag to debug the iterator constructor calls:
// WTREE_DBG_ITERATORS

// * [FEATURE] Use this flag for additional debug-related messages.
// DBGVERBOSE

#ifndef WTREE_TARGET_NODE_BYTES
#define WTREE_TARGET_NODE_BYTES 512
#endif

#ifndef WTREE_TARGET_INITIAL_BYTES

#if WTREE_TARGET_NODE_BYTES > 64
#define WTREE_TARGET_INITIAL_BYTES 64
#elif WTREE_TARGET_NODE_BYTES >= 16
#define WTREE_TARGET_INITIAL_BYTES 16
#else
#define WTREE_TARGET_INITIAL_BYTES WTREE_TARGET_NODE_BYTES
#endif

#endif

// Binary search thresholds: minimum number of elements to justify binary
// search.
// For complex key types (strings, objects), binary search is efficient
// with fewer elements due to expensive comparisons.
// For numeric key types (int, float), comparisons are cheap so linear
// search with sequential memory access is preferred until
// more elements are present.

#ifndef WTREE_BINARY_SEARCH_THRESHOLD_COMPLEX
#define WTREE_BINARY_SEARCH_THRESHOLD_COMPLEX 32
#endif

#ifndef WTREE_BINARY_SEARCH_THRESHOLD_NUMERIC
#define WTREE_BINARY_SEARCH_THRESHOLD_NUMERIC 256
#endif

// === End of user setup ===
// =========================

// Define a function for the next capacity of a leaf node.
// Current keeps n/k >= 80%.
#define NEW_SIZE(current_size)                                                 \
  current_size + (current_size >= 4 ? (current_size) >> 2 : 1)

#define SAFE_NEW_SIZE(current_size, limit, k)                                  \
  current_size < limit ? NEW_SIZE(current_size) : k

namespace WTreeLib {

// ============================================================================
// Rules & Size Traits
// ============================================================================

/**
This helper struct helps to validate assumptions over the implementation
on compile time. A change in this priorities must be carefully deliverated.
*/
struct WTreeRulesAssumptions {
  static const bool use_slide = true;
  static const bool use_split = false;
  // Assumptions for efficient performance.
  static const bool slide_over_split = true;
  static const bool slide_to_right_first = true;
  static const bool split_to_right_first = true;
};

template <typename Key, int TargetNodeBytes = WTREE_TARGET_NODE_BYTES,
          typename ValueType = Key>
struct WTreeSizeHelper {
public:
  using key_type = Key;
  using value_type = ValueType;
  using difference_type = std::ptrdiff_t;

  static constexpr uint kTargetBytes = TargetNodeBytes;
  static constexpr ushort value_size = sizeof(value_type);

  // Helper to compute aligned offset for values array.
  // This accounts for padding between base_fields and values due to
  // alignment requirements of value_type.
  static constexpr size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
  }

  // Calculate the actual size of base_fields struct for a given field_type.
  // The struct size must account for internal alignment
  // and end padding.
  template <typename FieldType>
  static constexpr size_t calculate_base_fields_size() {
    // Layout: field_type, field_type, bool
    // Each member is placed at its natural alignment
    constexpr size_t field_size = sizeof(FieldType);
    constexpr size_t field_align = alignof(FieldType);
    constexpr size_t bool_size = sizeof(bool);
    constexpr size_t bool_align = alignof(bool);

    // First field_type at offset 0
    constexpr size_t offset1 = field_size;
    // Second field_type aligned after first
    constexpr size_t offset2 = align_up(offset1, field_align) + field_size;
    // bool aligned after second field_type
    constexpr size_t offset3 = align_up(offset2, bool_align) + bool_size;
    // Struct size padded to alignment of largest member
    constexpr size_t struct_align = std::max(field_align, bool_align);
    return align_up(offset3, struct_align);
  }

  // Calculate total base size when followed by value_type array
  // (accounts for padding between base_fields and the values array)
  template <typename FieldType>
  static constexpr size_t calculate_aligned_base_size() {
    constexpr size_t base_size = calculate_base_fields_size<FieldType>();
    return align_up(base_size, alignof(value_type));
  }

  /**
    Determines the optimal k-value using target_node_bytes.
    First tries a small FieldType of uchar, but if the number of keys
    is bigger than uchar, a short type is used.
   */
  template <typename ValType = value_type>
  static constexpr int determine_optimal_k(int target_node_bytes) {
    if (target_node_bytes <= 0)
      return 0;

    // Try with small field_type (uint8_t) first
    constexpr size_t small_base_size =
        calculate_aligned_base_size<unsigned char>();
    const int max_available_bytes = target_node_bytes - small_base_size;
    const int k_for_small_base = max_available_bytes / value_size;
    if (k_for_small_base < 255)
      return k_for_small_base > 3 ? k_for_small_base : 3;

    // Need larger field_type (uint16_t) for k >= 255
    constexpr size_t large_base_size =
        calculate_aligned_base_size<unsigned short>();
    const int min_available_bytes = target_node_bytes - large_base_size;
    const int k_for_big_base = min_available_bytes / value_size;
    return k_for_big_base > 3 ? k_for_big_base : 3;
  }

  // N ->S + S/4
  // N ->S 5/4
  // N 4/5 -> S
  static constexpr int determine_last_growth_limit(int limit) {
    return (limit / 5) << 2;
  }
};

// ============================================================================
// Key Comparison Traits — tag dispatch, adapters, and comparers
// ============================================================================

// A helper type used to indicate that a key-compare-to functor has been
// provided. A user can specify a key-compare-to functor by doing:
//
//  struct MyStringComparer
//      : public util::wtree::wtree_key_compare_to_tag {
//    int operator()(const string& a, const string& b) const {
//      return a.compare(b);
//    }
//  };
//
// Note that the return type is an int and not a bool. There is a
// COMPILE_ASSERT which enforces this return type.
struct WTreeKeyCompareToTag {};

// A helper class that indicates if the Compare parameter is derived from
// wtree_key_compare_to_tag.
template <typename Compare>
struct WTreeIsKeyCompareTo
    : public std::is_convertible<Compare, WTreeKeyCompareToTag> {};

// A helper class to convert a boolean comparison into a three-way
// "compare-to" comparison that returns a negative value to indicate
// less-than, zero to indicate equality and a positive value to
// indicate greater-than. This helper class is specialized for
// less<string> and greater<string>.
//
// The wtree_key_compare_to_adapter class is provided for efficiency seek.
template <typename Compare> struct WTreeKeyCompareToAdapter : Compare {
  WTreeKeyCompareToAdapter() {}
  WTreeKeyCompareToAdapter(const Compare &c) : Compare(c) {}
  WTreeKeyCompareToAdapter(const WTreeKeyCompareToAdapter<Compare> &c)
      : Compare(c) {}
};

template <>
struct WTreeKeyCompareToAdapter<std::less<std::string>>
    : public WTreeKeyCompareToTag {
  WTreeKeyCompareToAdapter() {}
  WTreeKeyCompareToAdapter(const std::less<std::string> &s) {}
  WTreeKeyCompareToAdapter(
      const WTreeKeyCompareToAdapter<std::less<std::string>> &s) {}
  int operator()(const std::string &a, const std::string &b) const {
    return a.compare(b);
  }
};

template <>
struct WTreeKeyCompareToAdapter<std::greater<std::string>>
    : public WTreeKeyCompareToTag {
  WTreeKeyCompareToAdapter() {}
  WTreeKeyCompareToAdapter(const std::greater<std::string> &s) {}
  WTreeKeyCompareToAdapter(
      const WTreeKeyCompareToAdapter<std::greater<std::string>> &s) {}
  int operator()(const std::string &a, const std::string &b) const {
    return b.compare(a);
  }
};

// A helper class that allows a compare-to functor to behave like a plain
// compare functor. This specialization is used when we do not have a
// compare-to functor.
template <typename Key, typename Compare, bool HaveCompareTo>
struct WTreeKeyComparer {
  WTreeKeyComparer() {}
  WTreeKeyComparer(Compare c) : comp(c) {}
  static bool bool_compare(const Compare &comp, const Key &x, const Key &y) {
    return comp(x, y);
  }
  bool operator()(const Key &x, const Key &y) const {
    return bool_compare(comp, x, y);
  }
  Compare comp;
};

// A specialization of wtree_key_comparer when a compare-to functor is
// present. We need a plain (boolean) comparison in some parts of the wtree
// code, such as insert-with-hint.
template <typename Key, typename Compare>
struct WTreeKeyComparer<Key, Compare, true> {
  WTreeKeyComparer() {}
  WTreeKeyComparer(Compare c) : comp(c) {}
  static bool bool_compare(const Compare &comp, const Key &x, const Key &y) {
    return comp(x, y) < 0;
  }
  bool operator()(const Key &x, const Key &y) const {
    return bool_compare(comp, x, y);
  }
  Compare comp;
};

// A helper function to compare to keys using the specified compare
// functor. This dispatches to the appropriate wtree_key_comparer comparison,
// depending on whether we have a compare-to functor or not (which depends on
// whether Compare is derived from wtree_key_compare_to_tag).
template <typename Key, typename Compare>
static bool wtree_compare_keys(const Compare &comp, const Key &x,
                               const Key &y) {
  typedef WTreeKeyComparer<Key, Compare, WTreeIsKeyCompareTo<Compare>::value>
      key_comparer;
  return key_comparer::bool_compare(comp, x, y);
}

// ============================================================================
// Key Extraction Traits — tag dispatch for set vs map value extraction
// ============================================================================

struct WTreeExtractKeyFailTag {};
struct WTreeExtractKeySelfTag {};
struct WTreeExtractKeyFirstTag {};

template <typename ValueType, typename Key,
          typename RawValueType = typename std::remove_const<
              typename std::remove_reference<ValueType>::type>::type>
struct WTreeCanExtractKey
    : std::conditional<std::is_same<RawValueType, Key>::value,
                       WTreeExtractKeySelfTag, WTreeExtractKeyFailTag>::type {};

template <typename PairType, typename Key, typename First, typename Second>
struct WTreeCanExtractKey<PairType, Key, std::pair<First, Second>>
    : std::conditional<
          std::is_same<typename std::remove_const<First>::type, Key>::value,
          WTreeExtractKeyFirstTag, WTreeExtractKeyFailTag>::type {};

// wtree_can_extract_map_key uses true_type/false_type instead of the tags.
// It returns true if Key != ContainerValueType (the container is a map not a
// set) and ValueType == Key.
template <typename ValueType, typename Key, typename ContainerValueType,
          typename RawValueType = typename std::remove_const<
              typename std::remove_reference<ValueType>::type>::type>
struct WTreeCanExtractMapKey
    : std::integral_constant<bool, std::is_same<RawValueType, Key>::value> {};

// This specialization returns wtree_extract_key_fail_tag for non-map
// containers because Key == ContainerValueType
template <typename ValueType, typename Key, typename RawValueType>
struct WTreeCanExtractMapKey<ValueType, Key, Key, RawValueType>
    : std::false_type {};

// ============================================================================
// Search Dispatch — bound adapters and search strategy selection
// ============================================================================

// An adapter class that converts a lower-bound compare into an upper-bound
// compare.
template <typename Key, typename Compare>
struct WTreeUpperBoundAdapter : public Compare {
  WTreeUpperBoundAdapter(Compare c) : Compare(c) {}
  bool operator()(const Key &a, const Key &b) const {
    return !static_cast<const Compare &>(*this)(b, a);
  }
};

// ============================================================================
// Utilities
// ============================================================================

template <typename T> inline void wtree_swap_helper(T &a, T &b) {
  using std::swap;
  swap(a, b);
}

} // namespace WTreeLib
#endif
