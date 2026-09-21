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

#ifndef _WTREE_TEST_STD_PARITY_H_
#define _WTREE_TEST_STD_PARITY_H_

// High-priority STL/btree std-parity checks, reusable by the
// container_semantics and unique_containers suites. Every check runs the
// container under test against the matching std:: container built from the
// same input sequence, so the checks are differential by construction.
//
// Container types are passed as template parameters:
//   SetC : key type int, unique keys (WTreeLib::set<...> or std::set<int>)
//   MapC : key type int, value type int (WTreeLib::map<...> or
//   std::map<int,int>)
//
// Each check prints one [✅]/[❌] verdict line and returns bool.

#include "../../include/wtree/map.hpp"
#include "../../include/wtree/set.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace StdParity {

namespace detail {

// Deterministic-ish sequence of ints with duplicates and spread.
inline std::vector<int> make_input(int n, int seed) {
    std::mt19937 rng(static_cast<uint32_t>(seed));
    std::uniform_int_distribution<int> dist(-4000, 4000);
    std::vector<int> v;
    v.reserve(static_cast<size_t>(n));
    for(int i = 0; i < n; ++i)
        v.push_back(dist(rng));
    return v;
}

inline void verdict(bool ok, const char *name) {
    if(ok)
        std::printf("[✅] %s\n", name);
    else
        std::printf("[❌] %s\n", name);
}

template <typename ItA, typename ItB>
bool range_equal(ItA a1, ItA a2, ItB b1, ItB b2) {
    return std::distance(a1, a2) == std::distance(b1, b2) &&
           std::equal(a1, a2, b1);
}

} // namespace detail

// ---------------------------------------------------------------------------
// Bounds parity: lower_bound / upper_bound / equal_range vs std reference.
// ---------------------------------------------------------------------------

template <typename SetC, typename MapC> bool check_bounds_parity() {
    const char *name = "bounds parity (lower/upper/equal_range vs std)";
    const std::vector<int> data = detail::make_input(3000, 101);

    // Set differential.
    {
        SetC ws;
        std::set<int> rs;
        for(int v : data) {
            ws.insert(v);
            rs.insert(v);
        }
        if(ws.size() != rs.size())
            return detail::verdict(false, name), false;

        std::mt19937 rng(505u);
        std::uniform_int_distribution<int> probe(-4200, 4200);
        for(int i = 0; i < 500; ++i) {
            const int k = probe(rng);
            auto wsb = ws.lower_bound(k);
            auto rsb = rs.lower_bound(k);
            if((wsb == ws.cend()) != (rsb == rs.end()))
                return detail::verdict(false, name), false;
            if(wsb != ws.cend() && *wsb != *rsb)
                return detail::verdict(false, name), false;

            auto wsb_u = ws.upper_bound(k);
            auto rsb_u = rs.upper_bound(k);
            if((wsb_u == ws.cend()) != (rsb_u == rs.end()))
                return detail::verdict(false, name), false;
            if(wsb_u != ws.cend() && *wsb_u != *rsb_u)
                return detail::verdict(false, name), false;

            auto we = ws.equal_range(k);
            auto re = rs.equal_range(k);
            if((we.first == we.second) != (re.first == re.second))
                return detail::verdict(false, name), false;
        }
    }

    // Map differential.
    {
        MapC wm;
        std::map<int, int> rm;
        for(int v : data) {
            wm.emplace(v, v * 2);
            rm.emplace(v, v * 2);
        }
        if(wm.size() != rm.size())
            return detail::verdict(false, name), false;

        std::mt19937 rng(606u);
        std::uniform_int_distribution<int> probe(-4200, 4200);
        for(int i = 0; i < 500; ++i) {
            const int k = probe(rng);
            auto w = wm.equal_range(k);
            auto r = rm.equal_range(k);
            if((w.first == w.second) != (r.first == r.second))
                return detail::verdict(false, name), false;
            if(w.first != w.second && w.first->first != r.first->first)
                return detail::verdict(false, name), false;
        }
    }

    return detail::verdict(true, name), true;
}

// ---------------------------------------------------------------------------
// Range + initializer-list construction and insertion vs std.
// ---------------------------------------------------------------------------

template <typename SetC, typename MapC> bool check_range_and_init_list() {
    const char *name = "range/init-list ctor + insert parity vs std";
    const std::vector<int> data = detail::make_input(1500, 202);

    // Set: range ctor.
    {
        SetC wc(data.begin(), data.end());
        std::set<int> rc(data.begin(), data.end());
        if(!detail::range_equal(wc.cbegin(), wc.cend(), rc.begin(), rc.end()))
            return detail::verdict(false, name), false;
    }
    // Set: init-list ctor (with duplicates).
    {
        SetC wc = {3, 1, 2, 1, 9, 8, 9, 0, 5};
        std::set<int> rc = {3, 1, 2, 1, 9, 8, 9, 0, 5};
        if(!detail::range_equal(wc.cbegin(), wc.cend(), rc.begin(), rc.end()))
            return detail::verdict(false, name), false;
    }
    // Set: range + init-list insert.
    {
        SetC wc;
        std::set<int> rc;
        wc.insert(data.begin(), data.end());
        rc.insert(data.begin(), data.end());
        wc.insert({7, 21, 7, 4});
        rc.insert({7, 21, 7, 4});
        if(!detail::range_equal(wc.cbegin(), wc.cend(), rc.begin(), rc.end()))
            return detail::verdict(false, name), false;
    }

    // Map: init-list ctor (with duplicates).
    {
        MapC wc = {{1, 10}, {2, 20}, {1, 99}, {3, 30}};
        std::map<int, int> rc = {{1, 10}, {2, 20}, {1, 99}, {3, 30}};
        if(!detail::range_equal(wc.cbegin(), wc.cend(), rc.begin(), rc.end()))
            return detail::verdict(false, name), false;
    }

    return detail::verdict(true, name), true;
}

// ---------------------------------------------------------------------------
// Hint-insert: insert(hint), emplace_hint, try_emplace(hint) vs std.
// ---------------------------------------------------------------------------

template <typename SetC, typename MapC> bool check_hint_insert() {
    const char *name =
        "hint-insert parity vs std (insert(hint)/emplace_hint/try_emplace)";
    const std::vector<int> data = detail::make_input(2000, 303);

    // Set: insert with (mostly wrong) hint + emplace_hint.
    {
        SetC wc;
        std::set<int> rc;
        for(size_t i = 0; i < data.size(); ++i) {
            const int v = data[i];
            auto wit = (i & 1) ? wc.cend() : wc.cbegin();
            auto rit = (i & 1) ? rc.cend() : rc.cbegin();
            wc.insert(wit, v);
            rc.insert(rit, v);
            wc.emplace_hint(wc.cend(), v);
            rc.emplace_hint(rc.cend(), v);
        }
        if(wc.size() != rc.size())
            return detail::verdict(false, name), false;
        if(!detail::range_equal(wc.cbegin(), wc.cend(), rc.begin(), rc.end()))
            return detail::verdict(false, name), false;
    }

    // Map: emplace_hint + try_emplace(hint) + insert(hint, value_type).
    {
        MapC wm;
        std::map<int, int> rm;
        for(size_t i = 0; i < data.size(); ++i) {
            const int v = data[i];
            auto wit = wm.cend();
            auto rit = rm.cend();
            wm.emplace_hint(wit, v, v);
            rm.emplace_hint(rit, v, v);
            wm.try_emplace(wm.cend(), v, -v);
            rm.try_emplace(rm.cend(), v, -v);
            wm.insert(wm.cend(), typename MapC::value_type(v, v));
            rm.insert(rm.cend(), typename std::map<int, int>::value_type(v, v));
        }
        if(wm.size() != rm.size())
            return detail::verdict(false, name), false;
        if(!detail::range_equal(wm.cbegin(), wm.cend(), rm.begin(), rm.end()))
            return detail::verdict(false, name), false;
    }

    return detail::verdict(true, name), true;
}

// ---------------------------------------------------------------------------
// swap: member swap + ADL/qualified free swap vs std behavior.
// ---------------------------------------------------------------------------

template <typename SetC, typename MapC> bool check_swap_parity() {
    const char *name = "swap parity (member + free) vs std";
    {
        SetC a, b;
        a.emplace(1);
        a.emplace(3);
        b.emplace(2);
        SetC ra = a; // references reflect contents before swap
        SetC rb = b;
        a.swap(b);
        if(a.size() != rb.size() || b.size() != ra.size())
            return detail::verdict(false, name), false;
        if(!detail::range_equal(a.cbegin(), a.cend(), rb.cbegin(), rb.cend()))
            return detail::verdict(false, name), false;
        if(!detail::range_equal(b.cbegin(), b.cend(), ra.cbegin(), ra.cend()))
            return detail::verdict(false, name), false;
    }
    {
        MapC a, b;
        a.emplace(1, 100);
        b.emplace(2, 200);
        b.emplace(3, 300);
        MapC ra = a;
        MapC rb = b;
        using std::swap;
        swap(a, b); // ADL finds WTreeLib::swap / std::swap
        if(a.size() != rb.size() || b.size() != ra.size())
            return detail::verdict(false, name), false;
        if(!detail::range_equal(a.cbegin(), a.cend(), rb.cbegin(), rb.cend()))
            return detail::verdict(false, name), false;
        if(!detail::range_equal(b.cbegin(), b.cend(), ra.cbegin(), ra.cend()))
            return detail::verdict(false, name), false;
    }
    return detail::verdict(true, name), true;
}

// ---------------------------------------------------------------------------
// insert_or_assign (map only) vs std::map.
// ---------------------------------------------------------------------------

template <typename MapC> bool check_insert_or_assign() {
    const char *name = "insert_or_assign parity vs std::map";
    const std::vector<int> data = detail::make_input(1500, 404);
    MapC wm;
    std::map<int, int> rm;
    // Interleave fresh inserts and assignments via insert_or_assign.
    for(size_t i = 0; i < data.size(); ++i) {
        const int k = data[i];
        const int v = static_cast<int>(i % 97);
        wm.insert_or_assign(k, v);
        rm.insert_or_assign(k, v);
        if((i % 7) == 0) {
            wm.insert_or_assign(k, v + 1);
            rm.insert_or_assign(k, v + 1);
        }
    }
    if(wm.size() != rm.size())
        return detail::verdict(false, name), false;
    if(!detail::range_equal(wm.cbegin(), wm.cend(), rm.begin(), rm.end()))
        return detail::verdict(false, name), false;
    return detail::verdict(true, name), true;
}

// ---------------------------------------------------------------------------
// Run the full high-priority parity battery.
// ---------------------------------------------------------------------------

template <typename SetC, typename MapC> bool run_all() {
    bool ok = true;
    ok &= check_bounds_parity<SetC, MapC>();
    ok &= check_range_and_init_list<SetC, MapC>();
    ok &= check_hint_insert<SetC, MapC>();
    ok &= check_swap_parity<SetC, MapC>();
    ok &= check_insert_or_assign<MapC>();
    return ok;
}

} // namespace StdParity

#endif