# WTree test targets
# Invoked as add_subdirectory() from the root CMakeLists.txt
#
# Execution order matters — tests are registered so that CTest runs them
# from the fastest / most fundamental to the slowest / most integrated:
#
#   1. insert_rules_test     — minimal int-key insertion rules
#   2. erase_rules_test      — minimal int-key erase rules
#   3. locator_test          — WTreeLocator search/find

set(WTREE_TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/test)

# Option to skip release test builds (debug-only is enough for assertions)
option(WTREE_DEBUG_ONLY_TESTS "Build only debug test targets" ON)

# Helper function to create test executables
function(create_wtree_test TEST_NAME SOURCE_DIR)
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
            LABELS "unit_test"
        )
    endif()

    # --- Debug executable ---
    add_executable(${TEST_NAME}_debug
        ${SOURCE_DIR}/${TEST_NAME}.cpp
    )
    target_link_libraries(${TEST_NAME}_debug PRIVATE WTree::Container)

    target_compile_definitions(${TEST_NAME}_debug PRIVATE DEBUG DBGVERBOSE)
    target_compile_options(${TEST_NAME}_debug PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:${WTREE_DBG_TARGET_FLAGS_GCC}>
        $<$<CXX_COMPILER_ID:MSVC>:${WTREE_DBG_TARGET_FLAGS_MSVC}>
    )

    set_target_properties(${TEST_NAME}_debug PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${WTREE_TEST_OUTPUT_DIR}
        OUTPUT_NAME ${TEST_NAME}.dbg
    )

    add_test(
        NAME ${TEST_NAME}_debug_test
        COMMAND ${TEST_NAME}_debug
        WORKING_DIRECTORY ${WTREE_TEST_OUTPUT_DIR}
    )
    set_tests_properties(${TEST_NAME}_debug_test PROPERTIES
        TIMEOUT ${WTREE_DEBUG_TEST_TIMEOUT}
        LABELS "debug_test"
    )
endfunction()

# === Register tests (order mirrors the intended execution order) ===

# 1-2: Internal tree rules (int keys)
create_wtree_test(insert_rules_test         insert_rules)
create_wtree_test(erase_rules_test          erase_rules)

# 3: Locator / search
create_wtree_test(locator_test              locator)

# Collect all debug targets for convenience targets
set(ALL_DEBUG_TARGETS
    insert_rules_test_debug
    erase_rules_test_debug
    locator_test_debug
)

# Custom target: run every test via CTest
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel ${CMAKE_BUILD_PARALLEL_LEVEL}
    DEPENDS ${ALL_DEBUG_TARGETS}
    COMMENT "Running all WTree tests"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Custom target: run tests showing only last line of output
add_custom_target(run_tests_tail
    COMMAND ${CMAKE_COMMAND} -E echo "Running insert rules test..."
    COMMAND $<TARGET_FILE:insert_rules_test_debug> | tail -n 1 || echo "Insert rules test failed"
    COMMAND ${CMAKE_COMMAND} -E echo "Running erase rules test..."
    COMMAND $<TARGET_FILE:erase_rules_test_debug> | tail -n 1 || echo "Erase rules test failed"
    COMMAND ${CMAKE_COMMAND} -E echo "Running locator test..."
    COMMAND $<TARGET_FILE:locator_test_debug> | tail -n 1 || echo "Locator test failed"
    DEPENDS insert_rules_test_debug erase_rules_test_debug locator_test_debug
    COMMENT "Running tests with tail output"
    VERBATIM
)

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
