#include "WTree.h"

using namespace std;
using namespace WTreeLib;

/**
  Check if the key 'val' is in the tree.
*/
template <typename T> bool WTree<T>::find(T val) const {
  WTreeNode<T> *u = root;
  if (u->_fields.n == 0)
    return false;

  do {
    assert(u->_fields.n > 0);
    if (val < u->_fields.values[0] || val > u->_fields.values[u->_fields.n - 1])
      return false;

    T *anchor = u->_fields.values;
    typename node_type::field_type len = u->_fields.n, half;
    while (len > 1) {
      half = len >> 1;
      anchor += (anchor[half - 1] < val) * half;
      len -= half;
    }
    // now anchor is the lower-bound or exactly the key
    if (*anchor == val)
      return true;

    if (u->_fields.isInternal) {
      assert((anchor - u->_fields.values) > 0);
      u = u->_fields.pointers[anchor - u->_fields.values - 1];
    } else
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
    assert(u->_fields.n > 0);
    if (val < u->_fields.values[0] || val > u->_fields.values[u->_fields.n - 1])
      return false;

    T *anchor = u->_fields.values;
    typename node_type::field_type len = u->_fields.n, half;
    while (len > 1) {
      half = len >> 1;
      anchor += (anchor[half - 1] < val) * half;
      len -= half;
    }
    ui = anchor - u->_fields.values;

    // now anchor is the lower-bound or exactly the key
    if (*anchor == val) {
      assert(u->_fields.values[ui] == val);
      *ou = u;
      return true;
    }

    if (u->_fields.isInternal) {
      assert((anchor - u->_fields.values) > 0);
      pi = ui - 1; // Correction for the descendent
      *op = u;
      u = u->_fields.pointers[pi];
    } else
      return false;
  }

  return false;
}