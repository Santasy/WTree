# WTree
The **W-tree** data structure is intended for use in main memory. It manages ordered elements (or keys) to answer queries in realtime.

WTree is a new dynamic data structure, derived from a variation of a BST, that aims to obtain better empirical results than a traditional BST. Its nodes can contain up to $k$ keys and $(k-1)$ descendant connections, resulting with average height $O(\log n)$ for a fixed $k$ parameter. Consequently, the insert, search and remove operations also have $O(\log n)$ complexity, making it a viable alternative for modern utilization in Computer Science research and the industrial sector.

The implementation is header-only, STL-compliant, and exposes the `WTreeLib::set<Key>` and `WTreeLib::map<Key, Value>` containers following standard container semantics, including iterators, member types and allocators. It accepts unique keys of any practical type (such as `short`, `int`, or `long`) and keeps a tunable node size as a template parameter, with a default target of 512 bytes per node.

Its empirical performance for a uniform-distributed insert sequence of $10^8$ keys, running on a common machine, shows that using $k=2^9$ achieves a $3.5\times$ speedup against the GCC `std::set` container in C++20, which implements a Red-Black Tree, while using $\approx 2.31$ bytes of overhead (additional bytes per stored key) for 4-byte keys. When $k=2^{12}$, memory utilization improves to around 0.65 bytes.

Further research work could implement other low-level techniques and design paradigms to get even better performance under specific contexts.

## Project structure

The project is divided into 3 main folders:

1. `include/wtree/`: Contains the source files of the structure. This folder has the `.hpp` files that declare the `WTreeLib` namespace and the `WTree`, `set`, and `map` classes, among other structures.
1. `test/`: Contains various utilities for correctness validation, as well as other unit tests.
1. `examples/`: Various presentations with comparisons between the developed structures.

## Using CMake

From the root folder (where you can see this `README.md` file), run the following commands to use the cmake process to build relevant targets.

With these commands we first set the tests as compilation targets. By default, all test targets only use their debug version (flag `WTREE_DEBUG_ONLY_TESTS`). The `debug` preset uses the `build/debug/` directory for the output files.

```shell
cmake --preset debug
cmake --build build/debug
```

Run the fast core suites (`insert_rules`, `erase_rules`, `locator`, `unique_set`, `unique_map`, `container_semantics`, `node_size`, `manager`):

```shell
cd build/debug && ctest --output-on-failure
# or
cmake --build build/debug --target run_core_tests
```

The analysis suites (`set_stress`, `map_stress`, `impl_coverage`) are opt-in:

```shell
cmake --preset analysis
cmake --build build/debug-analysis --target run_analysis_tests
```

## See the examples

Some example main files are at `./examples/`. Among them:

- `set_example.cpp` and `map_example.cpp`: basic usage of the `WTreeLib::set` and `WTreeLib::map` containers.
- `string_map.cpp`: a map with string keys.
- `wtree_print_example.cpp`: prints the tree structure.
- `map_profiling_example.cpp`: measures time and memory performance.

## Use the W-tree

To use this W-tree implementation in your C++ project:

1. Add the `include/wtree` folder to your include path (or use CMake's `WTree::Container` target, see below).
2. `#include "wtree/set.hpp"` (or `wtree/map.hpp`).
3. In the `WTreeLib` namespace you will find the STL-compliant `set<Key>` and `map<Key, Value>` containers. They take a **target node size** as a template parameter (defaulting to `WTREE_TARGET_NODE_BYTES`, 512 bytes).

- `WTreeLib::set<Key, Compare, Alloc, TargetNodeSize> set;`
- `WTreeLib::map<Key, Value, Compare, Alloc, TargetNodeSize> map;`

Use example:

```cpp
#include "wtree/set.hpp"

using WTreeLib::set;
set<int> s;
s.insert(42);
```

### Import as a git submodule

This extraction is the standalone WTree library. A consuming project (e.g. the benchmark) should vendor it with `git submodule add`:

```shell
# WTree library itself
git submodule add <wtree_extract-url> libs/wtree

# Then, in the consuming project's CMakeLists.txt:
#   add_subdirectory(libs/wtree)
#   target_link_libraries(<your_target> PRIVATE WTree::Container)
```

### Copy the local include folder to a project:
```bash
cp -u -r -v include/wtree/ <other_project>/include
```

## Using Valgrind

From the root folder, build the target routine you want to test.

```bash
cmake --build build/debug --target insert_rules_test_debug
```

A quick check with Valgrind:

```bash
valgrind --tool=memcheck --error-exitcode=1 ./build/debug/bin/test/insert_rules_test.dbg
```

Full leak check:

```bash
valgrind --tool=memcheck \
  --leak-check=full \
  --show-leak-kinds=definite \
  --num-callers=20 \
  --error-exitcode=1 \
  ./build/debug/bin/test/insert_rules_test.dbg
```

## Format using the .clang-format file

```bash
find . \( -name "*.cpp" -o -name "*.hpp" -o -name "*.tpp" \) \
  -not -path "./build/*" \
  | xargs clang-format -i
```

Use `--dry-run` to preview changes.

## License

WTree is distributed under a dual license split by directory:

- The library (`include/wtree/`) is licensed under the **Apache License, Version 2.0** — see `LICENSE`. If you distribute software built against these headers, retain the Apache notice embedded in each file and include a copy of the license with the headers you vendor.
- `examples/` and `test/` are licensed under the **MIT License** — see `LICENSE-MIT`.

Copyright (c) 2026 Sebastián Pacheco Cáceres. See `NOTICE` for the design attribution of the API surface (inspired by Google's cpp-btree).

## Credits

This implementation was created as part of a thesis for a Master's degree in Computer Science, developed in collaboration with the supervising professor.

Sebastián Pacheco Cáceres, MSC. on Computer Science, UACh Valdivia.

MSc. Thesis: 2021-2024. Posterior work (2025-2026) is part of an incoming paper.

Héctor Ferrada, Ph. D. on Computer Science, UACh.

### Contact:
- **Sebastián Pacheco Cáceres**
  - Email: [sebastian.pacheco@uach.cl](mailto:sebastian.pacheco@uach.cl)
- **Héctor Ferrada**
  - Email: [hferrada@inf.uach.cl](mailto:hferrada@uach.cl)