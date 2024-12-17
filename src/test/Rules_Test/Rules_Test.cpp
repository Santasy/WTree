#include <ctime>

#include "RulesTestNamespace.hpp"

int main(int argc, char **argv) {
  printf("Run: %s <print_tree> <pause_between>", argv[0]);

  bool pause_between = false, print_tree = false;
  if (argc > 1)
    pause_between = atoi(argv[1]);
  if (argc > 2)
    print_tree = atoi(argv[2]);

  RulesTestNamespace::Test_AllRules(pause_between, print_tree);
  printf(
      "\n\n====================\nInsertion rules are working correctly.\n\n\n");

  RulesTestNamespace::Test_InstantiationType();

  return 0;
}