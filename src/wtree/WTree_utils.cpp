#include "WTree.h"

using namespace std;
using namespace WTreeLib;

template <typename T> char *WTree<T>::pcharParamsPrefix() {
  char *sufix = new char[48];
  sprintf(sufix, "%d", WTREE_TARGET_NODE_BYTES);
  return sufix;
}

/**
  Traverse the tree from the root,
  retreaving it's values in ascendent order (inorder).
*/
template <typename T> vector<T> WTree<T>::getInorder(bool fill) {

  node_type *u = root;
  vector<T> vec;
  vec.reserve(node_type::c_target_k);
  getInorder(u, vec, fill);
  return vec;
}

/**
  Traverse the tree from the current node 'u',
  storing the values indorder in the vector 'vec'.
*/
template <typename T>
void WTree<T>::getInorder(const node_type *u, std::vector<T> &vec, bool &fill) {
  if (!u)
    return;

  typename node_type::field_type i;
  for (i = 0; i < u->_fields.n - 1; ++i) {
    vec.push_back(u->_fields.values[i]);
    if (u->_fields.isInternal && (u->_fields.pointers[i] != nullptr))
      getInorder(u->_fields.pointers[i], vec, fill);
  }
  vec.push_back(u->_fields.values[u->_fields.n - 1]);

  if (fill)
    for (i = u->_fields.n; i < node_type::c_target_k; ++i)
      vec.push_back(0);
}

/**
  Traverse the tree from the root and stores all the values in a
  node before moving to the next inorder node.
*/
template <typename T> vector<T> WTree<T>::getInorderByNode() {
  node_type *u = root;
  vector<T> vec;
  vec.reserve(node_type::c_target_k);
  getInorderByNode(u, vec);
  return vec;
}

/**
  Traverse the tree from the current node 'u' and stores all the
  values in a node before moving to the next inorder node.
*/
template <typename T>
void WTree<T>::getInorderByNode(const node_type *u, std::vector<T> &vec) {
  if (!u)
    return;

  typename node_type::field_type i;
  for (i = 0; i < u->_fields.n; ++i)
    vec.push_back(u->_fields.values[i]);

  if (!(u->_fields.isInternal))
    return;

  for (i = 0; i < u->_fields.n - 1; ++i)
    getInorderByNode(u->_fields.pointers[i], vec);
}

/**
  Calculate the height of the tree
  traversing it's nodes from the root.
*/
template <typename T> int WTree<T>::getHeight() {
  if (root)
    return root->getHeight();
  return -1;
}

/**
  Call the recursive function to checkStatistics from the root of the tree.
  This inclues:
    - num_nodes, num_internals, num_leafs
    - unused_keycells: Unused cells in vector of values
    - unused_ptrscells: Unused cells in vector of pointers
    - connectivity: Connectivity is the ratio of used pointer connections over
                    all the allocated pointer cells.

*/
template <typename T> void WTree<T>::checkStatistics(MemRegister *reg) {
  if (root == nullptr)
    return;
  _checkStatistics(reg, root, 0);
  reg->evaluate<T, node_type>();
}

/**
  Calculate all the implemented statistics of the tree from a particular node.
*/
template <typename T>
void WTree<T>::_checkStatistics(MemRegister *reg, const node_type *u,
                                ulong depth) {
  assert(u);
  // First ensure the level exists
  for (size_t i = reg->levels.size(); i < depth + 1; ++i) {
    reg->levels.push_back(MemRegister::Level());
    reg->levels[i].level = i;
  }

  reg->levels[depth].keys += u->_fields.n;
  ++reg->levels[depth].num_nodes;
  reg->unused_keycells += u->_fields.capacity - u->_fields.n;

  if (u->_fields.isInternal) {
    ++reg->levels[depth].num_internals;
    for (typename node_type::field_type i = 0; i < node_type::c_target_k - 1;
         ++i) {
      if (u->_fields.pointers[i] != nullptr) {
        ++reg->levels[depth].acum_connectivity;
        _checkStatistics(reg, u->_fields.pointers[i], depth + 1);
      } else
        (reg->unused_ptrscells)++;
    }
  }
}

template <typename T> bool WTree<T>::verify() { return __verify(root); }

template <typename T> bool WTree<T>::verify(ulong expected_keys) {
  ulong total_keys = 0;
  bool ans = __verify(root, total_keys);
  if (total_keys != expected_keys) {
    printf("[VERIFY ERROR] The W-tree has [%4lu / %4lu] keys.\n", total_keys,
           expected_keys);
    ans = false;
  }
  return ans;
}

template <typename T> bool WTree<T>::__verify(node_type *u) {
  if (u == nullptr)
    return true;

  if (u->_fields.n > node_type::c_target_k) {
    printf("[VERIFY ERROR] u node has n=%lu/%lu\n", (ulong)u->_fields.n,
           (ulong)node_type::c_target_k);
    return false;
  }

  for (typename node_type::field_type i = 0; i < u->_fields.n - 1; ++i) {
    if (u->_fields.values[i] >= u->_fields.values[i + 1]) {
      printf("[VERIFY ERROR] Not in order: %lu %lu\n",
             (ulong)u->_fields.values[i], (ulong)u->_fields.values[i + 1]);
      return false;
    }

    if (u->_fields.isInternal) {
      node_type *w = u->_fields.pointers[i];
      if (w != nullptr) {
        if (u->_fields.values[i] >= w->_fields.values[0] ||
            u->_fields.values[i + 1] <= w->_fields.values[w->_fields.n - 1]) {
          printf(
              "[VERIFY ERROR] Not bounded: (u=%lu, w=%lu) , (w=%lu, u=%lu)\n",
              (ulong)u->_fields.values[i], (ulong)w->_fields.values[0],
              (ulong)w->_fields.values[w->_fields.n - 1],
              (ulong)u->_fields.values[i + 1]);
          return false;
        }
        if (!__verify(w))
          return false;
      }
    }
  }

  return true;
}

template <typename T> bool WTree<T>::__verify(node_type *u, ulong &total_keys) {
  if (u == nullptr)
    return true;

  for (typename node_type::field_type i = 0; i < u->_fields.n - 1; ++i) {
    if (u->_fields.values[i] >= u->_fields.values[i + 1]) {
      printf("[VERIFY ERROR] Not in order: %lu %lu\n",
             (ulong)u->_fields.values[i], (ulong)u->_fields.values[i + 1]);
      return false;
    }

    if (u->_fields.isInternal) {
      node_type *w = u->_fields.pointers[i];
      if (w != nullptr) {
        if (u->_fields.values[i] >= w->_fields.values[0] ||
            u->_fields.values[i + 1] <= w->_fields.values[w->_fields.n - 1]) {
          printf(
              "[VERIFY ERROR] Not bounded: (u=%lu, w=%lu) , (w=%lu, u=%lu)\n",
              (ulong)u->_fields.values[i], (ulong)w->_fields.values[0],
              (ulong)w->_fields.values[w->_fields.n - 1],
              (ulong)u->_fields.values[i + 1]);
          return false;
        }

        if (!__verify(w, total_keys))
          return false;
      }
    }
  }

  total_keys += u->_fields.n;
  return true;
}

template <typename T>
bool WTree<T>::fillNode(WTreeNode<T> **u, typename node_type::field_type size,
                        T left, T right, bool updateSize) {
  assert(size <= node_type::c_target_k);
  if (updateSize)
    this->size -= (*u)->_fields.n;
  if ((*u)->_fields.capacity < size) {
    *u = new_leaf_node(size);
  }

  T gap = (right - left) / (size + 1);
  left += gap;
  for (typename node_type::field_type i = 0; i < size; ++i, left += gap) {
    (*u)->_fields.values[i] = left;
  }

  (*u)->_fields.n = size;
  if (updateSize)
    this->size += size;
  return true;
}

template <typename T>
bool WTree<T>::fillSparsedNode(node_type *u, ushort size, T left, T right,
                               ulong expected_nodes) {
  assert(size <= u->_fields.capacity);
  assert(size > 0);
  const ulong gap = (right - left) / (expected_nodes - 1);

  u->_fields.n = size;
  for (typename node_type::field_type i = 0; i < size; ++i)
    u->_fields.values[i] = left + i * gap;

  return true;
}

template <typename T>
bool WTree<T>::eraseNodeChildren(node_type *u, bool updateSize) {
  if (!u->_fields.isInternal)
    return true;

  for (typename node_type::field_type i = 0; i < node_type::c_target_k - 1;
       ++i) {
    if (u->_fields.pointers[i] == nullptr)
      continue;
    size -= u->_fields.pointers[i]->_fields.n;
    if (u->_fields.pointers[i]->_fields.isInternal) {
      eraseNodeChildren(u->_fields.pointers[i], updateSize);
      delete_internal_node(u->_fields.pointers[i]);
    } else
      delete_leaf_node(u->_fields.pointers[i]);
    u->_fields.pointers[i] = nullptr;
  }

  return true;
}
