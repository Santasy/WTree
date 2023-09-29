#include "RulesTestNamespace.hpp"

using namespace RulesTestNamespace;

template <typename T>
bool RulesTestNamespace::Test_MassiveInsertDelete(ulong k) {
  typedef WTreeLib::WTree<T> WT_type;
  WT_type wt(k);
  const ulong maxval = min(500'000lu, (ulong)numeric_limits<T>::max());
  printf("\t\t%lu keys.\n", maxval);

  ulong i;
  T val;
  bool ans = true;

  vector<T> values(maxval >> 1);
  UniformGenerator<T> gen(0, maxval);

  for (i = 0; i < (maxval >> 1); ++i) {
    val = gen.getInexistentKey();
    values[i] = val;
    ans &= wt.insert(val);
    assert(ans);
  }

  if (wt.verify() == false)
    return false;

  for (i = 0; i < (maxval >> 1); ++i) {
    ans &= wt.remove(values[i]);
    assert(ans);
    assert(wt.verify());
  }

  return ans;
}

bool RulesTestNamespace::Test_InstantiationType() {
  cout << "  ===== Instantiations:\n";
  ulong kval = 64;

  cout << " 2 bytes: ushort ...\n";
  if (Test_MassiveInsertDelete<ushort>(kval) == false)
    return false;

  cout << " 4 bytes: uint ...\n";
  if (Test_MassiveInsertDelete<uint>(kval) == false)
    return false;

  cout << " 8 bytes: ulong ...\n";
  if (Test_MassiveInsertDelete<ulong>(kval) == false)
    return false;

  return true;
}

bool RulesTestNamespace::Test_AllRules(bool pause_between, bool print_tree) {
  const ushort kval = 4;
  WTree<int> WT(kval);
  bool ans;

  WTreeNode<int> n(kval);
  n.insert({10, 20, 30}, kval);

  ans = WT.insert(1000);
  if (!ans) {
    cout << "\n==========\n[ERROR] R5 not working with root.\n";
    return false;
  }
  cout << "[Correct] R5.1: Created root node\n";
  if (pause_between) {
    WT.printTree();
    cin.ignore();
  } else if (print_tree) {
    WT.printTree();
  }

  ans = WT.insert(2000);
  if (!ans) {
    cout << "\n==========\n[ERROR] R2 not working.\n";
    return false;
  }
  cout << "[Correct] R2: Inserted on available space\n";
  if (pause_between) {
    WT.printTree();
    cin.ignore();
  } else if (print_tree) {
    WT.printTree();
  }

  for (int v : {3000, 4000, 1500}) {
    ans = WT.insert(v);
    if (!ans) {
      cout << "\n==========\n[ERROR] R5 not working with empty node.\n";
      return false;
    }
  }
  cout << "[Correct] R5.2: Created empty node\n";
  if (pause_between) {
    WT.printTree();
    cin.ignore();
  } else if (print_tree) {
    WT.printTree();
  }

  ans = WT.insert(750);
  if (!ans) {
    cout << "\n==========\n[ERROR] R1 not working swaping with left.\n";
    return false;
  }
  assert(WT.verify());
  cout << "[Correct] R1.1: Swap left\n";

  if (pause_between) {
    WT.printTree();
    cin.ignore();
  } else if (print_tree) {
    WT.printTree();
  }

  ans = WT.insert(4500);
  if (!ans) {
    cout << "\n==========\n[ERROR] R1 not working swaping with right.\n";
    return false;
  }
  assert(WT.verify());
  cout << "[Correct] R1.1: Swap right\n";

  if (pause_between) {
    WT.printTree();
    cin.ignore();
  } else if (print_tree) {
    WT.printTree();
  }

  for (int v : {800, 850, 500}) {
    ans = WT.insert(v);
    if (!ans) {
      cout << "\n==========\n[ERROR] R3 not working splitting to right.\n";
      return false;
    }
    assert(WT.verify());
  }
  cout << "[Correct] R3.2: Splited to right\n";
  if (pause_between) {
    WT.printTree();
    cin.ignore();
  } else if (print_tree) {
    WT.printTree();
  }

  for (int v : {875, 890}) {
    ans = WT.insert(v);
    if (!ans) {
      cout << "\n==========\n[ERROR] R4 not working sliding to right.\n";
      return false;
    }
    assert(WT.verify());
  }
  cout << "[Correct] R4.2: Slided to right\n";
  if (pause_between) {
    WT.printTree();
    cin.ignore();
  } else if (print_tree) {
    WT.printTree();
  }

  for (int v : {4100, 4200, 4300, 3900}) {
    ans = WT.insert(v);
    if (!ans) {
      cout << "\n==========\n[ERROR] R4 not working sliding to left.\n";
      return false;
    }
    assert(WT.verify());
  }
  cout << "[Correct] R4.1: Slided to left\n";
  if (pause_between) {
    WT.printTree();
    cin.ignore();
  } else if (print_tree) {
    WT.printTree();
  }

  for (int v : {5000, 5100, 5200, 5300, 10000}) {
    ans = WT.insert(v);
    if (ans == false) {
      cout << "\n==========\n[ERROR] R3.1 not working splitting to left.\n";
      return false;
    }
    assert(WT.verify());
  }
  cout << "[Correct] R3.1: Splited to left\n";

  WT.printTree();
  ans = WT.verify();
  assert(ans);

  // == Inorder check
  vector<int> trueAns{500,  890,  3900, 10000, 750,  800,  850,
                      875,  1000, 1500, 2000,  3000, 4000, 4100,
                      4500, 5300, 4200, 4300,  5000, 5100, 5200};
  sort(trueAns.begin(), trueAns.end());
  vector<int> result = WT.getInorder();

  size_t ts = trueAns.size(), rs = result.size();
  if (ts != rs) {
    printf("[ERROR] Result has %lu/%lu keys.\n", rs, ts);
  }

  for (uint i = 0; i < trueAns.size(); ++i) {
    if (trueAns[i] != result[i]) {
      printf("[ERROR] Comparing answerd on index %u\n", i);
      return false;
    }
  }

  for (int v : {333, 444, 555, 666, 777, 888, 999, 1010, 2020}) {
    ans = WT.insert(v);
    trueAns.push_back(v);
    if (!ans) {
      printf("\n==========\n[ERROR] %d was not inserted.\n", v);
      return false;
    }
    assert(WT.verify());
  }

  WT.printTree();
  ans = WT.verify();
  assert(ans);

  // == Inorder check
  sort(trueAns.begin(), trueAns.end());
  result = WT.getInorder();
  ts = trueAns.size(), rs = result.size();
  if (ts != rs) {
    printf("[ERROR] Result has %lu/%lu keys.\n", rs, ts);
  }

  for (uint i = 0; i < trueAns.size(); ++i) {
    if (trueAns[i] != result[i]) {
      printf("[ERROR] Comparing answerd on index %u\n", i);
      return false;
    }
  }

  for (int v : {321, 432, 543, 654, 765, 876, 987, 1321, 1432, 1543, 99999,
                999991, 999995}) {
    ans = WT.insert(v);
    trueAns.push_back(v);
    if (!ans) {
      printf("\n==========\n[ERROR] %d was not inserted.\n", v);
      return false;
    }
  }

  WT.printTree();

  // == Inorder check
  sort(trueAns.begin(), trueAns.end());
  result = WT.getInorder();
  ts = trueAns.size(), rs = result.size();
  if (ts != rs) {
    printf("[ERROR] Result has %lu/%lu keys.\n", rs, ts);
  }

  for (uint i = 0; i < trueAns.size(); ++i) {
    if (trueAns[i] != result[i]) {
      printf("[ERROR] Comparing answerd on index %u\n", i);
      return false;
    }
  }
  return true;
}