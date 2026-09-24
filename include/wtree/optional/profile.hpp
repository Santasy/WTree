/*
 * Copyright (c) 2026 Sebastián Pacheco Cáceres
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _WTREE_PROFILING_H_
#define _WTREE_PROFILING_H_

#include "../detail/index.hpp"

#include "../detail/containers.hpp"

#include <format>

namespace WTreeLib {

/**
 * @brief Extended tree statistics: a node_stats plus a per-level breakdown.
 * @details Inherits the tree's @ref WTree "node_stats" aggregate (so any
 * node_stats method — nodes(), bytes_used(), fullness(), occupancy()... — is
 * callable on the instrument), and keeps a per-level record (levels). This is
 * the difference with the tree's inner collect_stats(), which only produces
 * the aggregate. Fill it via WTreeProfiler::check_statistics.
 * @tparam Params A @ref WTree "WTree<Params>" instantiation, where Params is
 *         @ref WTreeLib::WTreeSetParams "WTreeSetParams" or @ref
 *         WTreeLib::WTreeMapParams "WTreeMapParams".
 */
template <typename Params>
class WTreeMemoryInstrument : public WTree<Params>::node_stats {
  public:
    using wtree_type = WTree<Params>;
    using node_type = wtree_type::node_type;
    using size_type = wtree_type::size_type;
    using value_type = wtree_type::value_type;
    using stats_type = wtree_type::node_stats;

    static constexpr size_type kBasefieldsBytes = node_type::kBasefieldsBytes;

    /**
     * @brief Per-level statistics of the referenced tree.
     * @details Counts the keys, nodes and pointer cells present at one level,
     * with the same semantics as node_stats.
     */
    class Level : public WTree<Params>::node_stats {
      public:
        /**
         * @brief Creates a Level at some hight. Root is created at height=0.
         */
        Level(size_t height = 0) { this->height = height; };

        size_t level() const noexcept { return this->height; }

        // The total number of bytes used by the level (object + all allocated
        // node storage: base fields, key cells and internal pointer arrays).
        size_type bytes_used() const {
            size_t leaves_keycells;
            if(this->height == 0) { // Root is a special case.
                leaves_keycells =
                    this->keys < this->target_k ? 0 : this->keys_in_leaves();
            } else {
                leaves_keycells = this->keys_in_leaves();
            }
            const size_type internals_memory =
                this->internal_nodes * sizeof(node_type);
            const size_type leaves_memory =
                (this->leaf_nodes * kBasefieldsBytes) +
                (leaves_keycells * sizeof(value_type));
            return internals_memory + leaves_memory;
        }

        // The average number of bytes used per value stored in the level,
        // including overhead memory.
        double average_bytes_per_value() const {
            const size_type total = bytes_used();
            return this->keys > 0 ? total / double(this->keys) : 0;
        }

        // The total overhead of the level in bytes.
        size_type total_overhead() const {
            return bytes_used() - (this->keys * sizeof(value_type));
        }

        // The overhead of the level in bytes per value.
        // Returns zero when no keys are stored.
        double overhead() const {
            const size_type tov = total_overhead();
            return this->keys ? tov / double(this->keys) : 0.0;
        }
    };

    std::vector<Level> levels;

    /**
     * @brief Also returns the last created level.
     */
    Level *ensure_level_exists(size_t depth) noexcept {
        for(size_t i = levels.size(); i < depth + 1; ++i)
            levels.push_back(Level(i));
        return &levels[levels.size() - 1];
    }

    /**
     * @brief Finalizes the per-level records from the accumulated counts.
     */
    void evaluate() {
        this->height = levels.size() - 1;
        for(size_t i = 0; i < levels.size(); ++i) {
            Level &level = levels[i];
            assert(level.height == i);
            evaluate_level(level);
        }
    };

    /**
     * @brief Resets the instrument to its empty state.
     */
    void clean() { *this = WTreeMemoryInstrument(); };

    /**
     * @brief Builds the memory register JSON body (without the wrapper keys).
     * @return A concatenated std::string with all metrics for the tree, and
     * reduced metrics for the levels.
     */
    std::string toJsonBody() const {
        using std::string, std::format;
        string oline;
        const size_t suggested_size = 256 + (256 * levels.size());
        oline.reserve(suggested_size);
        oline += format("\"keys\": {:10},", this->keys);
        oline += format("\"height\": {:3},\n", this->height);
        oline += format("\"nodes\": {:6},", this->nodes());
        oline += format("\"leaves\": {:4},", this->leaf_nodes);
        oline += format("\"internals\": {:4},", this->internal_nodes);
        oline += format("\"unused_keycells\": {:6},", this->unused_keycells);
        oline += format("\"unused_ptrcells\": {:6},\n", this->unused_pointers);
        oline += format("\"total_bytes\": {:10},", this->bytes_used());
        oline += format("\"total_overhead\": {:10},", this->total_overhead());
        oline += format("\"overhead\": {:f},", this->overhead());
        oline += format("\"fullness\": {:f},", this->fullness());
        oline += format("\"occupancy\": {:f},\n", this->occupancy());

        oline += "\"levels\":[\n";
        const size_t last_level = levels.size() - 1;
        for(size_t i = 0; i < levels.size(); ++i) {
            // Reduced some metrics for the levels.
            const Level &level = levels[i];
            oline += "\t{";
            oline += format("\"height\": {:3},", level.height);
            oline += format("\"keys\": {:10},", level.keys);
            oline += format("\"leaves\": {:4},", level.leaf_nodes);
            oline += format("\"internals\": {:4},\n", level.internal_nodes);
            oline +=
                format("\"unused_keycells\": {:6},", level.unused_keycells);
            oline +=
                format("\"unused_ptrcells\": {:6},", level.unused_pointers);
            oline +=
                format("\"total_overhead\": {:10},", level.total_overhead());
            oline += format("\"overhead\": {:f},\n", level.overhead());
            oline += format("\"fullness\": {:f},", level.fullness());
            oline += format("\"occupancy\": {:f}", level.occupancy());
            oline += (i < last_level) ? "},\n" : "}\n";
        }
        oline += "]";
        return oline;
    };

  private:
    void evaluate_level(Level &level) {
        this->keys += level.keys;
        this->leaf_nodes += level.leaf_nodes;
        this->internal_nodes += level.internal_nodes;
        this->unused_keycells += level.unused_keycells;
        this->unused_pointers += level.unused_pointers;
    }
};

/**
 * @brief Computes the extended statistics of any WTree container.
 * @details Complements the tree's collect_stats() with a per-level
 * breakdown: the aggregate fields are copied verbatim from the tree's own
 * collect_stats(), and levels is filled by an extra traversal.
 * @tparam Params A @ref WTree "WTree<Params>" instantiation, where Params is
 *         @ref WTreeLib::WTreeSetParams "WTreeSetParams" or @ref
 *         WTreeLib::WTreeMapParams "WTreeMapParams".
 */
template <typename Params> struct WTreeProfiler {
    using wtree_type = WTree<Params>;
    using node_type = wtree_type::node_type;
    using container_type = WTreeContainer<wtree_type>;
    using instrument_type = WTreeMemoryInstrument<Params>;
    using level_type = instrument_type::Level;
    using it_type = container_type::iterator;
    using const_it_type = container_type::const_iterator;

    /**
     * @brief Creates an instrument to computes the tree statistics starting
     * from its root.
     * @details The aggregate is trusted to the tree's own collect_stats();
     * the added value over that inner call is the per-level breakdown kept
     * in instrument.levels.
     * @param container The container (set, map, multiset, multimap) whose
     *        tree is profiled.
     */
    static instrument_type collect_statistics(const container_type &container) {
        instrument_type reg;
        collect_statistics(container, reg);
        return reg;
    }

    /**
     * @brief Computes the tree statistics of a container and stores them in
     * the instrument.
     * @details The aggregate is trusted to the tree's own collect_stats();
     * the added value over that inner call is the per-level breakdown kept
     * in instrument.levels.
     * @param container The container (set, map, multiset, multimap) whose
     *        tree is profiled.
     * @param reg The register to fill; it is reset first.
     */
    static void collect_statistics(const container_type &container,
                                   instrument_type &reg) {
        reg.clean();
        const wtree_type &tree = *container.tree();
        static_cast<instrument_type::stats_type &>(reg) = tree.collect_stats();
        if(tree.croot() != nullptr) {
            collect_from_node(reg, tree.croot(), 0);
        }
        reg.evaluate();
    }

    /**
     * @brief Creates an instrument to computes the tree statistics starting
     * from its root.
     * @details The aggregate is trusted to the tree's own collect_stats();
     * the added value over that inner call is the per-level breakdown kept
     * in instrument.levels.
     * @param it And iterator from some WTree container (set, map, multiset,
     * multimap) whose tree is profiled.
     */
    static instrument_type collect_statistics(const const_it_type &it) {
        instrument_type reg;
        collect_statistics(it.node, reg);
        return reg;
    }

    /**
     * @brief Creates an instrument to computes the tree statistics starting
     * from its root.
     * @details The aggregate is trusted to the tree's own collect_stats();
     * the added value over that inner call is the per-level breakdown kept
     * in instrument.levels.
     * @param it And iterator from some WTree container (set, map, multiset,
     * multimap) whose tree is profiled.
     * @param reg The register to fill; it is reset first.
     */
    static void collect_statistics(const const_it_type &it,
                                   instrument_type &reg) {
        reg.clean();
        collect_statistics(it.node, reg);
        reg.evaluate();
    }

    /**
     * @brief Computes the statistics from an specified root node and stores
     * them in the instrument for the level.
     * @param reg The register to fill.
     * @param node Starting point of the collect navigation.
     * @param depth Current node depth to register.
     */
    static void collect_from_node(instrument_type &reg, const node_type *node,
                                  ulong depth = 0) {
        assert(node != nullptr);
        const uint k_value = node_type::kTargetK;

        level_type &level = *reg.ensure_level_exists(depth);
        level.keys += node->size();
        level.unused_keycells += node->capacity() - node->size();

        if(node->is_internal()) {
            ++level.internal_nodes;
            for(typename wtree_type::field_type i = 0; i < k_value - 1; ++i) {
                if(node->child(i)) {
                    collect_from_node(reg, node->child(i), depth + 1);
                } else {
                    ++level.unused_pointers;
                }
            }
        } else {
            ++level.leaf_nodes;
        }
    }
};

} // namespace WTreeLib

#endif // _WTREE_PROFILING_H_