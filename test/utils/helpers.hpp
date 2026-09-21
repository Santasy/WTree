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

#ifndef _WTREE_TEST_HELPERS_H_
#define _WTREE_TEST_HELPERS_H_

#include "testing.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

template <typename Map>
bool maps_are_equal(const Map &a, const Map &b, const std::string &context) {
    if(a.size() != b.size()) {
        std::cerr << context << ": Size mismatch: " << a.size() << " vs "
                  << b.size() << "\n";
        return false;
    }

    auto it_a = a.begin();
    auto it_b = b.begin();
    while(it_a != a.end()) {
        if(it_a->first != it_b->first) {
            std::cerr << context << ": Key mismatch at position\n";
            return false;
        }
        if(it_a->second != it_b->second) {
            std::cerr << context << ": Value mismatch for key " << it_a->first
                      << "\n";
            return false;
        }
        ++it_a;
        ++it_b;
    }
    return true;
}

template <typename Set>
bool sets_are_equal(const Set &a, const Set &b, const std::string &context) {
    if(a.size() != b.size()) {
        std::cerr << context << ": Size mismatch: " << a.size() << " vs "
                  << b.size() << "\n";
        return false;
    }

    auto it_a = a.begin();
    auto it_b = b.begin();
    while(it_a != a.end()) {
        if(*it_a != *it_b) {
            std::cerr << context << ": Value mismatch\n";
            return false;
        }
        ++it_a;
        ++it_b;
    }
    return true;
}

// ============================================================================
// Track: instrumented value used as the element/key type across test suites.
//
// Two ledgers in one:
//  - g_alive — net "owned" count (constructible-and-not-yet-destroyed):
//    construction of an owned object +1, destruction / move-overwrite -1;
//    moves transfer ownership without changing the count.  This is what the
//    manager-layer oracles (verify_node / check_alive) check.
//  - per-operation counters (constructions, destructions, copies, moves) and
//    a heap-buffer ledger (allocations, deallocations): the container-layer
//    lifecycle tests check balanced() (objects) and allocation_balanced()
//    (no leak / double-free), plus copies==0 for try_emplace-style paths.
//
// The heap buffer (data) doubles as a use-after-free detector: the value it
// points to stays valid while owned and is nulled on move/overwrite, so any
// stale access shows a poisoned value instead of silently replayed memory.
// ============================================================================

struct Track {
    inline static int g_alive = 0; // owned objects currently alive
    inline static int constructions = 0, destructions = 0, copies = 0,
                     moves = 0;
    inline static int allocations = 0, deallocations = 0;

    int id;
    bool owned; // owns the id/value (transferable by move)
    int *data;  // heap payload for use-after-free detection

    Track() : id(0), owned(true), data(new int(0)) {
        ++g_alive;
        ++constructions;
        ++allocations;
    }
    explicit Track(int v) : id(v), owned(true), data(new int(v)) {
        ++g_alive;
        ++constructions;
        ++allocations;
    }
    Track(const Track &o) : id(o.id), owned(o.owned),
                            data(o.owned ? new int(*o.data) : nullptr) {
        ++copies;
        if(owned) {
            ++g_alive;
            ++allocations;
        }
    }
    Track(Track &&o) noexcept : id(o.id), owned(o.owned), data(o.data) {
        ++moves;
        o.owned = false;
        o.data = nullptr;
    }
    Track &operator=(const Track &o) {
        if(this != &o) {
            ++copies;
            if(owned) {
                --g_alive;
                ++deallocations;
                delete data;
            }
            id = o.id;
            owned = o.owned;
            data = o.owned ? new int(*o.data) : nullptr;
            if(owned) {
                ++g_alive;
                ++allocations;
            }
        }
        return *this;
    }
    Track &operator=(Track &&o) noexcept {
        if(this != &o) {
            ++moves;
            if(owned) {
                --g_alive;
                ++deallocations;
                delete data;
            }
            id = o.id;
            owned = o.owned;
            data = o.data;
            o.owned = false;
            o.data = nullptr;
        }
        return *this;
    }
    ~Track() {
        ++destructions;
        if(owned) {
            --g_alive;
            ++deallocations;
            delete data;
        }
    }

    static void reset_counters() {
        constructions = destructions = copies = moves = 0;
        allocations = deallocations = 0;
    }
    static bool balanced() { return constructions == destructions; }
    static bool allocation_balanced() { return allocations == deallocations; }

    bool operator<(const Track &o) const { return id < o.id; }
    bool operator==(const Track &o) const { return id == o.id; }
};

// ============================================================================
// Scratch node + liveness oracle for direct manager-layer tests.
//
// A test-owned node built straight on a WTreeNodeManager with a per-slot
// expected-id vector (-1 = non-live) as an oracle. The value type is Track so
// the alive counter + ownership flag catch double-destroys, leaks, and
// overwrites of a not-yet-moved source.
//
// The manager is supplied per-node so the same helper works for any
// WTreeNodeManager<Params> instantiation.
// ============================================================================

// Scratch value capacity; kept well above the kTargetK bound so the debug
// index asserts (i < kTargetK) never trip while all our indices stay < K.
inline constexpr int kScratchCap = 24;

template <typename Manager> class ScratchNode {
  public:
    using node_type = typename Manager::node_type;
    using field_type = typename Manager::field_type;

    node_type *node;
    std::vector<int> id_at; // -1 = non-live (never-constructed or destroyed)
    field_type size;        // logical fields.size (set by the fixture)
    Manager &mgr;

    explicit ScratchNode(Manager &manager, field_type cap = kScratchCap)
        : node(manager.new_leaf_node(cap)), id_at(static_cast<size_t>(cap), -1),
          size(0), mgr(manager) {
        node->fields.size = 0;
    }
    ~ScratchNode() {
        destroy_all();
        mgr.delete_leaf_node(node);
    }

    void plant(int i, int id) {
        node->construct_value(i, Track(id));
        id_at[i] = id;
        set_size(static_cast<field_type>(count_live()));
    }
    void kill(int i) { // caller pre-destroy (shift / blank-move discipline)
        assert(id_at[i] != -1);
        node->destroy_value(i);
        id_at[i] = -1;
    }
    int count_live() const {
        int n = 0;
        for(int v : id_at)
            n += (v != -1) ? 1 : 0;
        return n;
    }
    void set_size(field_type s) {
        size = s;
        node->fields.size = s;
    }
    void destroy_all() {
        for(size_t i = 0; i < id_at.size(); ++i)
            if(id_at[i] != -1) {
                node->destroy_value(static_cast<int>(i));
                id_at[i] = -1;
            }
        node->fields.size = 0;
    }
};

// Verify one node against its oracle: every expected-live slot must hold a
// live, owned Track with the expected id.
template <typename Mgr>
bool verify_node(ScratchNode<Mgr> &sn, const char *tag,
                 TestPrinting::TestResults &results) {
    for(size_t i = 0; i < sn.id_at.size(); ++i) {
        if(sn.id_at[i] != -1) {
            const Track &v = sn.node->value(typename Mgr::field_type(i));
            if(v.id != sn.id_at[i] || !v.owned) {
                results.fail(std::string(tag) + " slot " + std::to_string(i),
                             std::string("expected id ") +
                                 std::to_string(sn.id_at[i]) + " got " +
                                 std::to_string(v.id) +
                                 " owned=" + (v.owned ? "1" : "0"));
                return false;
            }
        }
    }
    results.pass(tag);
    return true;
}

// Global consistency: the alive counter must equal the oracle-total liveness.
template <typename Mgr>
bool check_alive(std::initializer_list<const ScratchNode<Mgr> *> nodes,
                 TestPrinting::TestResults &results, const char *tag) {
    int expected = 0;
    for(const ScratchNode<Mgr> *n : nodes)
        expected += n->count_live();
    if(Track::g_alive != expected) {
        results.fail(tag, "alive=" + std::to_string(Track::g_alive) +
                              " oracle=" + std::to_string(expected));
        return false;
    }
    results.pass(tag);
    return true;
}

// Oracle update for a range move inside one node's buffer. Precondition: all
// destination slots outside the source range are already non-live.
template <typename Mgr>
void apply_shift_oracle(ScratchNode<Mgr> &sn, int a, int b, int c) {
    std::vector<int> snap(sn.id_at.begin() + a, sn.id_at.begin() + b);
    for(int i = a; i < b; ++i)
        sn.id_at[i] = -1; // mover destroys the sources
    for(size_t k = 0; k < snap.size(); ++k)
        sn.id_at[c + static_cast<int>(k)] = snap[k];
}

// Oracle update for a disjoint blank move (pointer-range version): sources
// are destroyed, dest receives the snapshot in order.
template <typename Mgr>
void apply_blank_move_oracle(ScratchNode<Mgr> &src, ScratchNode<Mgr> &dst,
                             int a, int b, int c) {
    std::vector<int> snap(src.id_at.begin() + a, src.id_at.begin() + b);
    for(int i = a; i < b; ++i)
        src.id_at[i] = -1;
    for(size_t k = 0; k < snap.size(); ++k)
        dst.id_at[c + static_cast<int>(k)] = snap[k];
}

// Destroy a caller-known live range and clear the oracle for it.
template <typename Mgr>
void destroy_slots_for(ScratchNode<Mgr> &sn, int from, int count) {
    for(int i = from; i < from + count; ++i)
        if(sn.id_at[i] != -1) {
            sn.node->destroy_value(i);
            sn.id_at[i] = -1;
        }
}

// =============================================================================
// Queue: A non-trivially copyable/movable class for testing
// =============================================================================

class Queue {
  public:
    std::vector<int> data;
    int id; // Unique identifier to track copies/moves

    static int copy_count;
    static int move_count;
    static int instance_count;

    static void reset_counters() {
        copy_count = 0;
        move_count = 0;
    }

    // Default constructor
    Queue() : id(++instance_count) {}

    // Constructor with initial data
    explicit Queue(std::initializer_list<int> init)
        : data(init), id(++instance_count) {}

    explicit Queue(const std::vector<int> &v) : data(v), id(++instance_count) {}

    // Copy constructor
    Queue(const Queue &other) : data(other.data), id(++instance_count) {
        ++copy_count;
    }

    // Move constructor
    Queue(Queue &&other) noexcept
        : data(std::move(other.data)), id(++instance_count) {
        ++move_count;
    }

    // Copy assignment
    Queue &operator=(const Queue &other) {
        if(this != &other) {
            data = other.data;
            ++copy_count;
        }
        return *this;
    }

    // Move assignment
    Queue &operator=(Queue &&other) noexcept {
        if(this != &other) {
            data = std::move(other.data);
            ++move_count;
        }
        return *this;
    }

    // Destructor
    ~Queue() = default;

    // Queue operations
    void push(int val) { data.push_back(val); }

    int pop() {
        if(data.empty())
            return -1;
        int val = data.front();
        data.erase(data.begin());
        return val;
    }

    bool empty() const { return data.empty(); }
    size_t size() const { return data.size(); }

    // Comparison operators
    bool operator==(const Queue &other) const { return data == other.data; }
    bool operator!=(const Queue &other) const { return !(*this == other); }
};

// Static member definitions (inline for header-only usage)
inline int Queue::copy_count = 0;
inline int Queue::move_count = 0;
inline int Queue::instance_count = 0;
#endif