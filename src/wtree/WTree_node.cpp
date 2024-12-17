#include "WTree.h"
#include <iostream>

#define EB emplace_back
using namespace std;
using namespace WTreeLib;

/** Please read WTree.cpp for node creation functions.
 */

/**
  Check if 'val' is inside the node.
*/
template <typename T> bool WTreeNode<T>::contains(T val) {
  typename self_type::field_type i = getPosition(val);
  return (i != _fields.n && _fields.values[i] == val);
}

/**
  Calculate the node height traversing it's nodes.

  Returns 0 if the node has no descendents.
*/
template <typename T> int WTreeNode<T>::getHeight() const {
  if (!_fields.isInternal)
    return 0;

  int h = -1;
  for (typename self_type::field_type it = 0; it < _fields.n - 1; ++it) {
    if (!(_fields.pointers[it]))
      continue;
    h = max(h, _fields.pointers[it]->getHeight());
  }
  return 1 + h;
}

template <typename T> void WTreeNode<T>::print() const {
  cout << "[" << (size_t)_fields.n << "] ";
  for (typename self_type::field_type i = 0; i < _fields.n; ++i)
    cout << _fields.values[i] << " ";
  cout << "\n";
}

/**
  Insert the key 'val' inside the node.
  The current version uses 'rotate' from <algorithm>.
*/
template <typename T> bool WTreeNode<T>::insert(T val) {
  // Linear search from the right
  typename self_type::field_type pos;
  assert(_fields.n != c_target_k);

  for (pos = _fields.n; pos > 0 && _fields.values[pos - 1] > val; --pos)
    ;

  if (pos > 0 && _fields.values[pos - 1] == val)
    return false;

  /* // Version move_backwards: ** This is slower than rotate **
  if (n == values.size())
    values.emplace_back(0); // arbitrarial value
  ++n;

  move_backward(values.begin() + pos, values.begin() - 1 + n, values.begin() +
  n); values[pos] = val;
  */

  /**/ // Version rotate: ** This is faster than move_backwards **
  _fields.values[_fields.n] = val;
  ++_fields.n;

  if (pos != _fields.n - 1) {
    T *pinit = _fields.values;
    rotate(pinit + pos, pinit + _fields.n - 1, pinit + _fields.n);
  }

  /**/

  return true;
}

/**
  Apply the insert function to all the keys in 'values'.
*/
template <typename T>
vector<bool> WTreeNode<T>::insert(const std::vector<T> &values) {
  vector<bool> results;
  for (int v : values)
    results.push_back(insert(v));
  return results;
}

/**
  Apply the insert function to all the keys in 'values'.
*/
template <typename T>
vector<bool> WTreeNode<T>::insert(const T *&values, size_t n) {
  vector<bool> results;
  for (size_t i = 0; i < n; ++i)
    results.push_back(insert(values[i]));
  return results;
}

/**
  Get the index of a possible node pointer to continue searching 'val'.
  The current version uses 'lower_bound' from <algorithm>.
*/
template <typename T>
typename WTreeNode<T>::field_type WTreeNode<T>::getPosition(T val) {
  if (_fields.n == 0)
    return 0;

  typename self_type::field_type pos;
  for (pos = 0; pos < _fields.n && _fields.values[pos] < val; ++pos)
    ;
  return pos;
}