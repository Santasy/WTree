# WTree example targets
# Invoked as add_subdirectory() from the root CMakeLists.txt,
# or can be built standalone.

set(WTREE_EXAMPLE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/examples)

# If building standalone, create the interface library
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(WTreeExamples LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    add_library(wtree_container INTERFACE)
    add_library(WTree::Container ALIAS wtree_container)
    target_include_directories(wtree_container INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )
    target_compile_features(wtree_container INTERFACE cxx_std_20)

    # Provide defaults when built standalone
    if(NOT DEFINED WTREE_DBG_TARGET_FLAGS_GCC)
        set(WTREE_DBG_TARGET_FLAGS_GCC -g -ggdb -O0)
    endif()
    if(NOT DEFINED WTREE_DBG_TARGET_FLAGS_MSVC)
        set(WTREE_DBG_TARGET_FLAGS_MSVC /Od /Zi)
    endif()
endif()

# Helper function to create an example executable + its debug variant
function(create_wtree_example EXAMPLE_NAME)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp)
    target_link_libraries(${EXAMPLE_NAME} PRIVATE WTree::Container)
    target_compile_options(${EXAMPLE_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-unused-but-set-variable>
    )
    set_target_properties(${EXAMPLE_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${WTREE_EXAMPLE_OUTPUT_DIR}
        OUTPUT_NAME ${EXAMPLE_NAME}
    )

    add_executable(${EXAMPLE_NAME}_debug ${EXAMPLE_NAME}.cpp)
    target_link_libraries(${EXAMPLE_NAME}_debug PRIVATE WTree::Container)
    target_compile_options(${EXAMPLE_NAME}_debug PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:${WTREE_DBG_TARGET_FLAGS_GCC} -Wno-unused-but-set-variable>
        $<$<CXX_COMPILER_ID:MSVC>:${WTREE_DBG_TARGET_FLAGS_MSVC}>
    )
    set_target_properties(${EXAMPLE_NAME}_debug PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${WTREE_EXAMPLE_OUTPUT_DIR}
        OUTPUT_NAME ${EXAMPLE_NAME}.dbg
    )
endfunction()

# === Register examples ===
create_wtree_example(single_operations)
create_wtree_example(map_example)
create_wtree_example(complex_map_example)
create_wtree_example(encapsulated_map_example)
create_wtree_example(wtree_print_example)
create_wtree_example(comprehensive_map_tests)
create_wtree_example(string_map)
