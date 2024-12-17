#include "WTree.h"

using namespace std;
using namespace WTreeLib;

/**
  Remove 'val' from the tree.
  Returns true if 'val' was removed.
*/
template <typename T> bool WTree<T>::remove(T val) {
  typename node_type::field_type ui, pi = 0, vpos;
  WTreeNode<T> *p = nullptr, *u = nullptr;

  bool found = find(val, &p, &u, pi, ui);

  if (__builtin_expect_with_probability(found == false, true, 0.1))
    return false;

  if (__builtin_expect_with_probability(u->_fields.n == 1, true, 0.1)) {
    if (__builtin_expect(p != nullptr, true)) {
      p->_fields.pointers[pi] = nullptr;
      if (u->_fields.isInternal)
        delete_internal_node(u);
      else
        delete_leaf_node(u);
    }

    --size;
    return true;

  } else if (u->_fields.isInternal) {
    // Seek valid descendent

    // First to the left:
    for (vpos = ui; vpos > 0 && u->_fields.pointers[vpos - 1] == nullptr;
         --vpos)
      ;

    if (vpos > 0) {                                    // Found to the left!
      WTreeNode<T> *v = u->_fields.pointers[vpos - 1]; // ok

      // Shift values to right
      move_backward(u->_fields.values + vpos, u->_fields.values + ui,
                    u->_fields.values + ui + 1);

      u->_fields.values[vpos] = popMax(v);

      // check descendencia vacia
      if (__builtin_expect_with_probability(v->_fields.n == 0, true, 0.1)) {
        u->_fields.pointers[vpos - 1] = nullptr;
        if (v->_fields.isInternal)
          delete_internal_node(v);
        else
          delete_leaf_node(v);
      }

      --size;
      return true;
    }

    // Then, to the right
    for (vpos = ui;
         vpos < u->_fields.n - 1 && u->_fields.pointers[vpos] == nullptr;
         ++vpos)
      ;

    if (vpos < u->_fields.n - 1) { // Found to the right!
      WTreeNode<T> *v = u->_fields.pointers[vpos];

      // Shift values to left
      move(u->_fields.values + ui + 1, u->_fields.values + vpos + 1,
           u->_fields.values + ui);

      u->_fields.values[vpos] = popMin(v);

      if (__builtin_expect_with_probability(v->_fields.n == 0, true, 0.1)) {
        u->_fields.pointers[vpos] = nullptr;
        if (v->_fields.isInternal)
          delete_internal_node(v);
        else
          delete_leaf_node(v);
      }

      --size;
      return true;
    }

    // At this point, we are certain that this node is not internal
    if (root != u) {
      u = make_leaf_from_internal(u);
      p->_fields.pointers[pi] = u;
    }
  }

  // Shift values to left
  move(u->_fields.values + ui + 1, u->_fields.values + u->_fields.n,
       u->_fields.values + ui);
  u->popBack();
  --size;
  return true;
}

/**
  Use the remove function with all keys in 'values'.
*/
template <typename T> vector<bool> WTree<T>::remove(const vector<T> &values) {
  vector<bool> results;
  for (T val : values)
    results.push_back(remove(val));
  return results;
}

/**
  Use the remove function with all keys in 'values'.
*/
template <typename T>
vector<bool> WTree<T>::remove(const T *&values,
                              typename node_type::field_type n) {
  vector<bool> results;
  for (typename node_type::field_type i = 0; i < n; ++i)
    results.push_back(remove(values[i]));
  return results;
}

/**
  Search and pop the minimum value inside the tree.
*/
template <typename T> T WTree<T>::popMin() {
  WTreeNode<T> *u = root;
  if (u)
    return popMin(u);

  return 0;
}

/**
  Search and pop the minimum value inside the tree with root 'u'.
*/
template <typename T> T WTree<T>::popMin(WTreeNode<T> *u) {
  assert(u != nullptr);
  assert(u->_fields.n > 0);

  const T minval = u->_fields.values[0];
  typename node_type::field_type vpos;

  if (__builtin_expect_with_probability(u->_fields.isInternal == false, true,
                                        0.1)) {
    // Move keys to the left
    move(u->_fields.values + 1, u->_fields.values + u->_fields.n,
         u->_fields.values);
    u->popBack();
    return minval;
  }

  // Search pointer to the right
  for (vpos = 0;
       vpos < node_type::c_target_k - 1 && u->_fields.pointers[vpos] == nullptr;
       ++vpos)
    ;

  if (__builtin_expect_with_probability(vpos == node_type::c_target_k - 1, true,
                                        0.1)) { // Not found
    // u->setAsLeaf(); // Node is not internal

    // Shift keys to the left
    move(u->_fields.values + 1, u->_fields.values + u->_fields.n,
         u->_fields.values);
    u->popBack();
    return minval;
  }

  // Then, 'v' is found.

  // Shift keys to left
  move(u->_fields.values + 1, u->_fields.values + vpos + 1, u->_fields.values);

  WTreeNode<T> *v = u->_fields.pointers[vpos];
  u->_fields.values[vpos] = popMin(v);
  if (__builtin_expect_with_probability(v->_fields.n == 0, true, 0.1)) {
    u->_fields.pointers[vpos] = nullptr;
    if (v->_fields.isInternal)
      delete_internal_node(v);
    else
      delete_leaf_node(v);
  }

  return minval;
}

/**
  Search and pop the maximun value inside the tree.
*/
template <typename T> T WTree<T>::popMax() {
  WTreeNode<T> *u = root;
  if (u)
    return popMax(u);

  return 0;
}

/**
  Search and pop the maximun value inside the tree with root 'u'.
*/
template <typename T> T WTree<T>::popMax(WTreeNode<T> *u) {
  assert(u != nullptr);
  assert(u->_fields.n > 0);

  const T maxval = u->_fields.values[u->_fields.n - 1];
  if (__builtin_expect_with_probability(u->_fields.isInternal == false, true,
                                        0.1)) {
    u->popBack();
    return maxval;
  }

  // Search pointer to the left
  typename node_type::field_type vpos;
  for (vpos = node_type::c_target_k - 1;
       vpos > 0 && u->_fields.pointers[vpos - 1] == nullptr; --vpos)
    ;

  if (__builtin_expect_with_probability(vpos == 0, true, 0.1)) {
    // u->setAsLeaf();
    u->popBack();
    return maxval;
  }

  // 'v' is found.
  WTreeNode<T> *v = u->_fields.pointers[vpos - 1];
  // Shift values to the right
  move_backward(u->_fields.values + vpos, u->_fields.values + u->_fields.n - 1,
                u->_fields.values + u->_fields.n);

  u->_fields.values[vpos] = popMax(v);
  if (__builtin_expect_with_probability(v->_fields.n == 0, true, 0.1)) {
    u->_fields.pointers[vpos - 1] = nullptr;
    if (v->_fields.isInternal)
      delete_internal_node(v);
    else
      delete_leaf_node(v);
  }

  return maxval;
}