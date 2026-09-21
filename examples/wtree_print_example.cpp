/*
 * Copyright (c) 2026 Sebastián Pacheco Cáceres
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "../include/wtree/set.hpp"

#include "../include/wtree/optional/print.hpp"
#include "../include/wtree/optional/utils.hpp"

#include <limits>

using namespace WTreeLib;

using WUtils = WTreeGenerationUtils;

using KeyType = int;
using WSet = WTreeLib::set<KeyType>;
using WTreeType = WSet::wtree_type;
using NodeType = WTreeType::node_type;
using Field = WTreeType::field_type;

void showcase_improved_printing() {
    WSet storage;
    WTreeType *wt = storage.tree();
    WTreeType::manager_type *manager = wt->manager();
    ulong k = WTreeType::kTargetK;

    // Fill some nodes.
    // First, the root node:
    WUtils::fill_root_node(*wt, k, 0, std::numeric_limits<KeyType>::max() / 2);

    // Children in the second level:
    WUtils::fill_child(*wt, wt->root(), k >> 1, k);
    WUtils::fill_child(*wt, wt->root(), (k >> 1) + 1, k / 3);
    WUtils::fill_child(*wt, wt->root(), (k >> 1) - 1, k / 3);

    // Children in the third level:
    wt->root()->set_child(k >> 1,
                          manager->make_internal(wt->root()->child(k >> 1)));
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