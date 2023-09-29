#include "../../wtree/WTree.h"
#include <bits/stdc++.h>
#include <cstdlib>
#include <ctime>
#include <sstream>

#define UINT unsigned int
#define PB push_back

// Time cheking:
#define TS chrono::steady_clock::now();
#define TE chrono::steady_clock::now();
#define TD(e, s) chrono::duration_cast<chrono::microseconds>(e - s).count();

// ===== User Parameters

using namespace std;
using namespace WTreeLib;

int main() {
  bool ans;
  int val;
  WTree<int> WT(4);

  vector<int> values{3000, 4000, 5000, 6000, 4100, 4200,
                     4300, 4400, 4250, 4260, 4270, 4280};

  for (const int &v : values) {
    ans = WT.insert(v);
    if (!ans) {
      cout << "\n==========\n[ERROR] Insertion not working.\n";
      exit(EXIT_FAILURE);
    }
  }

  WT.printTree();

  for (const int &v : values) {
    ans = WT.find(v);
    if (!ans) {
      cout << "\n==========\n[ERROR] Search not working\n";
      exit(EXIT_FAILURE);
    }
  }
  cout << "[Correct] True positives are working\n";

  for (const int &v : {1, 2, 3, 10001, 10002, 10003, 4229, 4231, 5100}) {
    ans = WT.find(v);
    if (ans) {
      cout << "\n==========\n[ERROR] False positive on search.\n";
      exit(EXIT_FAILURE);
    }
  }
  cout << "[Correct] True negatives are working\n";

  vector<int> inserted;
  for (uint i = 0; i < 100; ++i) {
    do {
      val = rand();
    } while (!WT.insert(val));
    inserted.push_back(val);
  }

  for (const int &v : inserted) {
    ans = WT.find(v);
    if (!ans) {
      printf("\n==========\n[ERROR] Existent key %d not found.\n", v);
      exit(EXIT_FAILURE);
    }
  }

  cout << "[Correct] Search of random insertions are working\n";

  printf("\n\n====================\nSearch is working correctly.\n");
  return 0;
}