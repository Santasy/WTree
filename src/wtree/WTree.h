#ifndef __WTREECLASS_H__
#define __WTREECLASS_H__

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <type_traits>
#include <vector>

#include "WTree_defs.hpp"

namespace WTreeLib {
// [1] MemRegisterStruct
struct MemRegister {
  struct Level {
    unsigned int level = 0;
    unsigned long keys = 0;
    unsigned long num_nodes = 0;
    unsigned long num_internals = 0;
    unsigned long num_leafs = 0;
    double connectivity = 0.0;

    double acum_connectivity = 0.0;

    void evaluate(unsigned int k) {
      num_leafs = num_nodes - num_internals;
      connectivity =
          num_internals > 0 ? acum_connectivity / (num_internals * (k - 1)) : 0;
    };
  };
  std::vector<Level> levels;

  unsigned long keys = 0;
  unsigned long num_nodes = 0;
  unsigned long height = 0;
  unsigned long num_internals =
      0;                       // Number of internal nodes (with descendents)
  unsigned long num_leafs = 0; // Number of leaf nodes (no descendents)
  unsigned long unused_keycells = 0;  // Unused cells in vector of keys
  unsigned long unused_ptrscells = 0; // Unused cells in vector of pointers

  unsigned long total_bytes = 0;

  double average_bytes_per_key = 0; // Calculated as total_bytes/keys
  double connectivity = 0.0;

  template <typename T, class NODE> void evaluate() {
    // Evaluate levels:
    for (uint l = 0; l < levels.size(); ++l) {
      Level &level = levels[l];
      assert(level.level == l);
      level.evaluate(NODE::c_target_k);
      keys += level.keys;
      num_nodes += level.num_nodes;
      num_internals += level.num_internals;
      connectivity += level.acum_connectivity; // use as acum
    }

    // Calculate general values:
    height = levels.size() - 1;
    num_leafs = num_nodes - num_internals;
    connectivity = num_internals > 0
                       ? connectivity / (num_internals * (NODE::c_target_k - 1))
                       : 0;

    total_bytes = num_nodes * NODE::basefields_size +
                  unused_keycells * sizeof(T) +
                  num_internals * (NODE::c_target_k - 1) * sizeof(void *);
    average_bytes_per_key = (double)total_bytes / keys;
  };

  char *toJsonBody() {
    char *oline = new char[1024 + 512 * levels.size()];
    const double fullness = (double)keys / (keys + unused_keycells);
    int send = sprintf(oline,
                       "\"keys\": %10lu,"
                       "\"nodes\": %6lu,"
                       "\"height\": %3lu,\n"
                       "\"internals\": %4lu,"
                       "\"leafs\": %4lu,"
                       "\"unused_keycells\": %6lu,"
                       "\"unused_ptrscells\": %6lu,\n"
                       "\"total_bytes\": %10lu,"
                       "\"overhead\": %f,"
                       "\"fullness\": %f,"
                       "\"connectivity\": %f,\n",
                       keys, num_nodes, height, num_internals, num_leafs,
                       unused_keycells, unused_ptrscells, total_bytes,
                       average_bytes_per_key, fullness, connectivity);
    send += sprintf(oline + send, "\"levels\":[\n");
    for (size_t l = 0; l < levels.size(); ++l) {
      const Level &level = levels[l];
      send += sprintf(oline + send,
                      "\t{"
                      "\"level\": %3u, "
                      "\"keys\": %5lu, "
                      "\"num_nodes\": %4lu, "
                      "\"num_internals\": %4lu, "
                      "\"num_leafs\": %4lu, "
                      "\"connectivity\": %f"
                      "}%c\n",
                      level.level, level.keys, level.num_nodes,
                      level.num_internals, level.num_leafs, level.connectivity,
                      l < levels.size() - 1 ? ',' : ' ');
    }
    sprintf(oline + send, "]");
    return oline;
  };

  void clean() {
    levels.clear();
    std::fill(&keys, &keys + sizeof(MemRegister) - sizeof(std::vector<Level>),
              0);
  };
};

// [2] Node structure
template <typename T> class WTreeNode {
public:
  typedef WTreeNode<T> self_type;

  enum {
    c_target_size = WTREE_TARGET_NODE_BYTES, // [bytes]
    c_target_k =
        (c_target_size - 4 - 4 * (c_target_size > (254 * sizeof(T) + 4))) /
        sizeof(T),
    c_last_growth = LAST_GROWTH_LIMIT(c_target_k),
  };

  typedef typename std::conditional <
      c_target_k<255, uint8_t, ushort>::type field_type;

  // == Atributes
  struct base_fields {   // 3 | 5 -> 4 | 6 bytes
    field_type n;        // 1-2 byte (max = [255, 65535])
    field_type capacity; // 1-2 byte (max = [255, 65535])
    bool isInternal;     // 1 byte
  };

  struct leaf_fields : public base_fields {
    T values[c_target_k]; // c_target_k * sizeof(T)
  };

  struct internal_fields : public leaf_fields {
    self_type *pointers[c_target_k - 1]; // (c_target_k - 1) * 8
  };

  internal_fields _fields;

  enum {
    // Include padding on the basefield size
    basefields_size = sizeof(self_type::base_fields) +
                      (4 - sizeof(self_type::base_fields) % 4)
  };

  // == Node Functions

  bool insert(T val);
  inline bool insertAt(T val, field_type lower_bound) {
    // Linear search from the right
    assert(_fields.n < c_target_k);

    // Version rotate: ** This is faster than move_backwards **
    _fields.values[_fields.n] = val;
    ++_fields.n;

    std::rotate(_fields.values + lower_bound + 1,
                _fields.values + _fields.n - 1, _fields.values + _fields.n);

    return true;
  }
  std::vector<bool> insert(const std::vector<T> &values);
  std::vector<bool> insert(const T *&values, size_t n);

  bool contains(T val);
  typename self_type::field_type getPosition(T val);

  void print() const;
  int getHeight() const;

  /**
    Erase the last key inside the node, and also it's last pointer if the node
    is an internal node.

    Returns false if the node was empty.
  */
  inline void popBack() {
    assert(_fields.n > 0);
    --(_fields.n);
  }

private:
  /* WTreeNode<T>(const self_type &);
  void operator=(const self_type &); */
};

template <typename T> class WTree {
  // W-tree: Data structure for keys management.

  typedef std::allocator<T> allocator_type;
  typedef std::allocator_traits<allocator_type> allocator_traits;
  typedef typename allocator_traits::template rebind_alloc<char>
      internal_allocator_type;
  typedef std::allocator_traits<internal_allocator_type>
      internal_allocator_traits;

public:
  typedef WTreeNode<T> node_type;

  enum {
    c_initial_cap =
        (WTREE_TARGET_INITIAL_BYTES - node_type::basefields_size) / sizeof(T)
  };

  // [3] Atributes (16 -> 16 bytes)
  node_type *root;
  ulong size = 0;

  // [4] Public Functions
  WTree();
  ~WTree();

  node_type *
  new_leaf_node(typename node_type::field_type capacity = c_initial_cap);
  node_type *new_internal_node();

  inline void delete_leaf_node(node_type *&u) {
    const ulong node_size =
        node_type::basefields_size + u->_fields.capacity * sizeof(T);
    internal_allocator_type ia;
    internal_allocator_traits::deallocate(ia, reinterpret_cast<char *>(u),
                                          node_size);
  }

  inline void delete_internal_node(node_type *&u) {
    internal_allocator_type ia;
    internal_allocator_traits::deallocate(
        ia, reinterpret_cast<char *>(u),
        sizeof(typename node_type::internal_fields));
  }

  node_type *
  init_leaf(typename node_type::leaf_fields *u,
            typename node_type::field_type init_capacity = c_initial_cap);
  node_type *init_internal(typename node_type::internal_fields *u);

  inline WTreeNode<T> *makeInternal(node_type *u) {
    if (u->_fields.isInternal)
      return u;

    node_type *node = new_internal_node();
    memcpy(node->_fields.values, u->_fields.values, u->_fields.n * sizeof(T));
    node->_fields.n = u->_fields.n;
    delete_leaf_node(u);
    return node;
  }

  template <typename FT = typename node_type::field_type>
  inline typename std::enable_if<std::is_same<FT, ushort>::value,
                                 WTreeNode<T> *>::type
  growLeaf(node_type *u) {
    assert(u->_fields.capacity < node_type::c_target_k);

    const FT newcap =
        std::min<FT>(NEW_SIZE(u->_fields.n), node_type::c_target_k);
    assert(newcap > u->_fields.capacity);

    node_type *node = new_leaf_node(newcap);
    memcpy(node->_fields.values, u->_fields.values, u->_fields.n * sizeof(T));
    node->_fields.n = u->_fields.n;
    delete_leaf_node(u);
    return node;
  }

  template <typename FT = typename node_type::field_type>
  inline typename std::enable_if<std::is_same<FT, uint8_t>::value,
                                 WTreeNode<T> *>::type
  growLeaf(node_type *u) {
    assert(u->_fields.capacity < node_type::c_target_k);

    const FT newcap = SAFE_NEW_SIZE(
        u->_fields.capacity, node_type::c_last_growth, node_type::c_target_k);
    assert(newcap > u->_fields.capacity);

    node_type *node = new_leaf_node(newcap);
    memcpy(node->_fields.values, u->_fields.values, u->_fields.n * sizeof(T));
    node->_fields.n = u->_fields.n;
    delete_leaf_node(u);
    return node;
  }

  template <typename FT = typename node_type::field_type>
  inline typename std::enable_if<std::is_same<FT, ushort>::value,
                                 WTreeNode<T> *>::type
  growLeafToSize(node_type *u, FT minsize) {
    if (minsize <= u->_fields.capacity)
      return u;

    const FT newcap = std::min<FT>(NEW_SIZE(minsize), node_type::c_target_k);
    assert(newcap > u->_fields.capacity);
    assert(newcap >= minsize);

    node_type *node = new_leaf_node(newcap);
    memcpy(node->_fields.values, u->_fields.values, u->_fields.n * sizeof(T));
    node->_fields.n = u->_fields.n;
    delete_leaf_node(u);
    return node;
  }

  template <typename FT = typename node_type::field_type>
  inline typename std::enable_if<std::is_same<FT, uint8_t>::value,
                                 WTreeNode<T> *>::type
  growLeafToSize(node_type *u, FT minsize) {
    if (minsize <= u->_fields.capacity)
      return u;

    const FT newcap =
        SAFE_NEW_SIZE(minsize, node_type::c_last_growth, node_type::c_target_k);
    assert(newcap > u->_fields.capacity);
    assert(newcap >= minsize);

    node_type *node = new_leaf_node(newcap);
    memcpy(node->_fields.values, u->_fields.values, u->_fields.n * sizeof(T));
    node->_fields.n = u->_fields.n;
    delete_leaf_node(u);
    return node;
  }

  inline WTreeNode<T> *make_leaf_from_internal(node_type *u) {
    assert(u->_fields.isInternal);
    node_type *node = new_leaf_node(node_type::c_target_k);
    memcpy(node->_fields.values, u->_fields.values, u->_fields.n * sizeof(T));
    node->_fields.n = u->_fields.n;
    delete_internal_node(u);
    return node;
  }

  /**
  Insert key 'val' in the tree.

  Returns true if the key was inserted.
*/
  inline bool insert(T key) {
    const bool inserted = _insertFromRoot(key);
    size += inserted;

#ifdef WTREE_ATOMICDBG
    if (inserted && !__verify(root)) {
      printf("Key %lu destructed some node.\n", (ulong)key);
      return false;
    }
#endif

    return inserted;
  };
  std::vector<bool> insert(const std::vector<T> &values);
  std::vector<bool> insert(const T *&values, size_t n);

  bool insert_in_leaf(node_type *p, typename node_type::field_type pi,
                      node_type *u, T val);

  bool _insert(node_type *p, node_type *u, typename node_type::field_type pi,
               T val);
  bool _insertFromRoot(T val);
  bool trySplit(node_type *u, const typename node_type::field_type lower_bound,
                node_type *p, const typename node_type::field_type pi, T val);
  bool trySlide(node_type *u, const typename node_type::field_type lower_bound,
                node_type *p, const typename node_type::field_type pi, T val);

  bool find(T val) const;
  bool find(T val, node_type **op, node_type **ou,
            typename node_type::field_type &pi,
            typename node_type::field_type &ix) const;

  T popMin();
  T popMin(node_type *u);

  T popMax();
  T popMax(node_type *u);

  bool remove(T val);
  std::vector<bool> remove(const std::vector<T> &values);
  std::vector<bool> remove(const T *&values, typename node_type::field_type n);

  // [4.1] Utils

  char *pcharParamsPrefix();
  bool fillNode(node_type **u, typename node_type::field_type size, T left,
                T right, bool updateSize = true);
  bool fillSparsedNode(node_type *u, ushort size, T left, T right,
                       ulong expected_nodes);
  bool eraseNodeChildren(node_type *u, bool updateSize = true);

  int getHeight();
  void printTree();

  std::vector<T> getInorder(bool fill = false);
  std::vector<T> getInorderByNode();

  void checkStatistics(MemRegister *reg);

  bool verify();
  bool verify(ulong expected_keys);
  bool __verify(node_type *u);
  bool __verify(node_type *u, ulong &total_keys);

private:
  // [5] Private Functions
  void _recursive_delete(node_type *u);

  // [5.1] Insert (nothing)
  //    Here just for clarity.

  // [5.2] Search (nothing)
  //    Here just for clarity.

  // [5.3] Remove (nothing)
  //    Here just for clarity.

  // [5.4] Print
  static void printNode(node_type *n, const std::string &prefix, bool isLeft);

  // [5.5] Utils
  void getInorder(const node_type *u, std::vector<T> &vec, bool &fill);
  void getInorderByNode(const node_type *u, std::vector<T> &vec);
  void _checkStatistics(MemRegister *reg, const node_type *u,
                        unsigned long depth);
};

template class WTreeNode<short>;
template class WTreeNode<unsigned short>;
template class WTreeNode<int>;
template class WTreeNode<unsigned int>;
template class WTreeNode<long>;
template class WTreeNode<unsigned long>;
// template class WTreeNode<long long>;
// template class WTreeNode<unsigned long long>;
// template class WTreeNode<float>;
// template class WTreeNode<double>;

template class WTree<short>;
template class WTree<unsigned short>;
template class WTree<int>;
template class WTree<unsigned int>;
template class WTree<long>;
template class WTree<unsigned long>;
// template class WTree<long long>;
// template class WTree<unsigned long long>;
// template class WTree<float>;
// template class WTree<double>;
} // namespace WTreeLib

#endif