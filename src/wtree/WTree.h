#ifndef WTREECLASS_H
#define WTREECLASS_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <stack>
#include <string>
#include <sys/types.h>
#include <vector>

namespace WTreeLib {

// [2] Node structure
template <typename T> class WTreeNode {
public:
  typedef WTreeNode<T> t_self;
  typedef unsigned short t_field;

  enum { init_capacity = 4 };

  // == Atributes
  struct base_fields {
    t_field n;        // 2 bytes (max = 32768)
    t_field capacity; // 2 bytes (max = 32768)
    bool isInternal;  // 1 byte
  };

  struct leaf_fields : public base_fields {
    T *keys = nullptr; // 8 bytes + sizeof(T) * vector.capacity()
  };

  struct internal_fields : public leaf_fields {
    t_self **pointers = nullptr; // 8 bytes + 8 * vector.capacity()
  };

  internal_fields _fields; // 21 -> 24 bytes

  // == Functions

  explicit WTreeNode(const t_field _k);
  WTreeNode(const t_field _k, const t_field init_capacity);
  ~WTreeNode();
  void reserve(const t_field capacity);
  bool setAsInternal(const t_field k);
  bool setAsLeaf();

  bool insert(T key, const t_field k);
  std::vector<bool> insert(const std::vector<T> &keys, const t_field k);
  std::vector<bool> insert(const T *&keys, unsigned int n, const t_field k);
  bool popBack();

  t_field getPosition(T key);
  bool contains(T key);

  void printKeys() const;
  void print() const;
  int getHeight() const;

  unsigned long getTotalBytes() const;
};

// [1] MemRegisterStruct
struct MemRegister {
  struct Level {
    unsigned int level = 0;
    unsigned long keys = 0;
    unsigned long num_nodes = 0;
    unsigned long num_internals = 0;
    unsigned long num_leafs = 0;
    double connectivity = 0.0;

    double acum_connectivity = 0.0;

    void evaluate(unsigned int k) {
      num_leafs = num_nodes - num_internals;
      connectivity =
          num_internals > 0 ? acum_connectivity / (num_internals * (k - 1)) : 0;
    };
  };
  std::vector<Level> levels;

  unsigned long keys = 0;
  unsigned long num_nodes = 0;
  unsigned long height = 0;
  unsigned long num_internals =
      0;                       // Number of internal nodes (with descendents)
  unsigned long num_leafs = 0; // Number of leaf nodes (no descendents)
  unsigned long unused_keycells = 0;  // Unused cells in vector of keys
  unsigned long unused_ptrscells = 0; // Unused cells in vector of pointers

  unsigned long total_bytes = 0;

  double average_bytes_per_key = 0; // Calculated as total_bytes/keys
  double connectivity = 0.0;

  template <typename T, class NODE> void evaluate(unsigned int k) {
    // Evaluate levels:
    for (uint l = 0; l < levels.size(); ++l) {
      Level &level = levels[l];
      assert(level.level == l);
      level.evaluate(k);
      keys += level.keys;
      num_nodes += level.num_nodes;
      num_internals += level.num_internals;
      connectivity += level.acum_connectivity; // use as acum
    }

    // Calculate general values:
    num_leafs = num_nodes - num_internals;
    connectivity = connectivity / (num_internals * (k - 1));

    total_bytes = num_nodes * sizeof(NODE) + // 24 bytes +
                  unused_keycells * sizeof(T) +
                  num_internals * (k - 1) * sizeof(NODE *);
    average_bytes_per_key = (double)total_bytes / keys;
  };

  char *toJsonBody() {
    char *oline = new char[300 + 140 * levels.size()];
    const double fullness = (double)keys / (keys + unused_keycells);
    int send = sprintf(oline,
                       "\"keys\": %10lu,"
                       "\"nodes\": %6lu,"
                       "\"height\": %3lu,\n"
                       "\"internals\": %4lu,"
                       "\"leafs\": %4lu,"
                       "\"unused_keycells\": %6lu,"
                       "\"unused_ptrscells\": %6lu,\n"
                       "\"total_bytes\": %10lu,"
                       "\"overhead\": %f,"
                       "\"fullness\": %f,"
                       "\"connectivity\": %f,\n",
                       keys, num_nodes, height, num_internals, num_leafs,
                       unused_keycells, unused_ptrscells, total_bytes,
                       average_bytes_per_key, fullness, connectivity);
    send += sprintf(oline + send, "\"levels\":[\n");
    for (size_t l = 0; l < levels.size(); ++l) {
      const Level &level = levels[l];
      send += sprintf(oline + send,
                      "\t{"
                      "\"level\": %3u, "
                      "\"keys\": %5lu, "
                      "\"num_nodes\": %4lu, "
                      "\"num_internals\": %4lu, "
                      "\"num_leafs\": %4lu, "
                      "\"connectivity\": %f"
                      "}%c\n",
                      level.level, level.keys, level.num_nodes,
                      level.num_internals, level.num_leafs, level.connectivity,
                      l < levels.size() - 1 ? ',' : ' ');
    }
    sprintf(oline + send, "]");
    return oline;
  };

  void clean() {
    levels.clear();
    std::fill(&keys, &keys + sizeof(MemRegister) - sizeof(std::vector<Level>),
              0);
  };
};

template <typename T> class WTree {
  // W-tree: Data structure for keys management.
  typedef WTree<T> t_wtree;
  typedef WTreeNode<T> t_node;
  using t_field = typename t_node::t_field;

public:
  // [3] Atributes (10 -> 16 bytes)
  const t_field k;
  size_t size = 0;
  t_node *root = nullptr; // 8 bytes

  // [4] Public Functions
  explicit WTree(t_field _k = 4) : k(_k) {}
  ~WTree() {
    if (root)
      delete root;
  }

  inline bool insert(T key) { return _insert(key, nullptr, root, 0); }
  std::vector<bool> insert(const std::vector<T> &keys);
  std::vector<bool> insert(const T *&keys, unsigned int n);

  bool find(T key) const;
  bool find(T key, t_node *&p, t_node *&u, t_field &p_lwb,
            t_field &u_lwb) const;

  T popMin();
  T popMin(t_node *u);

  T popMax();
  T popMax(t_node *u);

  bool remove(T key);
  std::vector<bool> remove(const std::vector<T> &keys);
  std::vector<bool> remove(const T *&keys, unsigned int n);

  // [4.1] Utils
  int getHeight();
  void printTree();

  std::vector<T> getInorder(bool fill = false);
  std::vector<T> getInorderByNode();

  unsigned long getTotalBytes();
  void checkStatistics(MemRegister *reg);
  inline bool verify() { return __verify(root); };

  // [5] Recursive Functions

  // [5.1] Insert
  bool _insert(T key, t_node *p, t_node *u, t_field p_lwb);
  bool trySplit(T key, t_node *p, const t_field p_lwb, t_node *u,
                const t_field lwb);
  bool trySlide(T key, t_node *p, const t_field p_lwb, t_node *u,
                const t_field lwb);

  // [5.2] Search (nothing)
  //    Here just for clarity.

  // [5.3] Remove (nothing)
  //    Here just for clarity.

  // [5.4] Print
  static void printNode(t_node *n, const std::string &prefix, bool isLast);

  // [5.5] Utils
  bool __verify(t_node *u);
  void getInorder(const t_node *u, std::vector<T> &vec, bool &fill);
  void getInorderByNode(const t_node *u, std::vector<T> &vec);
  void _checkStatistics(MemRegister *reg, const t_node *u, unsigned long depth);
};

template class WTreeNode<short>;
template class WTreeNode<unsigned short>;
template class WTreeNode<int>;
template class WTreeNode<unsigned int>;
template class WTreeNode<long>;
template class WTreeNode<unsigned long>;
template class WTreeNode<long long>;
template class WTreeNode<unsigned long long>;
template class WTreeNode<float>;
template class WTreeNode<double>;

template class WTree<short>;
template class WTree<unsigned short>;
template class WTree<int>;
template class WTree<unsigned int>;
template class WTree<long>;
template class WTree<unsigned long>;
template class WTree<long long>;
template class WTree<unsigned long long>;
template class WTree<float>;
template class WTree<double>;
} // namespace WTreeLib

#endif