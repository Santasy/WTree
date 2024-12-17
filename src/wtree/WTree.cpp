#include "WTree.h"
#include <iostream>

using namespace std;
using namespace WTreeLib;

template <typename T> WTree<T>::WTree() {
  root = nullptr;

#ifdef DBGVERBOSE
  cout << "c_target_k: " << node_type::c_target_k << "\n";
  cout << "c_last_growth: " << node_type::c_last_growth << "\n";

  cout << "== Node Field Sizes (bytes):\n";
  cout << "field_type: " << sizeof(typename node_type::field_type) << "\n";
  cout << "base_fields: " << sizeof(typename node_type::base_fields) << "\n";
  cout << "leaf_fields: " << sizeof(typename node_type::leaf_fields) << "\n";
  cout << "internal_fields: " << sizeof(typename node_type::internal_fields)
       << "\n";
#endif

  root = new_internal_node();
}

template <typename T> WTree<T>::~WTree() {
  if (root)
    _recursive_delete(root);
}

template <typename T> void WTree<T>::_recursive_delete(node_type *u) {
  if (u->_fields.isInternal) {
    for (typename node_type::field_type i = 0; i < node_type::c_target_k - 1;
         ++i) {
      if (u->_fields.pointers[i] != nullptr)
        _recursive_delete(u->_fields.pointers[i]);
    }
    delete_internal_node(u);
  } else {
    delete_leaf_node(u);
  }
}

template <typename T>

WTreeNode<T> *
WTree<T>::init_leaf(typename node_type::leaf_fields *u,
                    typename node_type::field_type init_capacity) {
  node_type *node = reinterpret_cast<node_type *>(u);
  node->_fields.n = 0;
  node->_fields.capacity = init_capacity;
  node->_fields.isInternal = false;
  // void *res = memset(&node->_fields.values, 0, init_capacity * sizeof(T));
  // assert(res != nullptr);
  return node;
}

template <typename T>
WTreeNode<T> *WTree<T>::init_internal(typename node_type::internal_fields *u) {
  WTreeNode<T> *node = reinterpret_cast<WTreeNode<T> *>(u);
  node->_fields.n = 0;
  node->_fields.capacity = node_type::c_target_k;
  node->_fields.isInternal = true;
  // void *res =
  //     memset(&node->_fields.values, 0, node_type::c_target_k * sizeof(T));
  // assert(res != nullptr);

  void *res = memset(node->_fields.pointers, 0, sizeof(node->_fields.pointers));
  assert(res != nullptr);
  return node;
}

template <typename T>
WTreeNode<T> *WTree<T>::new_leaf_node(typename node_type::field_type capacity) {
  using LFT = typename WTreeNode<T>::leaf_fields;
  internal_allocator_type ia;
  LFT *u;
  const ushort nbytes = node_type::basefields_size + sizeof(T) * capacity;
  u = reinterpret_cast<LFT *>(internal_allocator_traits::allocate(ia, nbytes));
  return init_leaf(u, capacity);
}

template <typename T> WTreeNode<T> *WTree<T>::new_internal_node() {
  internal_allocator_type ia;
  typename WTreeNode<T>::internal_fields *u =
      reinterpret_cast<typename WTreeNode<T>::internal_fields *>(
          internal_allocator_traits::allocate(
              ia, sizeof(typename WTreeNode<T>::internal_fields)));
  return init_internal(u);
}