#include "../shared.hpp"

#include "../../include/wtree/set.hpp"
#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

#include <cstdlib>
#include <ctime>
#include <sstream>

const string title = "Locator Test";

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;

using WUtils = WTreeGenerationUtils;

using __default_test_type = int;
WTREE_TEST_TYPES_PREAMBLE(__default_test_type, 128);
using Printer = WTreePrinter<WTreeType>;

int main(int argc, char **argv) {
  std::cout << "Use " << argv[0] << " <seed>\n\n";

  ulong seed = 42;
  if (argc >= 2)
    seed = atol(argv[1]);
  std::srand(seed);

  WTREE_TEST_PREAMBLE(__default_test_type);

  WTreeLib::WTreePrinter printer = build_test_printer<WTreeType>();

  job_str = "Insert some keys.";
  job_title(job_str);
  vector<int> values{3000, 4000, 5000, 6000, 4100, 4200,
                     4300, 4400, 4250, 4260, 4270, 4280};

  for (const int &v : values) {
    insert_result = storage.insert(v);
    result &= validate_and_evaluate_test_result(job_str, insert_result.second,
                                                insert_result.second, *wt);
    if (!result) {
      test_incorrect(title);
      return EXIT_FAILURE;
    }
  }
  printer.print_tree(wt->root());

  job_str = "Search all inserted keys.";
  job_title(job_str);
  for (const int &v : values) {
    lookup_result = storage.find(v);
    bool found = (lookup_result != storage.end());
    result &= validate_and_evaluate_test_result(job_str, found, found, *wt);
    if (!result) {
      test_incorrect(title);
      return EXIT_FAILURE;
    }
  }

  job_str = "Search non-existent keys.";
  job_title(job_str);
  for (const int &v : {1, 2, 3, 10001, 10002, 10003, 4229, 4231, 5100}) {
    lookup_result = storage.find(v);
    bool not_found = (lookup_result == storage.end());
    result &=
        validate_and_evaluate_test_result(job_str, not_found, not_found, *wt);
    if (!result) {
      test_incorrect(title);
      return EXIT_FAILURE;
    }
  }

  job_str = "Random insertions.";
  job_title(job_str);
  vector<KeyType> inserted;
  for (uint i = 0; i < 100; ++i) {
    int val;
    do {
      val = rand();
      insert_result = storage.insert(val);
      result = WTreeLib::WTreeValidationUtils::validate_wtree(*wt);
      if (!result) {
        job_bad_result("Tree validation failed while inserting random values.");
        test_incorrect(title);
        return EXIT_FAILURE;
      }
    } while (!insert_result.second);
    inserted.push_back(val);
  }

  result &= validate_and_evaluate_test_result(job_str, true, true, *wt);
  if (!result) {
    test_incorrect(title);
    return EXIT_FAILURE;
  }

  job_str = "Search all randomly inserted keys.";
  job_title(job_str);
  for (uint i = 0; i < inserted.size(); ++i) {
    const int &v = inserted[i];
    lookup_result = storage.find(v);
    bool found = (lookup_result != storage.end());

    if (!found) {
      job_out.str("");
      job_out << "Key = " << v << " was not found.";
      job_bad_result(job_out.str());
      printer.print_tree(wt->root());
      test_incorrect(title);
      return EXIT_FAILURE;
    }

    result &= validate_and_evaluate_test_result(job_str, found, found, *wt);
    if (!result) {
      test_incorrect(title);
      return EXIT_FAILURE;
    }
  }

  test_correct(title);
  return 0;
}
