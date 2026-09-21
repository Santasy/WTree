# WTree test targets
# Invoked as add_subdirectory() from the root CMakeLists.txt
#
# CTest labels:
#   unit_test   — every release test
#   debug_test  — every debug test
#   core        — fast, fundamental suites (always run)
#   ANALYSIS_TEST — slow/volume or std-differential suites (opt-in via
#                   WTREE_BUILD_ANALYSIS_TESTS)
#
# Core suites (label "core", run by `run_core_tests`):
#   1. insert_rules_test       — minimal int-key insertion rules
#   2. erase_rules_test        — minimal int-key erase rules
#   3. locator_test            — WTreeLocator search/find
#   4. unique_set_test         — deep unique-key set interface suite
#   5. unique_map_test         — unique-key map delta suite (set-deltas only)
#   6. container_semantics_test — copy/move/swap + balance policies (WTree+STD)
#   7. node_size_test          — kTargetK vs node bytes vs field_type
#   8. manager_test            — manager-layer relocations (movers + shifts)
#
# Analysis suites (label ANALYSIS_TEST, gated by WTREE_BUILD_ANALYSIS_TESTS):
#   9. set_stress_test         — massive inserts, multi byte-size keys
#  10. map_stress_test         — map volume at scale
#  11. impl_coverage_test      — std-parity vs std::set/std::map

set(WTREE_TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/test)

# Option to skip release test builds (debug-only is enough for assertions)
option(WTREE_DEBUG_ONLY_TESTS "Build only debug test targets" ON)

# Option to build the ANALYSIS_TEST suites (stress + std-differential).
# Off by default: the analysis suites are a final-resource debug, not the
# steady-state core run.
option(WTREE_BUILD_ANALYSIS_TESTS "Build and register the ANALYSIS_TEST suites" OFF)

# Helper function to create test executables
function(create_wtree_test TEST_NAME SOURCE_DIR)
    set(_extra_labels ${ARGN})

    # --- Release-like executable (optional) ---
    if(NOT WTREE_DEBUG_ONLY_TESTS)
        add_executable(${TEST_NAME}
            ${SOURCE_DIR}/${TEST_NAME}.cpp
        )
        target_link_libraries(${TEST_NAME} PRIVATE WTree::Container)
        set_target_properties(${TEST_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${WTREE_TEST_OUTPUT_DIR}
            OUTPUT_NAME ${TEST_NAME}
        )

        add_test(
            NAME ${TEST_NAME}_test
            COMMAND ${TEST_NAME}
            WORKING_DIRECTORY ${WTREE_TEST_OUTPUT_DIR}
        )
        set_tests_properties(${TEST_NAME}_test PROPERTIES
            TIMEOUT ${WTREE_TEST_TIMEOUT}
            LABELS "unit_test;${_extra_labels}"
        )
    endif()

    # --- Debug executable ---
    add_executable(${TEST_NAME}_debug
        ${SOURCE_DIR}/${TEST_NAME}.cpp
    )
    target_link_libraries(${TEST_NAME}_debug PRIVATE WTree::Container)

    target_compile_definitions(${TEST_NAME}_debug PRIVATE DEBUG DBGVERBOSE)
    target_compile_options(${TEST_NAME}_debug PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${WTREE_DBG_TARGET_FLAGS_GCC}>
        $<$<CXX_COMPILER_ID:MSVC>:${WTREE_DBG_TARGET_FLAGS_MSVC}>
    )

    set_target_properties(${TEST_NAME}_debug PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${WTREE_TEST_OUTPUT_DIR}
        OUTPUT_NAME ${TEST_NAME}.dbg
    )

    add_test(
        NAME ${TEST_NAME}_debug_test
        COMMAND $<TARGET_FILE:${TEST_NAME}_debug>
        WORKING_DIRECTORY ${WTREE_TEST_OUTPUT_DIR}
    )
    set_tests_properties(${TEST_NAME}_debug_test PROPERTIES
        TIMEOUT ${WTREE_DEBUG_TEST_TIMEOUT}
        LABELS "debug_test;${_extra_labels}"
    )
endfunction()

# === Register core suites (order mirrors the intended execution order) ===

# 1-2: Internal tree rules (int keys)
create_wtree_test(insert_rules_test         insert_rules          core)
create_wtree_test(erase_rules_test          erase_rules           core)

# 3: Locator / search
create_wtree_test(locator_test              locator               core)

# 4-5: Unique-key containers — deep set suite, then map delta suite
create_wtree_test(unique_set_test           unique_containers     core)
create_wtree_test(unique_map_test           unique_containers     core)

# 6: Container semantics (copy/move/swap — WTree vs STD) + balance policies
create_wtree_test(container_semantics_test  container_semantics   core)

# 7: Node-size parametrization
create_wtree_test(node_size_test            node_size             core)

# 8: Manager-layer relocations (movers / in-node shifts)
create_wtree_test(manager_test              manager               core)

# === Register analysis suites (opt-in) ===

if(WTREE_BUILD_ANALYSIS_TESTS)
    create_wtree_test(set_stress_test       stress                ANALYSIS_TEST)
    create_wtree_test(map_stress_test       stress                ANALYSIS_TEST)
    create_wtree_test(impl_coverage_test    impl_coverage         ANALYSIS_TEST)
endif()

# === Convenience targets ===

set(ALL_DEBUG_TARGETS
    insert_rules_test_debug
    erase_rules_test_debug
    locator_test_debug
    unique_set_test_debug
    unique_map_test_debug
    container_semantics_test_debug
    node_size_test_debug
    manager_test_debug
)

if(WTREE_BUILD_ANALYSIS_TESTS)
    list(APPEND ALL_DEBUG_TARGETS
        set_stress_test_debug
        map_stress_test_debug
        impl_coverage_test_debug
    )
endif()

# Custom target: run every registered test via CTest
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel $<IF:$<BOOL:${CMAKE_BUILD_PARALLEL_LEVEL}>,${CMAKE_BUILD_PARALLEL_LEVEL},4>
    DEPENDS ${ALL_DEBUG_TARGETS}
    COMMENT "Running all WTree tests"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Custom target: run the fast, fundamental core suites (label "core")
add_custom_target(run_core_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L core --parallel $<IF:$<BOOL:${CMAKE_BUILD_PARALLEL_LEVEL}>,${CMAKE_BUILD_PARALLEL_LEVEL},4>
    DEPENDS ${ALL_DEBUG_TARGETS}
    COMMENT "Running core WTree tests (rules, locator, unique set/map, container semantics, node size, manager relocations)"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    VERBATIM
)

# Custom target: run the opt-in analysis suites (label ANALYSIS_TEST)
if(WTREE_BUILD_ANALYSIS_TESTS)
    add_custom_target(run_analysis_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L ANALYSIS_TEST --parallel $<IF:$<BOOL:${CMAKE_BUILD_PARALLEL_LEVEL}>,${CMAKE_BUILD_PARALLEL_LEVEL},4>
        DEPENDS ${ALL_DEBUG_TARGETS}
        COMMENT "Running analysis WTree tests (stress + std-parity vs std)"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
    )
endif()

# === Valgrind support ===
find_program(VALGRIND_PROGRAM valgrind)
if(VALGRIND_PROGRAM)
    add_custom_target(valgrind_full
        COMMAND ${VALGRIND_PROGRAM}
            --leak-check=full
            --show-reachable=yes
            --show-possibly-lost=yes
            --undef-value-errors=yes
            --partial-loads-ok=yes
            --log-file=${CMAKE_BINARY_DIR}/valgrind_full.log
            $<TARGET_FILE:insert_rules_test_debug>
        DEPENDS insert_rules_test_debug
        COMMENT "Running full Valgrind analysis"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )

    add_custom_target(valgrind_quick
        COMMAND ${VALGRIND_PROGRAM}
            --leak-check=no
            --show-reachable=no
            --show-possibly-lost=no
            --undef-value-errors=no
            --partial-loads-ok=no
            --log-file=${CMAKE_BINARY_DIR}/valgrind_quick.log
            $<TARGET_FILE:insert_rules_test_debug>
        DEPENDS insert_rules_test_debug
        COMMENT "Running quick Valgrind check"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# === Coverage support (GCC/Clang with gcov + lcov) ===
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    foreach(_target IN LISTS ALL_DEBUG_TARGETS)
        target_compile_options(${_target} PRIVATE --coverage)
        target_link_options(${_target} PRIVATE --coverage)
    endforeach()

    find_program(LCOV_PROGRAM lcov)
    find_program(GENHTML_PROGRAM genhtml)

    if(LCOV_PROGRAM)
        # Reset counters before a fresh run
        add_custom_target(coverage_zero
            COMMAND ${LCOV_PROGRAM} --zerocounters --directory .
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Resetting coverage counters"
        )

        # Capture coverage data and filter out system headers
        add_custom_target(coverage
            COMMAND ${LCOV_PROGRAM} --capture --directory . --output-file coverage.info
            COMMAND ${LCOV_PROGRAM} --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND ${LCOV_PROGRAM} --list coverage.info
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Capturing coverage data"
        )

        # Generate browsable HTML report
        if(GENHTML_PROGRAM)
            add_custom_target(coverage_html
                COMMAND ${GENHTML_PROGRAM} coverage.info
                    --output-directory coverage_html
                    --title "WTree Coverage"
                    --legend
                DEPENDS coverage
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating HTML coverage report in coverage_html/"
            )
        endif()
    endif()
endif()