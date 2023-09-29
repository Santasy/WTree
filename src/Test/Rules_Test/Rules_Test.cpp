#include "../../wtree/WTree.h"
#include <bits/stdc++.h>
#include <cstdlib>
#include <ctime>
#include <sstream>

#include "RulesTestNamespace.hpp"

using namespace std;

int main(int argc, char **argv) {
  printf("Can receibe two positional args: %s <print_tree> <pause_between>",
         argv[0]);

  bool pause_between = false, print_tree = false;
  if (argc > 1)
    pause_between = atoi(argv[1]);
  if (argc > 2)
    print_tree = atoi(argv[2]);

  if (RulesTestNamespace::Test_AllRules(pause_between, print_tree) == false) {
    cout << "[ERROR] All rules not working.\n";
    return EXIT_FAILURE;
  }
  printf(
      "\n\n====================\nInsertion rules are working correctly.\n\n\n");

  if (RulesTestNamespace::Test_InstantiationType() == false) {
    cout << "[ERROR] Instantiation test not working.\n";
    return EXIT_FAILURE;
  }

  return 0;
}