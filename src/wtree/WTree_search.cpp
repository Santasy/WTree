#include "WTree.h"

using namespace std;
using namespace WTreeLib;

/**
  Check if 'key' is in the tree.
*/
template <typename T> bool WTree<T>::find(T key) const {
  WTreeNode<T> *u = root;
  t_field i;
  while (u != nullptr) {
    assert(u->_fields.n > 0);
    if (key < u->_fields.keys[0] || key > u->_fields.keys[u->_fields.n - 1])
      return false;

    for (i = 0; u->_fields.keys[i] < key; ++i)
      ;

    // From here, key must be in [min, max]
    // Therefore,   i must be at [0, n[

    if (u->_fields.keys[i] == key)
      return true;

    // Note that i must be corrected.
    u = (u->_fields.isInternal) ? u->_fields.pointers[--i] : nullptr;
  }

  return false;
}

/**
  Check if the key 'key' is in the tree, saving the following:
  - u: current node
  - p: ascendent node of 'u'
  - p_lwb: index of 'u' inside 'p'
  - lwb: lower_bound of 'key' within 'u'
*/
template <typename T>
bool WTree<T>::find(T key, WTreeNode<T> *&p, WTreeNode<T> *&u, t_field &p_lwb,
                    t_field &lwb) const {
  u = root;
  while (u != nullptr) {
    if (key < u->_fields.keys[0] || key > u->_fields.keys[u->_fields.n - 1])
      return false;

    for (lwb = 0; u->_fields.keys[lwb] < key; ++lwb)
      ;

    // From here, key must be in [min, max].
    // Therefore,   i must be at [0, n[

    if (u->_fields.keys[lwb] == key)
      return true;

    --lwb; // Correction for the descendent
    p = u;
    p_lwb = lwb;
    u = (u->_fields.isInternal) ? u->_fields.pointers[lwb] : nullptr;
  }

  return false;
}