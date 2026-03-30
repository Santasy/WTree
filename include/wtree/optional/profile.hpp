#ifndef _WTREE_PROFILING_H_
#define _WTREE_PROFILING_H_

#include "../detail/index.hpp"

namespace WTreeLib {

struct WTreeMemoryInstrument {
  struct Level {
    unsigned int level = 0;
    unsigned long keys = 0;
    unsigned long num_nodes = 0;
    unsigned long num_internals = 0;
    unsigned long num_leaves = 0;
    double connectivity = 0.0;

    double acum_connectivity = 0.0;

    void evaluate(unsigned int k) {
      num_leaves = num_nodes - num_internals;
      connectivity =
          num_internals > 0 ? acum_connectivity / (num_internals * (k - 1)) : 0;
    };
  };
  std::vector<Level> levels;

  unsigned long keys = 0;
  unsigned long num_nodes = 0;
  unsigned long height = 0;
  unsigned long num_internals =
      0;                        // Number of internal nodes (with descendents)
  unsigned long num_leaves = 0; // Number of leaves nodes (no descendents)
  unsigned long unused_keycells = 0; // Unused cells in vector of keys
  unsigned long unused_ptrcells = 0; // Unused cells in vector of pointers

  unsigned long total_bytes = 0;

  double average_bytes_per_key = 0; // Calculated as total_bytes/keys
  double connectivity = 0.0;

  template <typename T, class NODE> void evaluate() {
    const uint k_value = NODE::kTargetK;
    // Evaluate levels:
    for (unsigned int l = 0; l < levels.size(); ++l) {
      Level &level = levels[l];
      assert(level.level == l);
      level.evaluate(k_value);
      keys += level.keys;
      num_nodes += level.num_nodes;
      num_internals += level.num_internals;
      connectivity += level.acum_connectivity; // use as acum
    }

    // Calculate general values:
    height = levels.size() - 1;
    num_leaves = num_nodes - num_internals;
    connectivity =
        num_internals > 0 ? connectivity / (num_internals * (k_value - 1)) : 0;

    total_bytes = num_nodes * NODE::kBasefieldsBytes +
                  unused_keycells * sizeof(T) +
                  num_internals * (k_value - 1) * sizeof(void *);
    average_bytes_per_key = keys > 0 ? (double)total_bytes / keys : total_bytes;
  };

  char *toJsonBody() {
    char *oline = new char[1024 + (512 * levels.size())];
    const double fullness = (double)keys / (keys + unused_keycells);
    int send = sprintf(oline,
                       "\"keys\": %10lu,"
                       "\"nodes\": %6lu,"
                       "\"height\": %3lu,\n"
                       "\"internals\": %4lu,"
                       "\"leaves\": %4lu,"
                       "\"unused_keycells\": %6lu,"
                       "\"unused_ptrcells\": %6lu,\n"
                       "\"total_bytes\": %10lu,"
                       "\"overhead\": %f,"
                       "\"fullness\": %f,"
                       "\"connectivity\": %f,\n",
                       keys, num_nodes, height, num_internals, num_leaves,
                       unused_keycells, unused_ptrcells, total_bytes,
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
                      "\"num_leaves\": %4lu, "
                      "\"connectivity\": %f"
                      "}%c\n",
                      level.level, level.keys, level.num_nodes,
                      level.num_internals, level.num_leaves, level.connectivity,
                      l < levels.size() - 1 ? ',' : ' ');
    }
    sprintf(oline + send, "]");
    return oline;
  };

  void clean() {
    levels.clear();
    std::fill(
        &keys,
        &keys + sizeof(WTreeMemoryInstrument) - sizeof(std::vector<Level>), 0);
  };
};

template <typename Params> struct WTreeProfiler : public WTree<Params> {
  using wtree_type = WTree<Params>;
  using node_type = typename wtree_type::node_type;

public:
  /**
  Call the recursive function to checkStatistics from the root of the tree.
  This inclues:
  - num_nodes, num_internals, num_leafs
  - unused_keycells: Unused cells in vector of values
  - unused_ptrcells: Unused cells in vector of pointers
  - connectivity: Connectivity is the ratio of used pointer connections over
  all the allocated pointer cells.
  */
  static void check_statistics(wtree_type &sref, WTreeMemoryInstrument &reg) {
    if (sref.root() == nullptr)
      return;
    _check_statistics(reg, sref.root(), 0);
    reg.evaluate<Params, node_type>();
  }

private:
  /**
    Calculate all the implemented statistics of the tree from a particular node.
  */
  static void _check_statistics(WTreeMemoryInstrument &reg, const node_type *u,
                                ulong depth) {
    assert(u);
    const uint k_value = node_type::kTargetK;

    // First ensure the level exists
    for (size_t i = reg.levels.size(); i < depth + 1; ++i) {
      reg.levels.push_back(WTreeMemoryInstrument::Level());
      reg.levels[i].level = i;
    }

    reg.levels[depth].keys += u->size();
    ++reg.levels[depth].num_nodes;
    reg.unused_keycells += u->capacity() - u->size();

    if (u->is_internal()) {
      ++reg.levels[depth].num_internals;
      for (typename wtree_type::field_type i = 0; i < k_value - 1; ++i) {
        if (u->child(i)) {
          ++reg.levels[depth].acum_connectivity;
          _check_statistics(reg, u->child(i), depth + 1);
        } else
          (reg.unused_ptrcells)++;
      }
    }
  }
};

} // namespace WTreeLib
#endif
