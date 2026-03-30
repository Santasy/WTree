#include <cstdlib>

#include "insert_rules_namespace.hpp"

const std::string title = "Insert Rules Test";

using namespace std;
using namespace TestPrinting;
using namespace InsertRulesNamespace;

int main() {
  WTreeLib::WTreePrinter printer = build_test_printer<WTreeType>();

  bool res = Test_all_Rules(printer);
  if (!res) {
    test_incorrect(title);
    return EXIT_FAILURE;
  }

  test_correct(title);
  return 0;
}
