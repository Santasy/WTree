#include "WTree.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;
using namespace WTreeLib;

template <typename T> WTreeNode<T>::WTreeNode(const t_field k) {
  // k?
  _fields.n = 0;
  _fields.isInternal = false;
  _fields.capacity = init_capacity;
  _fields.keys = static_cast<T *>(malloc(sizeof(T) * init_capacity));
  _fields.pointers = nullptr;
}

template <typename T>
WTreeNode<T>::WTreeNode(const t_field k, const t_field capacity) {
  // k?
  assert(capacity <= k);

  _fields.n = 0;
  _fields.isInternal = false;
  _fields.capacity = capacity;
  _fields.keys = static_cast<T *>(malloc(sizeof(T) * capacity));
  _fields.pointers = nullptr;
}

template <typename T> WTreeNode<T>::~WTreeNode() {
  free(_fields.keys);
  if (_fields.isInternal) {
    for (t_field i = 0; i < _fields.n - 1; ++i) {
      if (_fields.pointers[i])
        delete _fields.pointers[i];
    }
    delete[] _fields.pointers;
  }
}

/**
  Reserves at least the memory for the specified capacity number of keys.
*/
template <typename T> void WTreeNode<T>::reserve(const t_field capacity) {
  if (capacity <= _fields.capacity)
    return;

  _fields.capacity = capacity;
  _fields.keys = static_cast<T *>(realloc(_fields.keys, sizeof(T) * capacity));
}

/**
  Set the node as internal, allocating the space
  needed for the pointer's vector.
*/
template <typename T> bool WTreeNode<T>::setAsInternal(const t_field k) {
  if (_fields.isInternal) [[unlikely]]
    return true;

  _fields.isInternal = true;
  const uint bytes_ptrs = sizeof(t_self *) * (k - 1);
  _fields.pointers = static_cast<t_self **>(operator new(bytes_ptrs));
  void *res = memset(_fields.pointers, 0, bytes_ptrs);
  assert(res);

  return true;
}

/**
  Set the node as leaf, deallocating the space
  needed for the pointer's vector.
*/
template <typename T> bool WTreeNode<T>::setAsLeaf() {
  if (_fields.isInternal == false) [[unlikely]]
    return true;

  _fields.isInternal = false;
  delete[] _fields.pointers;

  return true;
}

/**
  Get the index of a possible node pointer to continue searching 'key'.
  The current version uses 'lower_bound' from <algorithm>.
*/
template <typename T>
typename WTreeNode<T>::t_field WTreeNode<T>::getPosition(T key) {
  assert(_fields.n > 0);

  if (key > _fields.keys[_fields.n - 1])
    return -1;

  t_field pos;
  for (pos = 0; key > _fields.keys[pos]; ++pos)
    ;
  return pos;
}

/**
  Check if 'key' is inside the node.
*/
template <typename T> bool WTreeNode<T>::contains(T key) {
  t_field lwb = getPosition(key);
  return (lwb < _fields.n && _fields.keys[lwb] == key);
}

/**
  Calculate the node height traversing it's nodes.
  Returns 0 if the node has no descendents.
*/
template <typename T> int WTreeNode<T>::getHeight() const {
  if (_fields.isInternal == false)
    return 0;

  int h = -1;
  for (t_field lwb = 0; lwb < _fields.n - 1; ++lwb) {
    if (_fields.pointers[lwb] == nullptr)
      continue;
    h = max(h, _fields.pointers[lwb]->getHeight());
  }
  return 1 + h;
}

/**
  Insert 'key' inside the node.
  The current version uses 'rotate' from <algorithm>.
*/
template <typename T> bool WTreeNode<T>::insert(T key, const t_field k) {
  assert(_fields.n != k);

  t_field upb = 0;
  if (key > _fields.keys[_fields.n - 1])
    upb = _fields.n;
  else
    for (; key > _fields.keys[upb]; ++upb)
      ;

  if (upb < _fields.n && _fields.keys[upb] == key) [[unlikely]]
    return false;

  if (_fields.n == _fields.capacity) {
    _fields.capacity =
        min((t_field)(_fields.capacity + (_fields.capacity >> 1)), k);
    _fields.keys =
        static_cast<T *>(realloc(_fields.keys, sizeof(T) * _fields.capacity));
  }

  _fields.keys[_fields.n] = key;
  ++_fields.n;
  rotate(_fields.keys + upb, _fields.keys + _fields.n - 1,
         _fields.keys + _fields.n);

  return true;
}

/**
  Apply the insert function to all the keys in 'keys'.
*/
template <typename T>
vector<bool> WTreeNode<T>::insert(const std::vector<T> &keys, const t_field k) {
  vector<bool> results;
  for (int v : keys)
    results.push_back(insert(v, k));
  return results;
}

/**
  Apply the insert function to all the keys in 'keys'.
*/
template <typename T>
vector<bool> WTreeNode<T>::insert(const T *&keys, unsigned int n,
                                  const t_field k) {
  vector<bool> results;
  for (unsigned int i = 0; i < n; ++i)
    results.push_back(insert(keys[i], k));
  return results;
}

/**
  Erase the last key inside the node.
  Returns false if the node was empty.
*/
template <typename T> bool WTreeNode<T>::popBack() {

  if (_fields.n == 0) [[unlikely]]
    return false;
  --_fields.n;
  return true;
}

/**
  Calculate the total byte size for the node.
*/
template <typename T> unsigned long WTreeNode<T>::getTotalBytes() const {
  unsigned long total = sizeof(*this) + sizeof(T) * _fields.capacity;

  if (_fields.isInternal) {
    total += sizeof(t_self) * _fields.n - 1;
    for (t_field i = 0; i < _fields.n; ++i) {
      if (_fields.pointers[i])
        total += _fields.pointers[i]->getTotalBytes();
    }
  }
  return total;
}

template <typename T> void WTreeNode<T>::printKeys() const {
  printf("[%u - %c] ", _fields.n, _fields.isInternal ? 'I' : 'L');
  for (t_field i = 0; i < _fields.n; ++i)
    cout << _fields.keys[i] << " ";
  cout << "\n";
}

template <typename T> void WTreeNode<T>::print() const {
  printKeys();
  if (_fields.isInternal) {
    cout << "pointers: [ ";
    for (t_field i = 0; i < _fields.n - 1; ++i)
      printf("%u:%c ", i, _fields.pointers[i] ? 'C' : 'x');
    cout << "]\n";
  }
}