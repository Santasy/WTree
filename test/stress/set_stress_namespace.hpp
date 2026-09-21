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

#ifndef _WTREE_TEST_SET_STRESS_H_
#define _WTREE_TEST_SET_STRESS_H_

#include <cassert>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <limits>
#include <sys/types.h>

#include "../utils/declaration.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

#include "../utils/key_generators.hpp"

namespace SetStressNamespace {

using namespace std;
using WUtils = WTreeLib::WTreeGenerationUtils;

using TestPrinting::job_title, TestPrinting::job_bad_result,
    TestPrinting::job_correct_result,
    TestPrinting::validate_and_evaluate_test_result;

template <typename T, int SIZE = 256> bool Test_massive_insert() {
    using SetType = WTreeLib::set<T, std::less<T>, std::allocator<T>, SIZE>;
    using Fixture = WTreeTestUtil::TContainerFixture<SetType>;

    Fixture fx;
    auto &storage = fx.storage;
    bool result = true;
    std::ostringstream job_out;

    const ulong maxval = min(1'000'000LU, (ulong)numeric_limits<T>::max());

    job_out.str("");
    job_out << "Massive set insert test with " << (maxval >> 1) << " keys"
            << " (key=" << sizeof(T) << "B, node=" << SIZE << "B).";
    job_title(job_out.str());

    UniformGenerator<T> gen(0, maxval);
    vector<T> values(maxval >> 1);

    ulong i;
    T val;
    bool is_correct = true;

    for(i = 0; i < maxval >> 1; ++i) {
        val = gen.get_absent_key();
        values[i] = val;
        auto local_insert_result = storage.insert(val);
        is_correct &= local_insert_result.second;
        assert(local_insert_result.first.key() == val);
        is_correct &=
            WTreeLib::WTreeValidationUtils::validate_iterator_path(
                local_insert_result.first);
        assert(is_correct);
    }

    is_correct &=
        WTreeLib::WTreeValidationUtils::validate_wtree(*storage.tree(), i);
    result &= validate_and_evaluate_test_result(job_out.str(), is_correct,
                                                is_correct, *storage.tree());

    return result;
}

} // namespace SetStressNamespace

#endif