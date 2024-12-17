// This was taken (and then adapted) from:
// https://newbedev.com/print-binary-tree-in-a-pretty-way-using-c

#include <iostream>

#include "WTree.h"

using namespace std;
using namespace WTreeLib;

template <typename T> void WTree<T>::printTree() {
  printf("\n====================\nPrinting WTree [k:%u]\n",
         node_type::c_target_k);
  if (root == nullptr) {
    printf("WTree has no element(s).\n");
    return;
  }
  printNode(root, "", false);
  cout << "\n";
}

template <typename T>
void WTree<T>::printNode(WTreeNode<T> *u, const string &prefix, bool isLeft) {
  cout << prefix;
  cout << (isLeft ? "├──" : "└──");
  if (u == nullptr) {
    cout << "[x]\n";
    return;
  } else {
    u->print();
    if (u->_fields.isInternal && u->_fields.n > 1) {
      for (typename node_type::field_type i = 0; i < u->_fields.n - 2; ++i) {
        printNode(u->_fields.pointers[i], prefix + (isLeft ? "│   " : "    "),
                  true);
      }
      printNode(u->_fields.pointers[u->_fields.n - 2],
                prefix + (isLeft ? "│   " : "    "), false);
    }
  }
}