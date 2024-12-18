#include "WTree.h"

using namespace std;
using namespace WTreeLib;

/**
  Check if the key 'val' is in the tree.
*/
template <typename T> bool WTree<T>::find(T val) const {
  typename node_type::field_type ui;
  WTreeNode<T> *u = root;
  if (u->_fields.n == 0)
    return false;

  do {
    assert(u->_fields.n > 0);
    if (val < u->_fields.values[0] || val > u->_fields.values[u->_fields.n - 1])
      return false;

    for (ui = 0; val > u->_fields.values[ui]; ++ui)
      ;

    if (u->_fields.values[ui] == val)
      return true;

    if (u->_fields.isInternal)
      u = u->_fields.pointers[ui - 1];
    else
      return false;
  } while (u != nullptr);

  return false;
}

/**
  Check if the key 'val' is in the tree, saving the following:
  - ou: current node
  - op: ascendent node of the current node
  - pi: index of 'ou' inside 'op'
  - ix: index of 'val' inside 'ou'
*/
template <typename T>
bool WTree<T>::find(T val, WTreeNode<T> **op, WTreeNode<T> **ou,
                    typename node_type::field_type &pi,
                    typename node_type::field_type &ui) const {
  WTreeNode<T> *u = root;
  if (u->_fields.n == 0)
    return false;
  while (u != nullptr) {
    if (val < u->_fields.values[0] || val > u->_fields.values[u->_fields.n - 1])
      return false;

    for (ui = 0; val > u->_fields.values[ui]; ++ui)
      ;

    // From here, val must be in [min, max].
    // Therefore,   i must be at [0, n[

    if (u->_fields.values[ui] == val) {
      *ou = u;
      return true;
    }

    if (u->_fields.isInternal) {
      *op = u;
      pi = ui - 1; // Correction for the descendent
      u = u->_fields.pointers[pi];
    } else
      return false;
  }

  return false;
}