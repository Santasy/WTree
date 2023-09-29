#include "WTree.h"
#include <algorithm>
#include <cstddef>
#include <iterator>

using namespace std;
using namespace WTreeLib;

/**
  Use the remove function with all keys in 'keys'.
*/
template <typename T> vector<bool> WTree<T>::remove(const vector<T> &keys) {
  vector<bool> results;
  for (T key : keys)
    results.push_back(remove(key));
  return results;
}

/**
  Use the remove function with all keys in 'keys'.
*/
template <typename T>
vector<bool> WTree<T>::remove(const T *&keys, unsigned int n) {
  vector<bool> results;
  for (unsigned int i = 0; i < n; ++i)
    results.push_back(remove(keys[i]));
  return results;
}

/**
  Remove 'key' from the tree.
  Returns true if 'key' was removed.
*/
template <typename T> bool WTree<T>::remove(T key) {
  t_field lwb, p_lwb = 0;
  t_node *p = nullptr, *u = nullptr;

  bool found = find(key, p, u, p_lwb, lwb);
  if (found == false) [[unlikely]]
    return false;
  --size;

  if (u->_fields.n == 1) {
    if (p == nullptr)
      root = nullptr;
    else {
      p->_fields.pointers[p_lwb] = nullptr;
      // Here we can also check if p must be leaf again.
    }
    delete u;
    return true;
  }

  if (u->_fields.isInternal) {
    // Seek valid descendent
    // keys: [ 0   1   2   3       lwb     5   6   7   8  ]
    // ptrs: [   0   1   2   lwb-1     lwb   5   6   7   8]
    ushort vpos;

    // First to the left:
    for (vpos = lwb; vpos > 0 && u->_fields.pointers[vpos - 1] == nullptr;
         --vpos)
      ;

    if (vpos > 0) { // Found pointer to the left
      t_node *v = u->_fields.pointers[vpos - 1];

      // Shift keys to right
      if (vpos != lwb)
        move_backward(u->_fields.keys + vpos, u->_fields.keys + lwb,
                      u->_fields.keys + lwb + 1);
      u->_fields.keys[vpos] = popMax(v);

      // Disconnect v
      if (v->_fields.n > 0) [[likely]]
        return true;
      delete v;
      u->_fields.pointers[vpos - 1] = nullptr;
      return true;
    }

    // Then, to the right:
    for (vpos = lwb; vpos < k - 1 && u->_fields.pointers[vpos] == nullptr;
         ++vpos)
      ;

    if (vpos < k - 1) { // Found to the right!
      t_node *v = u->_fields.pointers[vpos];

      // Shift keys to left
      if (lwb != vpos) {
        move(u->_fields.keys + lwb + 1, u->_fields.keys + vpos + 1,
             u->_fields.keys + lwb);
      }
      u->_fields.keys[vpos] = popMin(v);

      // Disconnect v
      if (v->_fields.n > 0) [[likely]]
        return true;
      delete v;
      u->_fields.pointers[vpos] = nullptr;
      return true;
    }

    // At this point, we are certain that this node is not internal
    u->setAsLeaf();
  }

  // Shift keys to left
  move(u->_fields.keys + lwb + 1, u->_fields.keys + u->_fields.n,
       u->_fields.keys + lwb);
  --(u->_fields.n);
  return true;
}

/**
  Search and pop the minimum value inside the tree.
  Note that this function does not modify this->size.
*/
template <typename T> T WTree<T>::popMin() {
  if (size == 0) [[unlikely]]
    return T(0);
  return popMin(root);
}

/**
  Search and pop the minimum value inside the tree with root 'u'.
  Note that this function does not modify this->size.
*/
template <typename T> T WTree<T>::popMin(t_node *u) {
  assert(u != nullptr);
  T minval = u->_fields.keys[0];

  if (u->_fields.isInternal == false) {
    // Move keys to the left
    move(u->_fields.keys + 1, u->_fields.keys + u->_fields.n, u->_fields.keys);
    --u->_fields.n;
    return minval;
  }

  t_field vpos;
  for (vpos = 0; vpos < k - 1 && u->_fields.pointers[vpos] == nullptr; ++vpos)
    ;
  if (vpos > 0)
    move(u->_fields.keys + 1, u->_fields.keys + vpos + 1, u->_fields.keys);

  if (vpos == k) { // Not found
    u->setAsLeaf();
    --u->_fields.n;
    return minval;
  }

  t_node *v = u->_fields.pointers[vpos];
  u->_fields.keys[vpos] = popMin(v);

  if (v->_fields.n > 0) [[likely]]
    return minval;
  delete v;
  u->_fields.pointers[vpos] = nullptr;
  return minval;
}

/**
  Search and pop the maximun value inside the tree.
  Note that this function does not modify this->size.
*/
template <typename T> T WTree<T>::popMax() {
  if (size == 0) [[unlikely]]
    return T(0);
  return popMax(root);
}

/**
  Search and pop the maximun value inside the tree with root 'u'.
  Note that this function does not modify this->size.
*/
template <typename T> T WTree<T>::popMax(t_node *u) {
  assert(u != nullptr);

  if (u->_fields.isInternal == false) {
    --u->_fields.n;
    return u->_fields.keys[u->_fields.n];
  }

  // Search pointer to left
  t_field vpos;
  for (vpos = k - 1; vpos > 0 && u->_fields.pointers[vpos - 1] == nullptr;
       --vpos)
    ;

  if (vpos == 0) { // Not found
    u->setAsLeaf();
    --u->_fields.n;
    return u->_fields.keys[u->_fields.n];
  }

  // Shift keys to the right
  T maxval = u->_fields.keys[u->_fields.n - 1];
  move_backward(u->_fields.keys + vpos, u->_fields.keys + k - 1,
                u->_fields.keys + k);
  t_node *v = u->_fields.pointers[vpos - 1];
  u->_fields.keys[vpos] = popMax(v);

  // Disconnect v
  if (v->_fields.n > 0) [[likely]]
    return maxval;
  delete v;
  u->_fields.pointers[vpos - 1] = nullptr;
  return maxval;
}