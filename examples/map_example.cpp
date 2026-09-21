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

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "../include/wtree/map.hpp"
#include "../include/wtree/optional/print.hpp"

#include "../test/utils/key_generators.hpp"

#define MAX_SIZE 10'000'000

using namespace std;
using namespace WTreeLib;

using KeyType = int;
using WTreeSet = map<KeyType, KeyType>;
using Printer = WTreePrinter<WTreeSet>;

char ask_operation() {
    string input;
    do {
        cout << "Choose:\n"
             << "\t\t- operation ([i]nsert, [s]earch, [e]rase)\n"
             << "\tothers:\n"
             << "\t\t([p]rint, [c]lose)\n"
             << "> ";
        cin >> input;
        switch(input[0]) {
        case 'i':
        case 's':
        case 'e':
        case 'p':
        case 'c':
            return input[0];
        }
        cout << "Must enter just one of the character first.\n";
    } while(input.size() != 1);
    return '\0';
}

int ask_value(string operation = "insert") {
    cout << "\n> Value to " << operation << ": ";
    int val;
    cin >> val;
    return val;
}

template <typename __WMAP>
bool handle_command(__WMAP &storage, char command, int key) {
    switch(command) {
    case 'i':
        // storage.insert(key);
        break;
    case 's':
        // storage.find(key);
        break;
    case 'e':
        // storage.erase(key);
        break;
    default:
        cout << "Command [" << command << "] not recognized.\n";
        return false;
    }
    return true;
}

int main(int argc, char **argv) {

    int key, i;

    if(argc < 2 || atoi(argv[1]) < 0 || atoi(argv[1]) > MAX_SIZE) {
        printf("Run: %s starting_size(<= %d)\n", argv[0], MAX_SIZE);
        exit(EXIT_FAILURE);
    }

    int inserts = atoi(argv[1]);
    UniformGenerator<KeyType> gen(112233, 4 * inserts, 0);

    WTreeSet storage;

    // Printer printer = Printer();
    // printer.options.color_output = true;

    cout << "Creating from empty tree:\n";
    for(i = 0; i < inserts; ++i) {
        key = gen.get_absent_key();
        storage.insert({key, key * 10});
        // auto ans =
        // if (!ans.second) {
        //   printf("[ERROR] %d was not inserted.\n", key);
        //   exit(EXIT_FAILURE);
        // }
    }
    // printer.print_tree(wt->root());

    cout << "Now the tree accepts operations:\n";
    char command;
    while(true) {
        command = ask_operation();
        if(command == 'c')
            break;

        if(command == 'p') {
            // printer.print_tree(wt->root());
            continue;
        }
        key = ask_value();
        handle_command(storage, command, key);
    }

    printf("\n\n====================\n"
           "\tEnd\n");
    return 0;
}