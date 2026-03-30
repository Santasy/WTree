#ifndef _WTREE_CONTAINERS__H_
#define _WTREE_CONTAINERS__H_

#include "index.hpp"

#include <utility>

namespace WTreeLib {

/**
 * @brief This struct holds all types and constants from required types and
 * node byte-sizes.
 */
template <typename Key, typename Compare, typename Alloc,
          int TargetNodeBytes = WTREE_TARGET_NODE_BYTES,
          typename ValueType = Key, bool HoldsUnique = true>
struct WTreeCommonParams {
public:
  // Whether this container enforces unique keys (set/map) or allows
  // duplicates (multiset/multimap).
  static constexpr bool kUnique = HoldsUnique;
  // If Compare is derived from wtree_key_compare_to_tag then use it as the
  // key_compare type. Otherwise, use wtree_key_compare_to_adapter<> which
  // will fall-back to Compare if we don't have an appropriate specialization.
  typedef std::conditional_t<WTreeIsKeyCompareTo<Compare>::value, Compare,
                             WTreeKeyCompareToAdapter<Compare>>
      key_compare;
  // A type which indicates if we have a key-compare-to functor or a plain old
  // key-compare functor.
  using is_key_compare_to = WTreeIsKeyCompareTo<key_compare>;

  using key_type = Key;
  using value_type = ValueType;

  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;

  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using allocator_type = Alloc;
  using allocator_traits = std::allocator_traits<allocator_type>;

  // Internal allocator for node storage
  using internal_allocator_type =
      typename allocator_traits::template rebind_alloc<char>;
  using internal_allocator_traits =
      std::allocator_traits<internal_allocator_type>;

  using size_helper = WTreeSizeHelper<key_type, TargetNodeBytes, value_type>;

  static constexpr uint kTargetNodeBytes = TargetNodeBytes;
  static constexpr uint kTargetK =
      size_helper::determine_optimal_k(kTargetNodeBytes);
  static constexpr uint kLastGrowth =
      size_helper::determine_last_growth_limit(kTargetK);

  typedef typename std::conditional <
      kTargetK<255, uint8_t, uint16_t>::type field_type;

  // Binary search thresholds based on key type complexity.
  static constexpr bool is_numeric_key =
      std::is_integral<key_type>::value ||
      std::is_floating_point<key_type>::value;
  static constexpr uint kBinarySearchThreshold =
      is_numeric_key ? WTREE_BINARY_SEARCH_THRESHOLD_NUMERIC
                     : WTREE_BINARY_SEARCH_THRESHOLD_COMPLEX;

  // Whether binary search should be used for full internal nodes.
  // Internal nodes are always full with kTargetK elements.
  static constexpr bool kUseBinarySearchForInternal =
      kTargetK >= kBinarySearchThreshold;
};

/**
 * A common base class for WTreeLib::set, map, multiset and multimap.
 * @tparam Tree A @ref WTree "WTree<Params>" instantiation, where Params is
 *         @ref WTreeSetParams or @ref WTreeMapParams.
 */
template <typename Tree> class WTreeContainer {

public:
  using wtree_type = Tree;

  using params_type = typename Tree::params_type;
  using key_type = typename Tree::key_type;
  using value_type = typename Tree::value_type;
  using key_compare = typename Tree::key_compare;
  using allocator_type = typename Tree::allocator_type;
  using pointer = typename Tree::pointer;
  using const_pointer = typename Tree::const_pointer;
  using reference = typename Tree::reference;
  using const_reference = typename Tree::const_reference;
  using size_type = typename Tree::size_type;
  using iterator = typename Tree::iterator;
  using const_iterator = typename Tree::const_iterator;
  using reverse_iterator = typename Tree::reverse_iterator;
  using const_reverse_iterator = typename Tree::const_reverse_iterator;

  static constexpr int kExactMatch = Tree::kExactMatch;
  static constexpr int kMatchMask = Tree::kMatchMask;

protected:
  Tree m_tree;

public:
  // Default constructor.
  WTreeContainer(const key_compare &comp = key_compare(),
                 const allocator_type &alloc = allocator_type())
      : m_tree(comp, alloc) {}

  // Copy/move/assign — compiler-generated versions forward correctly.
  WTreeContainer(const WTreeContainer &) = default;
  WTreeContainer(WTreeContainer &&) noexcept = default;
  WTreeContainer &operator=(const WTreeContainer &) = default;
  WTreeContainer &operator=(WTreeContainer &&) noexcept = default;

  //** Returns a pointer to the underlying tree structure that manages the
  // data.
  //  */
  Tree *tree() noexcept { return &m_tree; };
  void clear() noexcept { m_tree.clear(); }

  void swap(WTreeContainer &other) noexcept { m_tree.swap(other.m_tree); }

  // Iterator routines.
  iterator begin() noexcept { return m_tree.begin(); }
  const_iterator begin() const noexcept { return m_tree.begin(); }
  const_iterator cbegin() const noexcept { return m_tree.cbegin(); }
  iterator end() noexcept { return m_tree.end(); }
  const_iterator end() const noexcept { return m_tree.cend(); }
  const_iterator cend() const noexcept { return m_tree.cend(); }
  reverse_iterator rbegin() noexcept { return m_tree.rbegin(); }
  const_reverse_iterator rbegin() const noexcept { return m_tree.rbegin(); }
  const_reverse_iterator crbegin() const noexcept { return m_tree.crbegin(); }
  reverse_iterator rend() noexcept { return m_tree.rend(); }
  const_reverse_iterator rend() const noexcept { return m_tree.rend(); }
  const_reverse_iterator crend() const noexcept { return m_tree.crend(); }

  allocator_type get_allocator() const noexcept {
    return allocator_type(m_tree.allocator());
  }

  key_compare key_comp() const { return m_tree.key_comp(); }

  size_type size() const noexcept { return m_tree.size(); }
  size_type max_size() const noexcept { return m_tree.max_size(); }
  bool empty() const noexcept { return m_tree.is_empty(); }

  // === Lookup routines (shared for unique and multi) ===

  // bool contains(const key_type &key) const { return find(key) != cend(); }

  iterator lower_bound(const key_type &key) {
    iterator iter(m_tree.root(), 0);
    return m_tree.lower_bound(key, iter).first;
  }
  const_iterator lower_bound(const key_type &key) const {
    const_iterator iter(m_tree.root(), 0);
    return m_tree.lower_bound(key, iter).first;
  }

  iterator upper_bound(const key_type &key) {
    iterator iter(m_tree.root(), 0);
    return m_tree.upper_bound(key, iter).first;
  }
  const_iterator upper_bound(const key_type &key) const {
    const_iterator iter(m_tree.root(), 0);
    return m_tree.upper_bound(key, iter).first;
  }

  std::pair<iterator, iterator> equal_range(const key_type &key) {
    return {lower_bound(key), upper_bound(key)};
  }
  std::pair<const_iterator, const_iterator>
  equal_range(const key_type &key) const {
    return {lower_bound(key), upper_bound(key)};
  }

  // === Deletion routines (shared) ===

  iterator erase(iterator iter) noexcept { return m_tree.erase(iter); }
  void erase(const iterator &first, const iterator &last) noexcept {
    m_tree.erase(first, last);
  }

  // === WTree-specific stats ===

  static double average_bytes_per_value() {
    return Tree::average_bytes_per_value();
  }
  double fullness() const { return m_tree.fullness(); }
  double overhead() const { return m_tree.overhead(); }
};

} // namespace WTreeLib

#endif