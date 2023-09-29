#include "WTree.h"
#include <cmath>
#include <cstdio>

using namespace std;
using namespace WTreeLib;

/**
  Traverse the tree from the root,
  retreaving it's values in ascendent order (inorder).
*/
template <typename T> vector<T> WTree<T>::getInorder(bool fill) {
  t_node *u = root;
  vector<T> vec;
  vec.reserve(k);
  getInorder(u, vec, fill);
  return vec;
}

/**
  Traverse the tree from the current node 'u',
  storing the values indorder in the vector 'vec'.
*/
template <typename T>
void WTree<T>::getInorder(const t_node *u, std::vector<T> &vec, bool &fill) {
  if (u == nullptr)
    return;

  unsigned short i;
  for (i = 0; i < u->_fields.n - 1; ++i) {
    vec.push_back(u->_fields.keys[i]);
    if (u->_fields.isInternal && (u->_fields.pointers[i] != nullptr))
      getInorder(u->_fields.pointers[i], vec, fill);
  }
  vec.push_back(u->_fields.keys[u->_fields.n - 1]);

  if (fill)
    for (i = u->_fields.n; i < k; ++i)
      vec.push_back(0);
}

/**
  Traverse the tree from the root and stores all the values in a
  node before moving to the next inorder node.
*/
template <typename T> vector<T> WTree<T>::getInorderByNode() {
  t_node *u = root;
  vector<T> vec;
  vec.reserve(k);
  getInorderByNode(u, vec);
  return vec;
}

/**
  Traverse the tree from the current node 'u' and stores all the
  values in a node before moving to the next inorder node.
*/
template <typename T>
void WTree<T>::getInorderByNode(const t_node *u, std::vector<T> &vec) {
  if (u == nullptr)
    return;

  unsigned short i;
  for (i = 0; i < u->_fields.n; ++i)
    vec.push_back(u->_fields.keys[i]);

  if (u->_fields.isInternal == false)
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
  Calculate the minimun total bytes needed for the structure.
*/
template <typename T> unsigned long WTree<T>::getTotalBytes() {
  unsigned long total = sizeof(*this);
  if (root)
    total += root->getTotalBytes();
  return total;
}

/**
  Call the recursive function to checkStatistics from the root of the tree.
*/
template <typename T> void WTree<T>::checkStatistics(MemRegister *reg) {
  if (root)
    _checkStatistics(reg, root, 0);
  reg->num_leafs = reg->num_nodes - reg->num_internals;
  reg->total_bytes = reg->num_nodes * sizeof(t_node) + // 24 bytes +
                     reg->unused_keycells * sizeof(T) +
                     reg->num_internals * (k - 1) * sizeof(t_node *);
  reg->average_bytes_per_key = (double)reg->total_bytes / reg->keys;
}

/**
  Calculate all the implemented statistics of the tree.
  This inclues:
    - num_nodes, num_internals, num_leafs
    - unused_valuecells: Unused cells in vector of values
    - unsigned long unused_pointercells: Unused cells in vector of pointers
*/
template <typename T>
void WTree<T>::_checkStatistics(MemRegister *reg, const t_node *u,
                                ulong depth) {
  ++(reg->num_nodes);
  reg->unused_keycells += u->_fields.capacity - u->_fields.n;
  reg->keys += u->_fields.n;

  reg->height = max(reg->height, depth);

  if (u->_fields.isInternal) {
    (reg->num_internals)++;
    for (ushort i = 0; i < k - 1; ++i) {
      if (u->_fields.pointers[i] != nullptr)
        _checkStatistics(reg, u->_fields.pointers[i], depth + 1);
      else
        (reg->unused_ptrscells)++;
    }
  }
}

template <typename T> bool WTree<T>::__verify(t_node *u) {
  if (u == nullptr)
    return true;

  for (ushort i = 0; i < u->_fields.n - 1; ++i) {
    if (u->_fields.keys[i] >= u->_fields.keys[i + 1]) {
#ifdef VERBOSE
      printf("[VERIFY ERROR] Not in order: %lu %lu\n",
             (ulong)u->_fields.keys[i], (ulong)u->_fields.keys[i + 1]);
#endif
      return false;
    }

    if (u->_fields.isInternal) {
      t_node *w = u->_fields.pointers[i];
      if (w != nullptr) {
        if (u->_fields.keys[i] >= w->_fields.keys[0] ||
            u->_fields.keys[i + 1] <= w->_fields.keys[w->_fields.n - 1]) {
#ifdef VERBOSE
          printf(
              "[VERIFY ERROR] Not bounded: (u=%lu, w=%lu) , (w=%lu, u=%lu)\n",
              (ulong)u->_fields.keys[i], (ulong)w->_fields.keys[0],
              (ulong)w->_fields.keys[w->_fields.n - 1],
              (ulong)u->_fields.keys[i + 1]);
#endif
          return false;
        }
      }
      if (__verify(w) == false)
        return false;
    }
  }

  return true;
}