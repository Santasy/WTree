#ifndef _WTREE_TEST_ERASE_RULES_H_
#define _WTREE_TEST_ERASE_RULES_H_

#include <cassert>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <limits>
#include <sys/types.h>

#include "../shared.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"
#include "../../include/wtree/set.hpp"

namespace EraseRulesNamespace {

using namespace std;
using WUtils = WTreeLib::WTreeGenerationUtils;

using __default_test_type = int;
WTREE_TEST_TYPES_PREAMBLE(__default_test_type, 128);
using Printer = WTreeLib::WTreePrinter<WTreeType>;

using TestPrinting::job_title, TestPrinting::job_bad_result,
    TestPrinting::job_correct_result;

bool Test_only_root(Printer printer = Printer()) {
  WTREE_TEST_PREAMBLE(int);

  job_str = "Erase empty WTree";
  job_title(job_str);
  const KeyType neutral_key = 0;
  ans = storage.erase(neutral_key);
  result &= validate_and_evaluate_test_result(job_str, ans, ans == 0, *wt);
  if (!result)
    return result;

  job_str = "Erase first key = 0";
  job_title(job_str);
  insert_result = storage.insert(0);
  assert(insert_result.second);
  ans = storage.erase(0);
  assert(storage.size() == 0);
  result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt);
  if (!result)
    return result;

  result = WUtils::fill_root_node(*wt, k_value, 0, 10000);
  assert(result);
  printer.print_tree(node);

  KeyType target_key = (node->key(0));
  job_out.str("");
  job_out << "Fill root and erase first key = " << target_key;
  job_title(job_out.str());
  ans = storage.erase(target_key);
  printer.print_tree(node);
  result &= validate_and_evaluate_test_result(job_out.str(), ans, ans > 0, *wt);
  if (!result)
    return result;

  result = WUtils::fill_root_node(*wt, k_value, 0, 10000);
  assert(result);
  printer.print_tree(node);

  target_key = (node->key(node->size() - 1));
  job_out.str("");
  job_out << "Fill root and erase last key = " << target_key;
  job_title(job_out.str());
  ans = storage.erase(target_key);
  printer.print_tree(wt->root());
  result &= validate_and_evaluate_test_result(job_out.str(), ans, ans > 0, *wt);
  if (!result)
    return result;

  return true;
}

bool Test_erase_root_children_with_one_value(Printer printer = Printer()) {
  WTREE_TEST_PREAMBLE(int);

  result = WUtils::fill_root_node(*wt, k_value, 0, 10000);
  assert(result);

  job_str = "Erase first child with one key.";
  job_title(job_str);

  KeyType target_key = wt->root()->key(0) + 1;
  storage.insert(target_key);
  printer.print_tree(wt->root());

  ans = storage.erase(target_key);
  printer.print_tree(wt->root());
  assert(wt->root()->child(0) == nullptr);
  result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt);
  if (!result)
    return result;

  job_str = "Erase last child with one key.";
  job_title(job_str);

  target_key = wt->root()->key(wt->root()->size() - 1) - 1;
  storage.insert(target_key);
  printer.print_tree(wt->root());

  ans = storage.erase(target_key);
  assert(wt->root()->child(wt->root()->size() - 2) == nullptr);
  printer.print_tree(wt->root());
  result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt);
  if (!result)
    return result;

  return true;
}

bool Test_erase_inside_root_children(Printer printer = Printer()) {
  WTREE_TEST_PREAMBLE(int);

  result = WUtils::fill_root_node(*wt, k_value, 0, 10000);
  assert(result);

  // == Using first child
  result = WUtils::fill_child(*wt, node, 0, k_value);
  NodeType *child = node->child(0);
  assert(result);

  job_str = "Erase first key.";
  job_title(job_str);

  printer.print_tree(node);
  KeyType target_key = child->key(0);

  ans = storage.erase(target_key);
  printer.print_tree(node);
  result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt);
  if (!result)
    return result;

  job_str = "Erase last key.";
  job_title(job_str);

  target_key = child->last_value();
  storage.insert(target_key);
  printer.print_tree(node);

  ans = storage.erase(target_key);
  printer.print_tree(node);
  result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt);
  if (!result)
    return result;

  job_str = "Erase all keys in child.";
  job_title(job_str);

  std::vector<KeyType> values = construct_values_vector<KeyType>(child);
  for (Field i = 0; i < child->size(); ++i) {
    target_key = values[i];
    printer.print_tree(node);
    cout << "\n> Erasing " << target_key << '\n';
    ans = storage.erase(target_key);
    printer.print_tree(node);
    result &= (ans > 0) && WTreeLib::WTreeValidationUtils::validate_wtree(*wt);
    if (!result)
      break;
  }

  validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt);
  return result;
}

bool Test_internal_erase_replacing_from_left(Printer printer = Printer()) {
  WTREE_TEST_PREAMBLE(int);
  long curr_size;

  job_str = "Erase replacing from left.";
  job_title(job_str);

  result = WUtils::fill_root_node(*wt, k_value, 0, 10000);
  assert(result);
  curr_size = k_value;

  // == Using first child
  result = WUtils::fill_child(*wt, node, 0, k_value);
  assert(result);
  curr_size += k_value;

  job_str = "Erase until leaf node is empty.";
  job_title(job_str);
  while (node->child(0) != nullptr && node->child(0)->size() > 0) {

    printer.print_tree(node);
    KeyType target_key = node->last_value();

    ans = storage.erase(target_key);
    printer.print_tree(node);
    result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt,
                                                --curr_size);
    if (!result) {
      job_bad_result(job_str);
      return result;
    }
  }

  job_str = "Path to leaf at level 3.";
  job_title(job_str);

  curr_size = 0;
  result =
      WUtils::fill_root_node(*wt, k_value, 0, numeric_limits<KeyType>::max());
  curr_size += k_value;
  assert(result);
  result = WUtils::fill_child(*wt, node, 0, k_value);
  curr_size += k_value;
  assert(result);
  wt->manager()->make_child_internal_unchecked(node, 0);
  result = WUtils::fill_child(*wt, node->child(0), 0, k_value);
  curr_size += k_value;
  assert(result);
  assert(WTreeLib::WTreeValidationUtils::validate_wtree(*wt));
  printer.print_tree(node);

  while (node->child(0) != nullptr && node->child(0)->size() > 0) {
    KeyType target_key = node->last_value();
    ans = storage.erase(target_key);
    printer.print_tree(node);
    result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt,
                                                --curr_size);
    if (!result) {
      job_bad_result(job_str);
      return result;
    }
  }

  job_str = "Path to leaf at level 4.";
  job_title(job_str);

  curr_size = 0;
  result =
      WUtils::fill_root_node(*wt, k_value, 0, numeric_limits<KeyType>::max());
  curr_size += k_value;
  assert(result);
  result = WUtils::fill_child(*wt, node, 0, k_value);
  curr_size += k_value;
  assert(result);
  wt->manager()->make_child_internal_unchecked(node, 0);
  result = WUtils::fill_child(*wt, node->child(0), 0, k_value);
  curr_size += k_value;
  assert(result);
  wt->manager()->make_child_internal_unchecked(node->child(0), 0);
  result = WUtils::fill_child(*wt, node->child(0)->child(0), 0, k_value);
  curr_size += k_value;
  assert(result);
  assert(WTreeLib::WTreeValidationUtils::validate_wtree(*wt, curr_size));
  printer.print_tree(node);

  // Now empty the container.
  while (node->size() > 0) {
    KeyType target_key = node->last_value();
    ans = storage.erase(target_key);
    printer.print_tree(node);
    result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt,
                                                --curr_size);
    if (!result) {
      job_bad_result(job_str);
      return result;
    }
  }

  return result;
}

bool Test_internal_erase_replacing_from_right(Printer printer = Printer()) {
  WTREE_TEST_PREAMBLE(int);
  long curr_size;

  job_str = "Erase replacing from right.";
  job_title(job_str);

  result = WUtils::fill_root_node(*wt, k_value, 0, 10000);
  curr_size = k_value;
  assert(result);

  // == Using last child
  Field last_child_index = k_value - 2;
  result = WUtils::fill_child(*wt, node, last_child_index, k_value);
  curr_size += k_value;
  assert(result);

  job_str = "Erase until leaf node is empty.";
  job_title(job_str);
  while (node->child(last_child_index) != nullptr &&
         node->child(last_child_index)->size() > 0) {

    printer.print_tree(node);
    KeyType target_key = node->value(0);

    ans = storage.erase(target_key);
    printer.print_tree(node);
    result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt,
                                                --curr_size);
    if (!result) {
      job_bad_result(job_str);
      return result;
    }
  }

  job_str = "Path to leaf at level 3.";
  job_title(job_str);
  curr_size = 0;

  result =
      WUtils::fill_root_node(*wt, k_value, 0, numeric_limits<KeyType>::max());
  curr_size += k_value;
  assert(result);
  result = WUtils::fill_child(*wt, node, last_child_index, k_value);
  curr_size += k_value;
  assert(result);
  wt->manager()->make_child_internal_unchecked(node, last_child_index);
  result = WUtils::fill_child(*wt, node->child(last_child_index),
                              last_child_index, k_value);
  curr_size += k_value;
  assert(result);
  assert(WTreeLib::WTreeValidationUtils::validate_wtree(*wt, curr_size));
  printer.print_tree(node);

  while (node->child(last_child_index) != nullptr &&
         node->child(last_child_index)->size() > 0) {
    KeyType target_key = node->value(0);
    ans = storage.erase(target_key);
    printer.print_tree(node);
    result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt,
                                                --curr_size);
    if (!result) {
      job_bad_result(job_str);
      return result;
    }
  }

  job_str = "Path to leaf at level 4.";
  job_title(job_str);
  curr_size = 0;

  result =
      WUtils::fill_root_node(*wt, k_value, 0, numeric_limits<KeyType>::max());
  curr_size += k_value;
  assert(result);
  result = WUtils::fill_child(*wt, node, last_child_index, k_value);
  curr_size += k_value;
  assert(result);
  wt->manager()->make_child_internal_unchecked(node, last_child_index);
  result = WUtils::fill_child(*wt, node->child(last_child_index),
                              last_child_index, k_value);
  curr_size += k_value;
  assert(result);
  wt->manager()->make_child_internal_unchecked(node->child(last_child_index),
                                               last_child_index);
  result = WUtils::fill_child(
      *wt, node->child(last_child_index)->child(last_child_index),
      last_child_index, k_value);
  curr_size += k_value;
  assert(result);
  assert(WTreeLib::WTreeValidationUtils::validate_wtree(*wt, curr_size));
  printer.print_tree(node);

  // Now empty the container.
  while (node->size() > 0) {
    KeyType target_key = node->value(0);
    ans = storage.erase(target_key);
    printer.print_tree(node);
    result &= validate_and_evaluate_test_result(job_str, ans, ans > 0, *wt,
                                                --curr_size);
    if (!result) {
      job_bad_result(job_str);
      return result;
    }
  }

  return result;
}
} // namespace EraseRulesNamespace

#endif
