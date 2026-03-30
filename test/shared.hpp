#ifndef _WTREE_TEST_SHARED_H_
#define _WTREE_TEST_SHARED_H_

#include "../include/wtree/detail/index.hpp"

#include "../include/wtree/optional/print.hpp"
#include "../include/wtree/optional/utils.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::cin;
using std::cout;
using std::string;
using std::vector;

// ============================================================================
// Logging — all test output goes through these helpers.
// Only two emojis: ✅ (pass) and ❌ (fail).
// ============================================================================

namespace TestPrinting {

void test_correct(string title) {
  printf("\n\n====================\n"
         "[✅] %s passed.\n",
         title.c_str());
}

void test_incorrect(string title) {
  printf("\n\n====================\n"
         "[❌] %s did not pass.\n",
         title.c_str());
}

void job_title(string title) {
  cout << "\n==========\n";
  printf("[[TEST]] %s\n", title.c_str());
}

void job_correct_result(string description = "") {
  printf("[✅] %s\n", description.c_str());
  cout << "==========\n";
}

void job_bad_result(string description = "") {
  printf("[❌] %s\n", description.c_str());
  cout << "==========\n";
}

void print_dbg_correct_msg(string msg) {
#ifdef DBGVERBOSE
  TestPrinting::job_correct_result("[IN JOB] " + msg);
#endif
}

} // namespace TestPrinting

// ============================================================================
// TestResults — pass/fail counter for tests with many sub-cases.
// ============================================================================

struct TestResults {
  int passed = 0;
  int failed = 0;

  void pass(const string &test_name) {
    cout << "[✅] " << test_name << "\n";
    passed++;
  }

  void fail(const string &test_name, const string &reason = "") {
    cout << "[❌] " << test_name;
    if (!reason.empty())
      cout << " - " << reason;
    cout << "\n";
    failed++;
  }

  void summary() const {
    cout << "\n========================================\n";
    cout << "TOTAL: " << (passed + failed) << " tests\n";
    cout << "PASSED: " << passed << "\n";
    cout << "FAILED: " << failed << "\n";
    cout << "========================================\n";
  }

  bool all_passed() const { return failed == 0; }
};

// ============================================================================
// Evaluation helpers
// ============================================================================

#define __WTREE_TEST_DEFAULT_NODE_SIZE 128

#define WTREE_TEST_TYPES_PREAMBLE(__KEY_TYPE, __SIZE)                          \
  using KeyType = __KEY_TYPE;                                                  \
  using WSet = WTreeLib::set<KeyType, __SIZE>;                                 \
  using WTreeType = typename WSet::wtree_type;                                 \
  using NodeType = typename WTreeType::node_type;                              \
  using Field = typename WTreeType::field_type;                                \
  using WIter = typename WSet::iterator;                                       \
  using InsertResult = typename WSet::InsertResult;

#define WTREE_TEST_PREAMBLE_WITH_SIZE(__KEY_TYPE, __SIZE)                      \
  WTREE_TEST_TYPES_PREAMBLE(__KEY_TYPE, __SIZE);                               \
  const Field k_value = WTreeType::kTargetK;                                   \
  WSet storage;                                                                \
  WTreeType *wt = storage.tree();                                              \
  NodeType *node = wt->root();                                                 \
  string job_str;                                                              \
  ostringstream job_out;                                                        \
  InsertResult insert_result;                                                  \
  WIter lookup_result;                                                         \
  int ans = -1;                                                                \
  bool result = true;

#define WTREE_TEST_PREAMBLE(__KEY_TYPE)                                        \
  WTREE_TEST_PREAMBLE_WITH_SIZE(__KEY_TYPE, __WTREE_TEST_DEFAULT_NODE_SIZE)

template <typename T>
bool evaluate_test_result(string job_str, T result, bool is_valid,
                          string description = "") {
  std::ostringstream job_out;
  if (!is_valid) {
    if (!description.empty()) {
      job_out << description;
    } else {
      job_out << "result = " << result;
    }
    TestPrinting::job_bad_result(job_out.str());
    return false;
  }
  TestPrinting::print_dbg_correct_msg(job_str);
  return true;
}

template <typename Params, typename T>
bool validate_and_evaluate_test_result(string job_str, T result, bool is_valid,
                                       WTreeLib::WTree<Params> &wt,
                                       long expected_values = -1,
                                       string description = "") {
  std::ostringstream job_out;
  if (!is_valid ||
      !WTreeLib::WTreeValidationUtils::validate_wtree(wt, expected_values)) {
    if (!description.empty()) {
      job_out << description;
    } else {
      job_out << "result = " << result;
    }
    TestPrinting::job_bad_result(job_out.str());
    return false;
  }
  TestPrinting::print_dbg_correct_msg(job_str);
  return true;
}

// ============================================================================
// Helpers
// ============================================================================

template <typename WTreeType>
WTreeLib::WTreePrinter<WTreeType> build_test_printer() {
  WTreeLib::WTreePrinter<WTreeType> printer;
  printer.options.color_output = true;
  printer.options.compact_values = true;
  return printer;
}

template <typename T, typename Params>
vector<T> construct_values_vector(WTreeLib::WTreeNode<Params> *node) {
  T *begin = node->fields.values;
  return vector<T>(begin, begin + node->size());
}

template <typename T> void print_vector(vector<T> &v, string title = "V: ") {
  cout << title << "\n";
  for (const T &val : v) {
    cout << val << " ";
  }
  cout << "\n";
}

#endif
