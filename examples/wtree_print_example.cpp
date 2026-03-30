#include "../include/wtree/set.hpp"

#include "../include/wtree/optional/print.hpp"
#include "../include/wtree/optional/utils.hpp"

#include <limits>

using namespace WTreeLib;

using WUtils = WTreeGenerationUtils;

using KeyType = int;
using WMap = WTreeLib::set<KeyType, 126>; // Small nodes
using WTreeType = WMap::wtree_type;
using NodeType = WTreeType::node_type;
using Field = WTreeType::field_type;

void showcase_improved_printing() {
  WMap storage;
  WTreeType *wt = storage.tree();
  ulong k = WTreeType::kTargetK;

  // Fill some nodes.
  // First, the root node:
  WUtils::fill_root_node(*wt, k, 0, std::numeric_limits<KeyType>::max() / 2);

  // Children in the second level:
  WUtils::fill_child(*wt, wt->root(), k >> 1, k);
  WUtils::fill_child(*wt, wt->root(), (k >> 1) + 1, k / 3);
  WUtils::fill_child(*wt, wt->root(), (k >> 1) - 1, k / 3);

  // Children in the third level:
  wt->root()->set_child(
      k >> 1, wt->manager()->make_internal(wt->root()->child(k >> 1)));
  WUtils::fill_child(*wt, wt->root()->child(k >> 1), k >> 1, k);

  // Example usage with different options
  WTreePrinter<WTreeType>::PrintOptions opts;
  WTreePrinter<WTreeType> defaultPrinter(opts);
  defaultPrinter.print_tree(wt->root());

  // Style 1: Detailed debugging
  opts.show_indices = true;
  opts.show_node_addresses = true;
  opts.show_capacity_info = true;
  opts.compact_values = false;
  opts.max_values_per_line = 6;
  WTreePrinter<WTreeType> debugPrinter(opts);
  debugPrinter.print_tree(wt->root());

  // Style 2: Clean presentation
  opts = {}; // Reset to defaults
  opts.compact_values = true;
  opts.max_values_per_line = 8;
  opts.vertical_spacing = true;
  opts.color_output = true;
  WTreePrinter<WTreeType> presentationPrinter(opts);
  presentationPrinter.print_tree(wt->root());

  // Style 3: Compact overview
  opts = {};
  opts.show_capacity_info = false;
  opts.show_null_ranges = false;
  opts.compact_values = true;
  WTreePrinter<WTreeType> compactPrinter(opts);
  compactPrinter.print_tree(wt->root());
}

int main() {
  showcase_improved_printing();
  return 0;
}