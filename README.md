# WTree
The **W-tree** data structure is intended for use in main memory. It manages ordered elements (or keys) to answer queries in realtime.\
This data structure is a Binary Search Tree variation such that its nodes can contain up to $k$ keys and $(k-1)$ descendant connections. It has an $O(\log n)$ complexity for its average height for a fixed $k$ parameter. Consequently, the insert, search and remove operations have also an $O(\log n)$ complexity, making it a viable alternative for modern utilization in Computer Science research and the industrial sector.

This is an implementation of a W-tree that accepts unique keys of any practical type (such as short, int, or long), and runs using a shared $k$ parameter among its nodes.
Its empirical performance, running in a common machine, shows that using $k=2^8$ it could achieve a $x3.28$ speedup against the GCC set container in C++17, which implements a Red-Black Tree. At the same time that use $\approx 5.24$ bytes of overhead (additional bytes per stored key) for 4-bytes keys.
A better memory use occurs when $k=2^{12}$, achieving $\approx 1$ byte.

Further research work could implement other low-level techniques and design paradigms to get even better performance under specific contexts.

## Project structure

This project has tree main folders within the `src` folder:
- `wtree/`: Contains the **W-tree** implementation with all its needed code. It has a `.h` file that declares the `WTreeLib` namespace and the `WTree<T>` class, among other structures. Here are some insert and search variants CPP files that define the same functions, and one of them must be chosen for compilation, See the `example/` folder and its makefile to see how to select the WTree CPP files.
- `utils/`: Here is the utility code that was used to implement the experimental routines that measured time and memory performance. This includes the timer and a 'KeyGenerator' class, which handles key generation while keeping track of the values that appear. This generation is capable of parameterizing uniform, normal (gaussian), and symetric bimodal distributions.
- `test/`: This folder contains subfolders for the W-tree's tree-critical operations, namely the insert, search, and removal operations. A makefile in the folder's root compiles a '.a' file and uses it to compile the corresponding test for each subdirectory.
- `example/`: Here is found an example routine that uses a parameterized compilation of the WTree variants using a flexible makefile. First fills the tree to an initial size, then executes a manual insertion and search of keys by user input.

## Use the W-tree

To use this naive W-tree implementation in your C++ project you should:
1. Clone this repository and keep track of the location of the `src/wtree` folder as `<basepath>`.
2. Add `#include "<basepath>/WTree.h"` to the desired C++ code, considering basepath as the path from the file to the `src/wtree/` folder.
3. In the `WTreeLib` namespace you will find the `WTree<T>` data structure. It sets a **target leaf node size** at compile time, and needs to be instantiated with an specific `T` type.

- Use example: `WTree<int> wt; wt.insert(42);`

### Contact:
This implementation was created as part of a tesis for a Master's degree in Computer Science. You can reach the team via the following email addresses:

- **Sebastián Pacheco Cáceres**
  - Email: [sebastian.pacheco@alumnos.uach.cl](mailto:sebastian.pacheco@alumnos.uach.cl) 
- **Héctor Ferrada**
  - Email: [hferrada@inf.uach.cl](mailto:hferrada@inf.uach.cl) 

Dec 2024 - Universidad Austral de Chile