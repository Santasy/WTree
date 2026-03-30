#include "../shared.hpp"

#include "erase_rules_namespace.hpp"

#include "../../include/wtree/set.hpp"
#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>

const std::string title = "Erase Rules Test";

using namespace std;
using namespace TestPrinting;
using namespace EraseRulesNamespace;

using WUtils = WTreeLib::WTreeGenerationUtils;

using KeyType = int;
using WSet = WTreeLib::set<KeyType, 128>;
using WTreeType = WSet::wtree_type;
using NodeType = WTreeType::node_type;
using Field = WTreeType::field_type;
using WITerator = WSet::iterator;

using Printer = WTreeLib::WTreePrinter<WTreeType>;

int main() {
  Printer printer;
  printer.options.color_output = true;
  bool result = true;

  result &= Test_only_root(printer);
  if (!result) {
    test_incorrect(title);
    return EXIT_FAILURE;
  }

  result &= Test_erase_root_children_with_one_value(printer) &&
            Test_erase_inside_root_children(printer) &&
            Test_internal_erase_replacing_from_left(printer) &&
            Test_internal_erase_replacing_from_right(printer);
  if (!result) {
    test_incorrect(title);
    return EXIT_FAILURE;
  }

  TestPrinting::test_correct(title);
  return 0;
}
