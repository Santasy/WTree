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

#ifndef _WTREE_TEST_DECLARATION_H_
#define _WTREE_TEST_DECLARATION_H_

// Test-declaration helpers: instantiate containers and access the underlying
// WTree without the variable-injecting PREAMBLE macros of old. Declarations
// live here; each test function builds only what it uses.

#include "testing.hpp"

#include <set>
#include <map>

namespace WTreeTestUtil {

using WTreeLib::WTreeBalanceOptions;
using WTreeLib::WTreePrinter;
using WTreeLib::WTreeValidationUtils;

// Default node size for the fast core suites (was
// __WTREE_TEST_DEFAULT_NODE_SIZE).
constexpr int kTestDefaultNodeBytes = 128;

// Type-alias templates (types only, never variables).
template <typename Key, int NodeBytes = kTestDefaultNodeBytes,
          typename Balance = WTreeBalanceOptions<>>
using TestSet =
    WTreeLib::set<Key, std::less<Key>, std::allocator<Key>, NodeBytes, Balance>;

template <typename Key, typename Value, int NodeBytes = kTestDefaultNodeBytes,
          typename Balance = WTreeBalanceOptions<>>
using TestMap = WTreeLib::map<Key, Value, std::less<Key>,
                              std::allocator<std::pair<const Key, Value>>,
                              NodeBytes, true, Balance>;

// Tag types for container selection
struct WTreeContainerTag {};
struct StdContainerTag {};

// Map container selector
template <typename Key, typename Value, typename Tag> struct MapSelector;

template <typename Key, typename Value>
struct MapSelector<Key, Value, WTreeContainerTag> {
    using type =
        WTreeLib::map<Key, Value, std::less<Key>, std::allocator<Key>, 256>;
    static constexpr const char *name = "WTreeLib::map";
};

template <typename Key, typename Value>
struct MapSelector<Key, Value, StdContainerTag> {
    using type = std::map<Key, Value>;
    static constexpr const char *name = "std::map";
};

// Set container selector
template <typename Key, typename Tag> struct SetSelector;

template <typename Key> struct SetSelector<Key, WTreeContainerTag> {
    using type = WTreeLib::set<Key, std::less<Key>, std::allocator<Key>, 256>;
    static constexpr const char *name = "WTreeLib::set";
};

template <typename Key> struct SetSelector<Key, StdContainerTag> {
    using type = std::set<Key>;
    static constexpr const char *name = "std::set";
};

// Fixture owning a container and exposing the usual tree/node accessors.
template <typename Container> struct TContainerFixture {
    Container storage;
    using container_type = Container;
    using wtree_type = typename Container::wtree_type;
    using node_type = typename wtree_type::node_type;
    using params_type = typename wtree_type::params_type;

    wtree_type *wt() { return storage.tree(); }
    node_type *root() { return wt()->root(); }

    bool validate(long expected_values = -1) {
        return WTreeValidationUtils::validate_wtree(*wt(), expected_values);
    }

    WTreePrinter<wtree_type> printer() {
        WTreePrinter<wtree_type> p;
        p.options.color_output = true;
        p.options.compact_values = true;
        return p;
    }
};

} // namespace WTreeTestUtil

#endif