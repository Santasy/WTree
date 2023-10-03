# WTree
The **W-tree** data structure is intended for use in main memory. It manages ordered elements (or keys) to answer queries in realtime.\
This data structure is a Binary Search Tree variation such that its nodes can contain up to $k$ keys and $(k-1)$ descendant connections. It has a $O(\log n)$ complexity for its height and $O(k \cdot \log n)$ for the insert, search and remove operations, making it a viable alternative for modern utilization in Computer Science research and the industrial sector.

This is a rudimentary implementation of a W-tree that accepts unique keys of any practical type (such as short, int, or long), and runs using a shared $k$ parameter among its nodes.
Was develop for C++17, and implements the crucial operations to insert, search and remove keys.\
Its empirical performance, running in a common machine, shows that using $k=2^9$ it could achieve a x2.0 speedup against the GCC set container in C++17, which implements a Red-Black Tree, at the same time that use $\approx 1.92$ bytes of overhead (additional bytes per stored key).
Further work could implement other low-level techniques and design paradigms to get even better performance.

## Project structure

This project has tree main folders within the `src` folder:
- `wtree/`: Contains the **W-tree** implementation with all its needed code. It has a `.h` file that declares the `WTreeLib` namespace and the `WTree<T>` class, among other structures. This `.h` file must be included in the desired C++ code, and all its `.cpp` files must be included for compilation.
- `utils/`: Here is the utility code that was used to implement the experimental routines that measured time and memory performance. This includes the timer and a 'KeyGenerator' class, which handles key generation while keeping track of the values that appear. This generation is capable of parameterizing uniform, normal (gaussian), and symetric bimodal distributions.
- `Test/`: This folder contains subfolders for the W-tree's tree-critical operations, namely the insert, search, and removal operations. A makefile in the folder's root compiles a '.a' file and uses it to compile the corresponding test for each subdirectory.

## Use the W-tree

To use this naive W-tree implementation in your C++ project you should:
1. Clone this repository and keep track of the location of the `src/wtree/` folder.
2. Add `#include "<basepath>/WTree.h"` to the desired C++ code, considering basepath as the path from the file to the `src/wtree/` folder.
3. In the `WTreeLib` namespace you will find the `WTree<T>` data structure. It needs to be instantiated with an specific `T` type, and use a particular `k` value for its constructor.

- Use example: `WTree<int> wt(256); wt.insert(42);`.
You can also found more examples at the routines in the subfolders of the `src/Test/` folder, which also has a `makefile` for its compilation.

### Contact:
This implementation was created as part of a tesis for a Master's degree in Computer Science. You can reach the team via the following email addresses:

- **Sebastián Pacheco Cáceres**
  - Email: [sebastian.pacheco@alumnos.uach.cl](mailto:sebastian.pacheco@alumnos.uach.cl) 
- **Héctor Ferrada**
  - Email: [hferrada@inf.uach.cl](mailto:hferrada@inf.uach.cl) 

Oct 2023 - Universidad Austral de Chile