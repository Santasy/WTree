#include "WTree.h"

using namespace std;
using namespace WTreeLib;

template <typename T>
bool WTree<T>::insert_in_leaf(node_type *p, typename node_type::field_type pi,
                              node_type *u, T key) {
  assert(p != nullptr);
  assert(u != nullptr);
  assert(u->_fields.n < node_type::c_target_k);
  // By construction, at this point nodes must have at least one key.
  assert(u->_fields.n > 0);

  if (key >
      u->_fields.values[u->_fields.n - 1]) { // Key must be inserted at the end
    if (u->_fields.n == u->_fields.capacity) {
      // root does not need to grow
      assert(p != nullptr);
      u = growLeaf(u);
      p->_fields.pointers[pi] = u;
    }

    u->_fields.values[u->_fields.n] = key;
    ++(u->_fields.n);
    return true;
  }
  // else: key will be less or equal to values[i] at least once

  typename node_type::field_type upb; // upper-bound index
  for (upb = 0; key > u->_fields.values[upb]; ++upb)
    ;

  if (u->_fields.values[upb] == key)
    return false;

  if (u->_fields.n == u->_fields.capacity) {
    // root does not need to grow
    assert(p != nullptr);
    u = growLeaf(u);
    p->_fields.pointers[pi] = u;
  }

  u->_fields.values[u->_fields.n] = key;
  ++(u->_fields.n);

  rotate(u->_fields.values + upb, u->_fields.values + u->_fields.n - 1,
         u->_fields.values + u->_fields.n);
  return true;

  // // else (n == 0)
  // u->_fields.values[0] = key;
  // u->_fields.n = 1;
  // return true;
}

/**
  Insert all keys in 'values'.
*/
template <typename T> vector<bool> WTree<T>::insert(const vector<T> &values) {
  vector<bool> results;
  for (T val : values)
    results.push_back(_insert(nullptr, root, 0, val));
  return results;
}

/**
  Insert all keys in 'values'.
*/
template <typename T>
vector<bool> WTree<T>::insert(const T *&values, size_t n) {
  vector<bool> results;
  for (size_t i = 0; i < n; ++i)
    results.push_back(_insert(nullptr, root, 0, values[i]));
  return results;
}

/**
  Insert key 'val' in the tree starting at 'u'. Due to some rules applied, is
  required to keep track of the previus ascendent node 'p'
  and the index 'pi' of 'u' inside 'p'.

  Returns true if the key was inserted.
*/
template <typename T>
bool WTree<T>::_insert(WTreeNode<T> *p, WTreeNode<T> *u,
                       typename node_type::field_type pi, T val) {
  while (u != nullptr) {

    if (u->_fields.n < node_type::c_target_k) {
      // R2: There is space left
      return insert_in_leaf(p, pi, u, val);
    }

    // u->n == k

    // R1: Swap laterals if needed
    typename node_type::field_type ui = 0;
    if (val < u->_fields.values[0]) {
      swap(val, u->_fields.values[0]);
    } else if (val > u->_fields.values[node_type::c_target_k - 1]) {
      swap(val, u->_fields.values[node_type::c_target_k - 1]);
      ui = node_type::c_target_k - 2; // In a set, this is garanteed to be in
                                      // the last valid pointer index
    } else {
      // Calculate position of 'val' in 'u'
      for (ui = 0; val > u->_fields.values[ui]; ++ui)
        ;

      if (
          // ui == u->_fields.n || // Exp: Esto no ocurre gracias al swap
          u->_fields.values[ui] == val)
        return false;
      --ui;
    }

    // Now we have a key that can be inside W(u).

    // Then:
    //  - if u is a leaf, we must try to insert horizontally
    //  - else it is an internal node, we must try to descend

    // Root will always be internal?
    if (u->_fields.isInternal == true) {
      // if 'u' isInternal, the next 'u' can exist or be null.
      p = u;
      u = u->_fields.pointers[ui];
      pi = ui;
      continue;
    }

    // Else: u is a leaf
    // - it could not be the root, so p must exists
    assert(p != nullptr);

    // First, try to slide and split.
    // Else, u must become an internal node.
    // R3: BalancedSlideSide
    if (trySlide(u, ui, p, pi, val) == true)
      return true;

    // R4: SplitSide
    if (trySplit(u, ui, p, pi, val) == true)
      return true;

    // Else: u must be internal and connect to a new node
    u = makeInternal(u);
    p->_fields.pointers[pi] = u;
    p = u;
    pi = ui;
    break;
  }

  // R5: Create the node
  u = new_leaf_node();
  u->_fields.values[0] = val;
  u->_fields.n = 1;

  if (p != nullptr) { // u node will not be root if p exists
    p->_fields.pointers[pi] = u;
    return true;
  }
  //  else: create root
  root = u;
  return true;
}

/**
  Tries to split the node 'u' in two only if an adyacent node is available.

  Returns true if the process was successful.
*/
template <typename T>
bool WTree<T>::trySplit(node_type *u,
                        const typename node_type::field_type lower_bound,
                        node_type *p, const typename node_type::field_type pi,
                        T val) {
  assert(p != nullptr && p->_fields.isInternal == true);
  assert(u != nullptr && u->_fields.isInternal == false);

  const typename node_type::field_type m = node_type::c_target_k >> 1;
  // The key at index 'm' of 'u' must be put in node 'p'.

  WTreeNode<T> *v;

  const bool rightIsValid = (pi < node_type::c_target_k - 2) &&
                            (p->_fields.pointers[pi + 1] == nullptr);
  if (rightIsValid) {
    T ival;
    v = new_leaf_node(node_type::c_target_k);
    p->_fields.pointers[pi + 1] = v;

    assert(val <
           u->_fields.values[node_type::c_target_k - 1]); // If moveRightmost
    ival = u->_fields.values[node_type::c_target_k - 1];  // rightmost

    --(u->_fields.n);
    u->insertAt(val, lower_bound);

    move(u->_fields.values + m + 1, u->_fields.values + node_type::c_target_k,
         v->_fields.values);

    u->_fields.n = m; // Lazy delete

    v->_fields.values[node_type::c_target_k - m - 1] = ival;
    v->_fields.values[node_type::c_target_k - m] = p->_fields.values[pi + 1];
    v->_fields.n = node_type::c_target_k - m + 1;

    p->_fields.values[pi + 1] = u->_fields.values[m]; // This is kinda illegal

    return true;
  }

  const bool leftIsValid = (pi > 0) && (p->_fields.pointers[pi - 1] == nullptr);
  if (leftIsValid) {
    v = new_leaf_node(node_type::c_target_k);
    p->_fields.pointers[pi - 1] = v;

    v->_fields.values[0] =
        p->_fields.values[pi]; // The topkey in p is garanteed to be first.
    v->_fields.values[1] = u->_fields.values[0];

    move(u->_fields.values + 1, u->_fields.values + lower_bound + 1,
         u->_fields.values);
    u->_fields.values[lower_bound] = val;

    move(u->_fields.values, u->_fields.values + m - 1, v->_fields.values + 2);
    v->_fields.n = m + 1; // garanteed to be this size

    p->_fields.values[pi] = u->_fields.values[m - 1];

    move(u->_fields.values + m, u->_fields.values + node_type::c_target_k,
         u->_fields.values);
    u->_fields.n -= m;

    return true;
  }

  return false;
}

/**
  Tries to slide a key from the node 'u' to some adyascent node 'v'
  that exists and has space lef.

  Returns true if the process was successful.
*/
template <typename T>
bool WTree<T>::trySlide(node_type *u,
                        const typename node_type::field_type lower_bound,
                        node_type *p, const typename node_type::field_type pi,
                        T val) {
  WTreeNode<T> *v = nullptr;

  assert(p != nullptr && p->_fields.isInternal == true);
  assert(u != nullptr && u->_fields.isInternal == false);
  // This is garanteed:
  assert(u->_fields.values[0] < val &&
         val < u->_fields.values[node_type::c_target_k - 1]);

  const bool rightIsValid =
      (pi < node_type::c_target_k - 2) &&
      (p->_fields.pointers[pi + 1] != nullptr) &&
      (p->_fields.pointers[pi + 1]->_fields.n < node_type::c_target_k);
  if (rightIsValid) {
    v = p->_fields.pointers[pi + 1];
    const typename node_type::field_type right =
        (node_type::c_target_k >> 1) + (v->_fields.n >> 1) + 1;
    const typename node_type::field_type v_newsize =
        node_type::c_target_k + 1 - right + v->_fields.n;

    v = growLeafToSize<typename node_type::field_type>(v, v_newsize);
    p->_fields.pointers[pi + 1] = v;
    move_backward(v->_fields.values, v->_fields.values + v->_fields.n,
                  v->_fields.values + v_newsize);

    v->_fields.values[node_type::c_target_k - right] =
        p->_fields.values[pi + 1];
    v->_fields.n = v_newsize;
    u->_fields.n = right;

    if (lower_bound == right - 1) { // val must be in top
      p->_fields.values[pi + 1] = val;
      move(u->_fields.values + right, u->_fields.values + node_type::c_target_k,
           v->_fields.values);
      return true;

    } else if (lower_bound > right - 1) { // val must be in v
      p->_fields.values[pi + 1] = u->_fields.values[right];
      move(u->_fields.values + right + 1, u->_fields.values + lower_bound + 1,
           v->_fields.values);
      v->_fields.values[lower_bound - right] = val;
      move(u->_fields.values + lower_bound + 1,
           u->_fields.values + node_type::c_target_k,
           v->_fields.values + lower_bound - right + 1);
      return true;
    }

    // else // val must be in u
    move(u->_fields.values + right, u->_fields.values + node_type::c_target_k,
         v->_fields.values);

    p->_fields.values[pi + 1] = u->_fields.values[right - 1];
    move_backward(u->_fields.values + lower_bound + 1,
                  u->_fields.values - 1 + right, u->_fields.values + right);
    u->_fields.values[lower_bound + 1] = val;
    return true;
  }

  const bool leftIsValid =
      (pi > 0) && (p->_fields.pointers[pi - 1] != nullptr) &&
      (p->_fields.pointers[pi - 1]->_fields.n < node_type::c_target_k);
  if (leftIsValid) {
    v = p->_fields.pointers[pi - 1];

    // The values from this position can remain in the node u
    const typename node_type::field_type right = node_type::c_target_k - 1 -
                                                 (node_type::c_target_k >> 1) -
                                                 (v->_fields.n >> 1);

    v = growLeafToSize<typename node_type::field_type>(v, v->_fields.n + right +
                                                              1);
    p->_fields.pointers[pi - 1] = v;

    // inserta topval al final de v (garantizado)
    v->_fields.values[v->_fields.n] = p->_fields.values[pi];

    if (lower_bound + 1 == right) { // then val must be on top
      // v insert u[:r]
      // top = val
      // move in u
      move(u->_fields.values, u->_fields.values + right,
           v->_fields.values + v->_fields.n + 1);
      p->_fields.values[pi] = val;
      move(u->_fields.values + right, u->_fields.values + node_type::c_target_k,
           u->_fields.values);
      v->_fields.n += right + 1;
      u->_fields.n -= right;
      return true;

    } else if (lower_bound + 1 < right) { // val must be in 'v'
      // v insert u[:lb+1]
      // v insert val
      // v insert u[lb+1:r-1]
      // top = u[r-1]
      // move in u

      move(u->_fields.values, u->_fields.values + lower_bound + 1,
           v->_fields.values + v->_fields.n + 1);
      // v.n += lower_bound + 1

      v->_fields.values[v->_fields.n + 1 + lower_bound + 1] = val;
      // v.n ++

      move(u->_fields.values + lower_bound + 1, u->_fields.values + right - 1,
           v->_fields.values + v->_fields.n + 1 + lower_bound + 2);
      // v.n += right - 1 - lower_bound - 1
      // v.n += right - lower_bound - 2

      p->_fields.values[pi] = u->_fields.values[right - 1];

      move(u->_fields.values + right, u->_fields.values + node_type::c_target_k,
           u->_fields.values);

      v->_fields.n += right + 1;
      u->_fields.n -= right;
      return true;
    }

    // else // val must be in 'u'
    // v insert u[:r]
    // top = u[r]
    // move u[r+1:lb+1] to u[0]
    // u[lb-r] = val
    // move u[lb+1:] to u[lb+1-r]

    move(u->_fields.values, u->_fields.values + right,
         v->_fields.values + v->_fields.n + 1);
    // v.n += right

    p->_fields.values[pi] = u->_fields.values[right];

    move(u->_fields.values + right + 1, u->_fields.values + lower_bound + 1,
         u->_fields.values);

    u->_fields.values[lower_bound - right] = val;

    move(u->_fields.values + lower_bound + 1,
         u->_fields.values + node_type::c_target_k,
         u->_fields.values + lower_bound + 1 - right);

    u->_fields.n -= right;
    v->_fields.n += right + 1;

    return true;
  }
  return false;
}