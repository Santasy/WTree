#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <type_traits>

#include "../include/wtree/optional/print.hpp"
#include "../include/wtree/set.hpp"

#include "../utils/key_generators.hpp"

#define MAX_SIZE 10'000'000

using namespace std;
using namespace WTreeLib;

using KeyType = int;
using WMap = WTreeLib::set<KeyType, 126>; // Small nodes
using WTreeType = WMap::wtree_type;
using NodeType = WTreeType::node_type;
using Field = WTreeType::field_type;

using Printer = WTreeLib::WTreePrinter<WTreeType>;

char ask_operation() {
  string input;
  do {
    cout << "Choose:\n"
         << "\t\t- operation ([i]nsert, [s]earch, [e]rase)\n"
         << "\tothers:\n"
         << "\t\t([p]rint, [c]lose)\n"
         << "> ";
    cin >> input;
    switch (input[0]) {
    case 'i':
    case 's':
    case 'e':
    case 'p':
    case 'c':
      return input[0];
    }
    cout << "Must enter just one of the character first.\n";
  } while (input.size() != 1);
  return '\0';
}

int ask_value(string operation = "insert") {
  cout << "\n> Value to " << operation << ": ";
  int val;
  cin >> val;
  return val;
}

bool handle_command(WMap &storage, char command, int key) {
  switch (command) {
  case 'i':
    storage.insert(key);
    break;
  case 's':
    storage.find(key);
    break;
  case 'e':
    storage.erase(key);
    break;
  default:
    cout << "Command [" << command << "] not recognized.\n";
    return false;
  }
  return true;
}

int main(int argc, char **argv) {

  int key, i;

  if (argc < 2 || atoi(argv[1]) < 0 || atoi(argv[1]) > MAX_SIZE) {
    printf("Run: %s starting_size(<= %d)\n", argv[0], MAX_SIZE);
    exit(EXIT_FAILURE);
  }

  int inserts = atoi(argv[1]);
  UniformGenerator<KeyType> gen(112233, 10 * inserts);

  WMap storage;
  WTreeType *wt = storage.tree();
  Printer printer = Printer();
  printer.options.color_output = true;

  cout << "Creating from empty tree:\n";
  for (i = 0; i < inserts; ++i) {
    key = gen.get_absent_key();
    auto ans = storage.insert(key);
    if (!ans.second) {
      printf("[ERROR] %d was not inserted.\n", key);
      exit(EXIT_FAILURE);
    }
  }
  printer.print_tree(wt->root());

  cout << "Now the tree accepts operations:\n";
  char command;
  while (true) {
    command = ask_operation();
    if (command == 'c')
      break;

    if (command == 'p') {
      printer.print_tree(wt->root());
      continue;
    }
    key = ask_value();
    handle_command(storage, command, key);
  }

  printf("\n\n====================\n"
         "\tEnd\n");
  return 0;
}