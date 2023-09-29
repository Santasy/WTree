// This was taken (and then adapted) from:
// https://newbedev.com/print-binary-tree-in-a-pretty-way-using-c

#include "WTree.h"
#include <cstddef>
#include <iostream>

using namespace std;
using namespace WTreeLib;

template <typename T> void WTree<T>::printTree() {
  printf("\n====================\nPrinting WTree [k:%u]\n", k);
  if (root == nullptr) {
    printf("WTree has no element(s).\n");
    return;
  }
  printNode(root, "", false);
  cout << "\n";
}

template <typename T>
void WTree<T>::printNode(WTreeNode<T> *u, const string &prefix, bool isLast) {
  cout << prefix << (isLast ? "└──" : "├──");

  if (u == nullptr) {
    cout << "[x]\n";
    return;
  }

  u->printKeys();
  if (u->_fields.isInternal && u->_fields.n > 1) {
    for (unsigned short i = 0; i < u->_fields.n - 2; ++i) {
      printNode(u->_fields.pointers[i], prefix + (isLast ? "    " : "│   "),
                false);
    }
    printNode(u->_fields.pointers[u->_fields.n - 2],
              prefix + (isLast ? "    " : "│   "), true);
  }
}