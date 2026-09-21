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

#ifndef _WTREE_TEST_MANAGER_H_
#define _WTREE_TEST_MANAGER_H_

/**
 * @file manager_test.hpp
 * @brief Manager-layer relocation suite (WTreeNodeManager directly).
 *
 * Exercises the two mover families straight on the manager, with a
 * construction/destruction-counted key type (Track) plus a test-owned oracle
 * (per-slot expected id/liveness) that is independent of the implementation:
 *
 *   - move_values_to_node: in-node shifts only. Left shifts (dest < first)
 *     run forward; right shifts (dest > first) run backward. Every
 *     destination slot is expected non-live: inside the source range the
 *     mover destroys it in-pass (which is exactly why the traversal
 *     direction matters), outside it the caller has killed it or it was
 *     never constructed.
 *   - move_values_to_blank_node: disjoint relocations only. The destination
 *     must be non-live; the mover never destroys at the destination.
 *
 * Groups:
 *   1. move_values_to_node left shifts (forward)
 *   2. move_values_to_node right shifts (backward), incl. the
 *      grow_leaf_and_shift_right in-place case with a never-constructed tail
 *   3. move_values_to_blank_node — dest after source (node-pair and
 *      pointer-range) and dest before source (grown-sibling-tail shape)
 *   4. Non-blank destinations:
 *      a. dead-but-constructed slots (the empty prefix a right-shift leaves
 *         behind, which a slide then reuses) — blank mover must not destroy
 *         there;
 *      b. LIVE destination -> negative contract test pinning that the mover
 *         leaks the old occupants (callers must pre-destroy).
 *
 * The oracle + Track::alive counters catch, without any library
 * instrumentation: double-destroys, leaks, and a shift that overwrites a
 * not-yet-moved source.
 */

#include "../utils/declaration.hpp"
#include "../utils/testing.hpp"
#include "../utils/helpers.hpp"

#include <memory>

namespace ManagerTest {

using namespace WTreeLib;
using namespace TestPrinting;

// ============================================================================
// Fixture: direct manager, no container.
// ============================================================================

// Track grew from a plain key to a full instrumented value (id + owned flag +
// heap payload, 16 bytes), so the default 128-byte node budget would shrink
// kTargetK below the 12-slot floor the scratch layout exercises.  A 256-byte
// budget keeps kTargetK at 15, matching the pre-instrumentation layout.
constexpr int kManagerNodeBytes = 256;

using TestParams =
    WTreeSetParams<Track, std::less<Track>, std::allocator<Track>,
                   kManagerNodeBytes>;
using Mgr = WTreeNodeManager<TestParams>;
using node_type = Mgr::node_type;
using field_type = Mgr::field_type;

Mgr manager(typename Mgr::internal_allocator_type{});

// ScratchNode + oracle helpers (Track-based liveness checks) live in
// ../utils/helpers.hpp, templated on the manager type.

// ============================================================================
// Group 1: move_values_to_node — left shifts (forward pass)
// ============================================================================

bool group1_left_shifts(TestResults &results) {
    TestPrinting::job_title("move_values_to_node: left shifts (forward)");

    // 1a. Erase-leaf / erase-right-branch shape: kill the erased slot, then
    //     shift [p+1, size) -> [p, size-1).
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 0; i < 9; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(9);
        sn.kill(3);
        apply_shift_oracle(sn, 4, 9, 3);
        manager.move_values_to_node(sn.node->fields.values + 4,
                                    sn.node->fields.values + 9,
                                    sn.node->fields.values + 3);
        sn.set_size(8);
        if(!verify_node(sn, "L1a erase-leaf shape [4,9)->[3,8)", results))
            return false;
        if(!check_alive({&sn}, results, "L1a alive balanced"))
            return false;
    }

    // 1b. internal_move_smallest_upward root shape: kill slot 0, shift
    //     [1, size) -> [0, size-1).
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 0; i < 11; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(11);
        sn.kill(0);
        apply_shift_oracle(sn, 1, 11, 0);
        manager.move_values_to_node(sn.node->fields.values + 1,
                                    sn.node->fields.values + 11,
                                    sn.node->fields.values);
        sn.set_size(10);
        if(!verify_node(sn, "L1b smallest-upward shape [1,11)->[0,10)",
                        results))
            return false;
        if(!check_alive({&sn}, results, "L1b alive balanced"))
            return false;
    }

    // 1c. Multi-slot overlap: dest [2,8) starts below source [6,12) but
    //     reaches into it; dest slots [2,6) are killed first, [6,8) are
    //     sources destroyed in-pass. Forward pass must consume each source
    //     before its slot is re-constructed.
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 0; i < 12; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(12);
        for(int i = 2; i < 6; ++i)
            sn.kill(i);
        apply_shift_oracle(sn, 6, 12, 2);
        manager.move_values_to_node(sn.node->fields.values + 6,
                                    sn.node->fields.values + 12,
                                    sn.node->fields.values + 2);
        sn.set_size(8);
        if(!verify_node(sn, "L1c overlapping left shift [6,12)->[2,8)",
                        results))
            return false;
        if(!check_alive({&sn}, results, "L1c alive balanced"))
            return false;
    }

    // 1d. Disjoint left shift, dest wholly below source: [6,10)->[2,6).
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 6; i < 10; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(10);
        apply_shift_oracle(sn, 6, 10, 2);
        manager.move_values_to_node(sn.node->fields.values + 6,
                                    sn.node->fields.values + 10,
                                    sn.node->fields.values + 2);
        sn.set_size(4);
        if(!verify_node(sn, "L1d disjoint left shift [6,10)->[2,6)", results))
            return false;
        if(!check_alive({&sn}, results, "L1d alive balanced"))
            return false;
    }

    // 1e. No-op case dest == first.
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 0; i < 5; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(5);
        manager.move_values_to_node(sn.node->fields.values,
                                    sn.node->fields.values + 5,
                                    sn.node->fields.values);
        if(!verify_node(sn, "L1e no-op dest==first", results))
            return false;
        if(!check_alive({&sn}, results, "L1e alive balanced"))
            return false;
    }

    return true;
}

// ============================================================================
// Group 2: move_values_to_node — right shifts (backward pass)
// ============================================================================

bool group2_right_shifts(TestResults &results) {
    TestPrinting::job_title("move_values_to_node: right shifts (backward)");

    // 2a. Erase-left-branch shape (tree.tpp): kill the pivot at the top of
    //     the dest [clb+1, index+1), source [clb, index); the highest dest
    //     slot was the killed pivot, everything else overlaps.
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 0; i < 9; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(9);
        sn.kill(8); // the removed pivot
        apply_shift_oracle(sn, 3, 8, 4);
        manager.move_values_to_node(sn.node->fields.values + 3,
                                    sn.node->fields.values + 8,
                                    sn.node->fields.values + 4);
        sn.set_size(8);
        if(!verify_node(sn, "R2a erase-left shape [3,8)->[4,9)", results))
            return false;
        if(!check_alive({&sn}, results, "R2a alive balanced"))
            return false;
    }

    // 2b. internal_move_greatest_upward full-node shape: kill kTargetK-1
    //     analog, source [pos+1, n) -> dest [pos+2, n+1).
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 0; i < 8; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(8);
        sn.kill(7);
        apply_shift_oracle(sn, 3, 7, 4);
        manager.move_values_to_node(sn.node->fields.values + 3,
                                    sn.node->fields.values + 7,
                                    sn.node->fields.values + 4);
        sn.set_size(7);
        if(!verify_node(sn, "R2b greatest-upward shape [3,7)->[4,8)", results))
            return false;
        if(!check_alive({&sn}, results, "R2b alive balanced"))
            return false;
    }

    // 2c. greatest-upward not-full shape: dest tail lands on never-constructed
    //     slots, no pre-destroy needed.
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 0; i < 6; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(6);
        apply_shift_oracle(sn, 3, 6, 4); // dest [4,7), slot 6 blank
        manager.move_values_to_node(sn.node->fields.values + 3,
                                    sn.node->fields.values + 6,
                                    sn.node->fields.values + 4);
        sn.set_size(7);
        if(!verify_node(sn, "R2c greatest-upward not-full [3,6)->[4,7)",
                        results))
            return false;
        if(!check_alive({&sn}, results, "R2c alive balanced"))
            return false;
    }

    // 2d. Overlapping right shift: [2,8)->[5,11); dest tail slots 8..10 are
    //     never-constructed, dest head slots 5..7 are sources destroyed
    //     in-pass (backward).
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 2; i < 8; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(8);
        apply_shift_oracle(sn, 2, 8, 5);
        manager.move_values_to_node(sn.node->fields.values + 2,
                                    sn.node->fields.values + 8,
                                    sn.node->fields.values + 5);
        sn.set_size(11);
        if(!verify_node(sn, "R2d overlapping right shift [2,8)->[5,11)",
                        results))
            return false;
        if(!check_alive({&sn}, results, "R2d alive balanced"))
            return false;
    }

    // 2e. grow_leaf_and_shift_right in place: [0,old)->[new-old,new). The
    //     destination tail [old,new) is never-constructed and the vacated
    //     prefix dead — this is the shape a slide_to_right then reuses.
    {
        ScratchNode<Mgr> sn(manager);
        for(int i = 0; i < 5; ++i)
            sn.plant(i, 10 + i);
        sn.set_size(5);
        apply_shift_oracle(sn, 0, 5, 3);
        node_type *ret = manager.grow_leaf_and_shift_right(sn.node, 8);
        if(ret != sn.node) {
            results.fail("R2e grow_leaf_and_shift_right", "node reallocated");
            return false;
        }
        results.pass("R2e grow_leaf_and_shift_right in-place (no realloc)");
        sn.set_size(8);
        if(!verify_node(sn, "R2e grow_leaf_and_shift_right [0,5)->[3,8)",
                        results))
            return false;
        if(!check_alive({&sn}, results, "R2e alive balanced"))
            return false;
        // The vacated prefix [0,3) must be non-live.
        for(int i = 0; i < 3; ++i) {
            if(sn.id_at[i] != -1) {
                results.fail("R2e vacated prefix dead",
                             "slot " + std::to_string(i));
                return false;
            }
        }
        results.pass("R2e vacated prefix dead");
    }

    return true;
}

// ============================================================================
// Group 3: move_values_to_blank_node — disjoint relocations
// ============================================================================

bool group3_blank_moves(TestResults &results) {
    TestPrinting::job_title("move_values_to_blank_node: disjoint relocations");

    // 3a. Node-pair, dest after source (make_internal/reallocate pattern):
    //     sources end up moved-from, never destroyed by the mover.
    {
        ScratchNode<Mgr> n1(manager), n2(manager);
        for(int i = 0; i < 4; ++i)
            n1.plant(i, 100 + i);
        n1.set_size(4);
        apply_blank_move_oracle(n1, n2, 0, 4, 6);
        manager.move_values_to_blank_node(n1.node, n2.node, 4, 6);
        n2.set_size(10);
        if(!verify_node(n2, "B3a node-pair dest ids", results))
            return false;
        if(Track::g_alive != n2.count_live()) {
            results.fail("B3a alive (node-pair sources moved-from, not dead)",
                         std::to_string(Track::g_alive));
            return false;
        }
        results.pass("B3a alive (node-pair sources moved-from, not dead)");
        // Simulate the caller destroying the source node: the moved-from
        // values are still destructible.
        for(int i = 0; i < 4; ++i)
            n1.node->destroy_value(i);
        if(!check_alive({&n1, &n2}, results, "B3a alive after source teardown"))
            return false;
    }

    // 3b. Pointer-range, dest after source (make_leaf_from_internal /
    //     pointer blank shape): sources are destroyed by the mover.
    {
        ScratchNode<Mgr> n1(manager), n2(manager);
        for(int i = 0; i < 4; ++i)
            n1.plant(i, 100 + i);
        n1.set_size(4);
        apply_blank_move_oracle(n1, n2, 0, 4, 2);
        manager.move_values_to_blank_node(n1.node->fields.values,
                                          n1.node->fields.values + 4,
                                          n2.node->fields.values + 2);
        n2.set_size(6);
        if(!verify_node(n2, "B3b pointer dest-after-source ids", results))
            return false;
        if(!check_alive({&n1, &n2}, results, "B3b alive (sources destroyed)"))
            return false;
    }

    // 3c. Pointer-range, dest before source (slide_to_left appended-tail
    //     shape): the sibling keeps its live prefix [0,3), dest lands at
    //     offset 3.
    {
        ScratchNode<Mgr> n1(manager), n2(manager);
        for(int i = 0; i < 4; ++i)
            n1.plant(i, 100 + i);
        n1.set_size(4);
        for(int i = 0; i < 3; ++i)
            n2.plant(i, 200 + i);
        n2.set_size(3);
        apply_blank_move_oracle(n1, n2, 0, 4, 3);
        manager.move_values_to_blank_node(n1.node->fields.values,
                                          n1.node->fields.values + 4,
                                          n2.node->fields.values + 3);
        n2.set_size(7);
        if(!verify_node(n1, "B3c source node fully destroyed", results))
            return false;
        if(!verify_node(n2, "B3c sibling dest-before-source ids", results))
            return false;
        if(!check_alive({&n1, &n2}, results, "B3c alive balanced"))
            return false;
    }

    // 3d. Dead-but-constructed destination (slide_to_right vacated-prefix
    //     pattern): prepopulate the dest region, kill it first, then blank
    //     move; the mover must never destroy at the destination.
    {
        ScratchNode<Mgr> n1(manager), n2(manager);
        for(int i = 0; i < 4; ++i)
            n1.plant(i, 100 + i);
        n1.set_size(4);
        const int dst = 3;
        for(int i = 0; i < 4; ++i)
            n2.plant(dst + i, 500 + i); // live occupants
        n2.set_size(10);
        for(int i = 0; i < 4; ++i)
            n2.kill(dst + i); // caller pre-destroy
        apply_blank_move_oracle(n1, n2, 0, 4, dst);
        manager.move_values_to_blank_node(n1.node->fields.values,
                                          n1.node->fields.values + 4,
                                          n2.node->fields.values + dst);
        n2.set_size(7);
        if(!verify_node(n1, "B3d source destroyed", results))
            return false;
        if(!verify_node(n2, "B3d dead-but-constructed dest ids", results))
            return false;
        if(!check_alive({&n1, &n2}, results,
                        "B3d no destroy-at-dest (alive balanced)"))
            return false;
    }

    return true;
}

// ============================================================================
// Group 4: negative contract test — LIVE destination leaks
// ============================================================================

bool group4_live_dest_contract(TestResults &results) {
    TestPrinting::job_title(
        "negative contract: live destination must leak (undocumented use)");

    {
        ScratchNode<Mgr> n1(manager), n2(manager);
        const int count = 4;
        std::vector<int *> occupant_payloads; // leaked buffers, freed by hand
        for(int i = 0; i < count; ++i)
            n1.plant(i, 100 + i);
        n1.set_size(count);
        const int dst = 2;
        for(int i = 0; i < count; ++i) {
            n2.plant(dst + i, 500 + i); // LIVE occupants — contract violation
            occupant_payloads.push_back(
                n2.node->value((field_type)(dst + i)).data);
        }
        n2.set_size(6);

        const int before = Track::g_alive; // 4 sources + 4 occupants
        manager.move_values_to_blank_node(n1.node->fields.values,
                                          n1.node->fields.values + count,
                                          n2.node->fields.values + dst);

        // The mover constructed over the live occupants; sources destroyed.
        // Old occupants were never destroyed -> they leak.
        if(Track::g_alive != before) {
            results.fail("N4 leak observed", "alive=" + std::to_string(before));
            return false;
        }
        results.pass("N4 leak observed (old occupants never destroyed)");

        // Content is still the moved values (placement-new over the dead).
        for(int i = 0; i < count; ++i) {
            const Track &v = n2.node->value((field_type)(dst + i));
            if(v.id != 100 + i) {
                results.fail("N4 dest ids", "slot " + std::to_string(dst + i) +
                                                " got " + std::to_string(v.id));
                return false;
            }
        }
        results.pass("N4 dest ids correct");
        for(int i = 0; i < count; ++i)
            n2.id_at[dst + i] = 100 + i;

        // Teardown: destroy the moved-in values and the (dead) sources; the
        // leaked occupants remain counted but are unreachable. Production
        // code would leak real memory here — free the occupant payloads by
        // hand (no Track dtor, so the counter/destruction ledgers stay as
        // documented above).
        destroy_slots_for(n2, dst, count);
        for(int *p : occupant_payloads)
            delete p;
        const int leaked = before - count; // the old occupants
        if(Track::g_alive != leaked) {
            results.fail("N4 leaked occupant count",
                         std::to_string(Track::g_alive) + " vs " +
                             std::to_string(leaked));
            return false;
        }
        results.pass("N4 leaked occupants unreachable but counted");
        // Reset (documented: a real allocator would have leaked these).
        Track::g_alive = 0;
    }

    return true;
}

} // namespace ManagerTest

#endif