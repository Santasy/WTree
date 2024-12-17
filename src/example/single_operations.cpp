#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "../utils/KeyGenerators.h"
#include "../wtree/WTree.h"

#define MAX_SIZE 10'000'000

using namespace std;
using namespace WTreeLib;

int main(int argc, char **argv) {
  int val, i;
  bool ans;

  //
  if (argc != 2 || atoi(argv[1]) < 0 || atoi(argv[1]) > MAX_SIZE) {
    printf("Run: ./%s starting_size(<= %d)\n", argv[0], MAX_SIZE);
    exit(EXIT_FAILURE);
  }

  int inserts = atoi(argv[1]);
  UniformGenerator<int> gen(112233, 10 * inserts);

  WTree<int> WT;
  cout << "Inserting:\n";
  for (i = 0; i < inserts; ++i) {
    val = gen.getInexistentKey();
    ans = WT.insert(val);
    if (!ans) {
      printf("[ERROR] %d was not inserted.\n", val);
      exit(EXIT_FAILURE);
    }
  }

  WT.printTree();
  while (1) {
    cout << "Value to insert (break with 0): ";
    cin >> val;
    if (val == 0)
      break;

    if (WT.insert(val)) {
      cout << "Value has been inserted.\n";
      gen.annotateKey(val);
      WT.printTree();
    } else
      cout << "Repeated value has not been inserted.\n";
  }

  while (1) {
    cout << "Value to find (break with 0): ";
    cin >> val;
    if (val == 0)
      break;
    if (WT.find(val)) {
      cout << "Value has been founded.\n\nPress any key ...";
      cin.ignore();
      WT.printTree();
    } else
      cout << "Value has not been found.\n\nPress any key ...";
    cin.ignore();
  }

  printf("\n\n====================\nFin del programa.\n");
  return 0;
}