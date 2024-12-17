#include "WTree.h"

using namespace std;
using namespace WTreeLib;

/**
  Insert key 'val' in the tree starting at the root node of the tree.
  - Due to some rules applied, is required to keep track of the previus
  ascendent node 'p' and the index 'pi' of 'u' inside 'p'.

  Returns true if the key was inserted.
*/
template <typename T> bool WTree<T>::_insertFromRoot(T key) {
  node_type *p, *u;
  typename node_type::field_type ui;
  assert(root != nullptr);

  // First fill the root, to later can assume that nodes have at least one key
  // when inserting.
  if (root->_fields.n < node_type::c_target_k) {
    if (root->_fields.n > 0) {
      if (key > root->_fields.values[root->_fields.n - 1]) {
        root->_fields.values[root->_fields.n] = key;
        ++(root->_fields.n);
        return true;
      }

      for (ui = 0; key > root->_fields.values[ui]; ++ui)
        ;

      if (root->_fields.values[ui] == key)
        return false;

      root->_fields.values[root->_fields.n] = key;
      ++(root->_fields.n);

      rotate(root->_fields.values + ui,
             root->_fields.values + root->_fields.n - 1,
             root->_fields.values + root->_fields.n);
      return true;
    }
    // else { // n == 0
    root->_fields.values[0] = key;
    root->_fields.n = 1;
    return true;
  }

  typename node_type::field_type pi = 0;
  p = nullptr;
  u = root;
  goto run_internal;

  while (u != nullptr) {

    if (u->_fields.n < node_type::c_target_k) {
      // R2: There is space left
      return insert_in_leaf(p, pi, u, key);
    }

    // u->n == k
  run_internal:

    // R1: Swap laterals if needed
    ui = 0;
    if (key < u->_fields.values[0]) {
      swap(key, u->_fields.values[0]);
    } else if (key > u->_fields.values[node_type::c_target_k - 1]) {
      swap(key, u->_fields.values[node_type::c_target_k - 1]);
      ui = node_type::c_target_k - 2; // In a set, this is garanteed to be in
                                      // the last valid pointer index
    } else {
      // Calculate position of 'val' in 'u'
      for (ui = 0; key > u->_fields.values[ui]; ++ui)
        ;

      if (u->_fields.values[ui] == key)
        return false;
      --ui;
    }

    // Now we have a key that can be inside W(u).

    // Then:
    //  - if u is a leaf, we must try to insert horizontally
    //  - else it is an internal node, we must try to descend

    // Root will always be internal
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

    // R3: BalancedSlideSide
    if (trySlide(u, ui, p, pi, key) == true)
      return true;

    // Else: u must be internal and connect to a new node
    u = makeInternal(u);
    p->_fields.pointers[pi] = u;
    p = u;
    pi = ui;
    break;
  }

  // R5: Create the node
  // - It will never be the root node
  u = new_leaf_node();
  u->_fields.values[0] = key;
  u->_fields.n = 1;

  p->_fields.pointers[pi] = u;
  return true;
}
