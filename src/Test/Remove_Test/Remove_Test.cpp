#include "../../wtree/WTree.h"
#include <bits/stdc++.h>

#define UINT unsigned int
#define PB push_back

// Time cheking:
#define TS chrono::steady_clock::now();
#define TE chrono::steady_clock::now();
#define TD(e, s) chrono::duration_cast<chrono::microseconds>(e - s).count();

// ===== User Parameters
#define PRINT 1

#define _HARDCODED_SEED 321
//#define _BIGN 100000
#define _BIGN 30

using namespace std;
using namespace WTreeLib;

void printVector(vector<int> &v, string title = "V:") {
  cout << title << "\n";
  for (int &val : v) {
    cout << val << " ";
  }
  cout << "\n";
}

int main() {
  bool ans;
  int val;
  unsigned short i;

  WTree<int> WT(8);
  WT.insert({100, 200, 300, 400, 500, 600, 700, 800});
  WT.printTree();

  // ####################
  cout << "1.1 First value\n";

  ans = WT.remove(100);
  if (!ans) {
    cout << "\n==========\n[ERROR] First value not removed.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  // ####################
  cout << "1.2 Last value\n";

  WT.insert(100);
  WT.printTree();
  ans = WT.remove(800);
  if (!ans) {
    cout << "\n==========\n[ERROR] Last value not removed.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();


  // ####################
  cout << "1.3 Remove nodes size 1\n";
  
  WT.insert({800, 10, 900});
  WT.printTree();

  ans = WT.remove(100);
  if (!ans) {
    cout << "\n==========\n[ERROR] Left node of size 1 was not removed.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();
  
  ans = WT.remove(800);
  if (!ans) {
    cout << "\n==========\n[ERROR] Left node of size 1 was not removed.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();


  // ####################
  cout << "2.1 Popmin with path size = 1\n";
  
  WT.insert({9, 20});
  WT.printTree();
  val = WT.popMin(WT.root);
  if (val != 9) {
    cout << "\n==========\n[ERROR] Popmin path size = 1 is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  val = WT.popMin(WT.root);
  if (val != 10) {
    cout << "\n==========\n[ERROR] Popmin path size = 1 with remove node is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  // ####################
  cout << "2.2 Popmin with wide path\n";
  WT.insert({605, 610});
  WT.printTree();
  val = WT.popMin(WT.root);
  if (val != 20) {
    cout << "\n==========\n[ERROR] Popmin wide path node is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  val = WT.popMin(WT.root);
  if (val != 200) {
    cout << "\n==========\n[ERROR] Popmin wide path with remove node is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  // ####################
  cout << "2.3 Popmin with path size = 2\n";
  WT.insert({705, 720, 730, 740, 750, 800, 820, 850});
  WT.insert({710, 715, 725, 735, 745, 755, 765, 770});
  WT.insert(748);
  WT.printTree();

  for(i = 0; i < 8; ++i)
    WT.popMin();
  WT.printTree();

  val = WT.popMin(WT.root);
  if (val != 710) {
    cout << "\n==========\n[ERROR] Popmin wide path size = 2 and node remove is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();


  // ####################
  cout << "3.1 Popmax with path size = 1\n";

  val = WT.popMax(WT.root);
  if (val != 900) {
    cout << "\n==========\n[ERROR] Popmax path size = 1 is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  for(i = 0; i < 7; ++i)
    WT.popMin();
  WT.printTree();

  WT.insert({700, 705});//, 710, 715, 720, 725, 730, 735});
  WT.printTree();


  // ####################
  cout << "3.2 Popmax with wide path\n";
  
  val = WT.popMax(WT.root);
  if (val != 850) {
    cout << "\n==========\n[ERROR] Popmax wide path and node remove is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  val = WT.popMax(WT.root);
  if (val != 820) {
    cout << "\n==========\n[ERROR] Popmax wide path and node remove is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  cout << "3.3 Popmax with path size = 2\n";

  WT.insert({710, 715, 720, 725, 730, 735, 740, 745});
  WT.insert({500, 510, 520, 530, 540, 550, 560, 570});
  WT.insert({503, 506});
  WT.printTree();

  for(i = 0; i < 8; ++i)
    WT.popMax();
  WT.printTree();

  val = WT.popMax(WT.root);
  if (val != 735) {
    cout << "\n==========\n[ERROR] Popmax path size = 2 is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  val = WT.popMax(WT.root);
  if (val != 730) {
    cout << "\n==========\n[ERROR] Popmax path size = 2 with node remove is incorrect.\n";
    exit(EXIT_FAILURE);
  }
  WT.printTree();

  // TODO: Test remove. Try on borders and removing nodes.

  cout << "====================\nRemove is working correctly.\n";
  return 0;
}