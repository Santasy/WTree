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

#include "../utils/declaration.hpp"

#include "../../include/wtree/optional/print.hpp"
#include "../../include/wtree/optional/utils.hpp"

#include <cstdlib>
#include <ctime>
#include <sstream>

using namespace std;
using namespace WTreeLib;
using namespace TestPrinting;

using WUtils = WTreeGenerationUtils;

const string title = "Locator Test";

using BaseSet = WTreeTestUtil::TestSet<int>;
using Fixture = WTreeTestUtil::TContainerFixture<BaseSet>;
using WTreeType = typename Fixture::wtree_type;
using KeyType = int;
using Printer = WTreePrinter<WTreeType>;

int main(int argc, char **argv) {
    std::cout << "Use " << argv[0] << " <seed>\n\n";

    ulong seed = 42;
    if(argc >= 2)
        seed = atol(argv[1]);

    Fixture fx;
    auto &storage = fx.storage;
    WTreeType *wt = fx.wt();
    bool result = true;
    string job_str;
    ostringstream job_out;

    WTreePrinter printer = build_test_printer<WTreeType>();

    job_str = "Insert some keys.";
    job_title(job_str);
    vector<int> values{3000, 4000, 5000, 6000, 4100, 4200,
                       4300, 4400, 4250, 4260, 4270, 4280};

    for(const int &v : values) {
        auto insert_result = storage.insert(v);
        result &= validate_and_evaluate_test_result(
            job_str, insert_result.second, insert_result.second, *wt);
        if(!result) {
            test_incorrect(title);
            return EXIT_FAILURE;
        }
    }
    printer.print_tree(wt->root());

    job_str = "Search all inserted keys.";
    job_title(job_str);
    for(const int &v : values) {
        auto lookup_result = storage.find(v);
        bool found = (lookup_result != storage.end());
        result &= validate_and_evaluate_test_result(job_str, found, found, *wt);
        if(!result) {
            test_incorrect(title);
            return EXIT_FAILURE;
        }
    }

    job_str = "Search non-existent keys.";
    job_title(job_str);
    for(const int &v : {1, 2, 3, 10001, 10002, 10003, 4229, 4231, 5100}) {
        auto lookup_result = storage.find(v);
        bool not_found = (lookup_result == storage.end());
        result &= validate_and_evaluate_test_result(job_str, not_found,
                                                    not_found, *wt);
        if(!result) {
            test_incorrect(title);
            return EXIT_FAILURE;
        }
    }

    job_str = "Random insertions.";
    job_title(job_str);
    vector<KeyType> inserted;
    for(uint i = 0; i < 100; ++i) {
        int val;
        while(true) {
            val = rand();
            auto insert_result = storage.insert(val);
            if(!insert_result.second)
                continue;
            result = WTreeLib::WTreeValidationUtils::validate_wtree(*wt);
            if(!result) {
                job_bad_result(
                    "Tree validation failed while inserting random values.");
                test_incorrect(title);
                return EXIT_FAILURE;
            }
            break;
        }
        inserted.push_back(val);
    }

    result &= validate_and_evaluate_test_result(job_str, true, true, *wt);
    if(!result) {
        test_incorrect(title);
        return EXIT_FAILURE;
    }

    job_str = "Search all randomly inserted keys.";
    job_title(job_str);
    for(uint i = 0; i < inserted.size(); ++i) {
        const int &v = inserted[i];
        auto lookup_result = storage.find(v);
        bool found = (lookup_result != storage.end());

        if(!found) {
            job_out.str("");
            job_out << "Key = " << v << " was not found.";
            job_bad_result(job_out.str());
            printer.print_tree(wt->root());
            test_incorrect(title);
            return EXIT_FAILURE;
        }

        result &= validate_and_evaluate_test_result(job_str, found, found, *wt);
        if(!result) {
            test_incorrect(title);
            return EXIT_FAILURE;
        }
    }

    test_correct(title);
    return 0;
}