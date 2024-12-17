#ifndef RULESTESTNAMESPACE_H
#define RULESTESTNAMESPACE_H

#include <cassert>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <limits>
#include <sys/types.h>

#include "../../utils/KeyGenerators.h"
#include "../../wtree/WTree.h"

namespace RulesTestNamespace {

template <typename T> bool Test_MassiveInsertDelete() {
  typedef WTreeLib::WTree<T> WT_type;
  WT_type wt;
  const ulong maxval = min(1'000'000lu, (ulong)numeric_limits<T>::max());
  printf("\t\t%lu keys.\n", maxval >> 1);

  ulong i;
  T val;
  bool ans = true;

  UniformGenerator<T> gen(0, maxval);
  vector<T> values(maxval >> 1);

  for (i = 0; i < maxval >> 1; ++i) {
    val = gen.getInexistentKey();
    values[i] = val;
    ans &= wt.insert(val);
    assert(ans);
  }

  for (i = 0; i < maxval >> 1; ++i) {
    ans &= wt.remove(values[i]);
    assert(ans);
  }

  return ans == true;
}

bool Test_InstantiationType() {
  cout << "  ===== Instantiations:\n";

  cout << " 2 bytes: ushort ...\n";
  Test_MassiveInsertDelete<ushort>();

  cout << " 4 bytes: uint ...\n";
  Test_MassiveInsertDelete<uint>();

  cout << " 8 bytes: ulong ...\n";
  Test_MassiveInsertDelete<ulong>();

  // TODO: Test some type bigger than 8 bytes.

  return true;
}

bool Test_AllRules(bool pause_between, bool print_tree) {
  typedef WTreeLib::WTree<int> TWTree;
  typedef WTreeLib::WTreeNode<int> TWTNode;

  ushort k = TWTNode::c_target_k;
  bool ans;

  TWTree wt;
  TWTNode *node = wt.root;

  // 1. Fill root node
  wt.fillNode(&node, k, 0, numeric_limits<int>::max());

  // 2. Replace laterals
  int new_max = node->_fields.values[k - 1] + 1;
  ans = wt.insert(new_max);
  if (!ans) {
    cout << "\n==========\n[ERROR] Replace right lateral not working.\n";
    return false;
  }

  int new_min = node->_fields.values[0] - 1;
  ans = wt.insert(new_min);
  if (!ans) {
    cout << "\n==========\n[ERROR] Replace left lateral not working.\n";
    return false;
  }

  cout << "[Correct] Replace laterals\n";
  if (pause_between) {
    wt.printTree();
    cin.ignore();
  } else if (print_tree) {
    wt.printTree();
  }

  if (!WTreeRulesAssumptions::slide_over_split ||
      !WTreeRulesAssumptions::slide_to_right_first ||
      !WTreeRulesAssumptions::split_to_right_first) {
    cout << "\n==========\n[WARNING] Assumptions for this test case are not "
            "valid. Process will end as correct.\n";
    return false;
  }

  // 3. Slide to adjacent of size 1
  int mid = k / 2;
  node->_fields.pointers[mid] = wt.new_leaf_node(k);
  wt.fillNode(&node->_fields.pointers[mid], k, node->_fields.values[mid] + 1,
              node->_fields.values[mid + 1] - 1);

  // 3.1 to right
  wt.insert(node->_fields.values[mid + 1] + 1);
  ans = wt.insert(node->_fields.pointers[mid]->_fields.values[mid] + 1);
  if (!ans) {
    cout << "\n==========\n[ERROR] Slide to right not working.\n";
    return false;
  }

  cout << "[Correct] Slide to right.\n";
  if (pause_between) {
    wt.printTree();
    cin.ignore();
  } else if (print_tree) {
    wt.printTree();
  }

  // 3.2 to left

  wt.fillNode(&node->_fields.pointers[mid], k, // fill mid
              node->_fields.values[mid] + 1, node->_fields.values[mid + 1] - 1);
  wt.fillNode(&node->_fields.pointers[mid + 1], k, // fill right
              node->_fields.values[mid + 1] + 1,
              node->_fields.values[mid + 2] - 1);
  wt.insert(node->_fields.values[mid - 1] + 1); // create left
  ans = wt.insert(node->_fields.pointers[mid]->_fields.values[mid] + 1);
  if (!ans || !wt.verify()) {
    cout << "\n==========\n[ERROR] Slide to left not working.\n";
    return false;
  }

  cout << "[Correct] Slide to left.\n";
  if (pause_between) {
    wt.printTree();
    cin.ignore();
  } else if (print_tree) {
    wt.printTree();
  }

  // 4. Split
  wt.eraseNodeChildren(node);
  node->_fields.pointers[mid] = wt.new_leaf_node(k);

  // 4.1 to right
  wt.fillNode(&node->_fields.pointers[mid], k, node->_fields.values[mid] + 1,
              node->_fields.values[mid + 1] - 1);
  ans = wt.insert(node->_fields.pointers[mid]->_fields.values[mid] + 1);
  if (!ans || !wt.verify()) {
    cout << "\n==========\n[ERROR] Split to right not working.\n";
    return false;
  }

  cout << "[Correct] Split to right.\n";
  if (pause_between) {
    wt.printTree();
    cin.ignore();
  } else if (print_tree) {
    wt.printTree();
  }

  // 4.2 to left
  wt.fillNode(&node->_fields.pointers[mid], k, node->_fields.values[mid] + 1,
              node->_fields.values[mid + 1] - 1);
  wt.fillNode(&node->_fields.pointers[mid + 1], k,
              node->_fields.values[mid + 1] + 1,
              node->_fields.values[mid + 2] - 1);
  ans = wt.insert(node->_fields.pointers[mid]->_fields.values[mid] + 1);
  if (!ans || !wt.verify()) {
    cout << "\n==========\n[ERROR] Split to left not working.\n";
    return false;
  }

  cout << "[Correct] Split to left.\n";
  if (pause_between) {
    wt.printTree();
    cin.ignore();
  } else if (print_tree) {
    wt.printTree();
  }

  return true;
}
} // namespace RulesTestNamespace

#endif