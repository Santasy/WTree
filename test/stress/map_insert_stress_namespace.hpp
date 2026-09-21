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

#ifndef _WTREE_TEST_MAP_INSERT_STRESS_H_
#define _WTREE_TEST_MAP_INSERT_STRESS_H_

#include <cassert>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <sys/types.h>

#include "../utils/testing.hpp"

#include "../../include/wtree/map.hpp"
#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

#include "../utils/key_generators.hpp"

namespace MapInsertStressNamespace {

using namespace std;
using WUtils = WTreeLib::WTreeGenerationUtils;

using TestPrinting::job_title, TestPrinting::job_bad_result,
    TestPrinting::job_correct_result,
    TestPrinting::validate_and_evaluate_test_result;

template <typename Key, typename Value, int SIZE> struct MapTestTypes {
    using WMap =
        WTreeLib::map<Key, Value, std::less<Key>, std::allocator<Key>, SIZE>;
    using WTreeType = typename WMap::wtree_type;
    using NodeType = typename WTreeType::node_type;
    using Field = typename WTreeType::field_type;
    using WIter = typename WMap::iterator;
    using InsertResult = typename WMap::InsertResult;
};

template <int SIZE = 256>
bool Test_massive_insert_map(
    WTreeLib::WTreePrinter<
        typename MapTestTypes<ulong, queue<ulong>, SIZE>::WTreeType>
        printer = WTreeLib::WTreePrinter<
            typename MapTestTypes<ulong, queue<ulong>, SIZE>::WTreeType>()) {

    using Types = MapTestTypes<ulong, queue<ulong>, SIZE>;
    using WMap = typename Types::WMap;
    using WTreeType = typename Types::WTreeType;

    WMap storage;
    WTreeType *wt = storage.tree();
    ostringstream job_out;
    bool result = true;

    const ulong maxval = 1'000'000LU;

    job_out.str("");
    job_out << "Massive map insert test with " << (maxval >> 1)
            << " keys, SIZE=" << SIZE << ".";
    job_title(job_out.str());

    UniformGenerator<ulong> gen(0, maxval);

    ulong i;
    ulong key;
    bool is_correct = true;

    for(i = 0; i < maxval >> 1; ++i) {
        key = gen.get_absent_key();

        queue<ulong> val;
        val.push(key * 3 + 1);
        val.push(key * 7 + 13);
        val.push(key * 11 + 37);

        auto local_insert_result = storage.insert({key, std::move(val)});
        is_correct &= local_insert_result.second;
        assert(local_insert_result.first->first == key);
        is_correct &=
            WTreeLib::WTreeValidationUtils::validate_map_iterator_path(
                local_insert_result.first, [](ulong k, const queue<ulong> &q) {
                    return q.size() == 3 && q.front() == (k * 3) + 1;
                });
        assert(is_correct);
    }

    is_correct &=
        WTreeLib::WTreeValidationUtils::validate_wtree(*storage.tree(), i);
    result &= validate_and_evaluate_test_result(job_out.str(), is_correct,
                                                is_correct, *storage.tree());

    return result;
}

} // namespace MapInsertStressNamespace

#endif
