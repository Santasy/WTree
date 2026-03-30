cmake_minimum_required(VERSION 3.20)

project(WTreeContainer
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "High-performance STL-compatible container library that implements a WTree under the hood."
)

# === Project-wide settings ===
set(WTREE_CXX_STANDARD 20)
set(WTREE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(WTREE_HEADER_PREFIX include/wtree)
set(WTREE_TEST_TIMEOUT 30)
set(WTREE_DEBUG_TEST_TIMEOUT 60)

set(CMAKE_CXX_STANDARD ${WTREE_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# === Compiler flags ===
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(COMMON_CXX_FLAGS
        -Wall
        -Wno-unknown-pragmas
        -fmax-errors=10
        -Wno-array-bounds
        -Wno-unused-variable
    )

    set(RELEASE_CXX_FLAGS
        -O3
        -DNDEBUG
        -Wno-unknown-pragmas
    )

    set(DEBUG_CXX_FLAGS
        -g3
        -gdwarf-4
        -O0
        -fno-omit-frame-pointer
        -fno-inline
        -fno-inline-functions
        -DDEBUG
        -DDBGVERBOSE
        -Wno-unused-but-set-variable
        -Wno-unused-variable
    )

    # Flags for debug-variant targets in subdirectories
    set(WTREE_DBG_TARGET_FLAGS_GCC -g -ggdb -O0)
    set(WTREE_DBG_TARGET_FLAGS_MSVC /Od /Zi)

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(COMMON_CXX_FLAGS /W3)
    set(RELEASE_CXX_FLAGS /O2 /DNDEBUG)
    set(DEBUG_CXX_FLAGS /Od /DDEBUG /DDBGVERBOSE)
    set(WTREE_DBG_TARGET_FLAGS_GCC "")
    set(WTREE_DBG_TARGET_FLAGS_MSVC /Od /Zi)
endif()

# === Custom compilation options ===
option(WTREE_TARGET_NODE_BYTES "Target node bytes for WTree" OFF)
if(WTREE_TARGET_NODE_BYTES)
    add_compile_definitions(WTREE_TARGET_NODE_BYTES=${WTREE_TARGET_NODE_BYTES})
endif()

set(NVER "v000" CACHE STRING "Version identifier")
add_compile_definitions(NVER="${NVER}")

# === Interface library (header-only) ===
add_library(wtree_container INTERFACE)
add_library(WTree::Container ALIAS wtree_container)

set(WTREE_HEADERS
    ${WTREE_HEADER_PREFIX}/set.hpp
    ${WTREE_HEADER_PREFIX}/map.hpp
    # detail/
    ${WTREE_HEADER_PREFIX}/detail/index.hpp
    ${WTREE_HEADER_PREFIX}/detail/traits.hpp
    ${WTREE_HEADER_PREFIX}/detail/type_aliases.hpp
    ${WTREE_HEADER_PREFIX}/detail/node.hpp
    ${WTREE_HEADER_PREFIX}/detail/node.tpp
    ${WTREE_HEADER_PREFIX}/detail/iterator.tpp
    ${WTREE_HEADER_PREFIX}/detail/iterator.hpp
    ${WTREE_HEADER_PREFIX}/detail/node_manager.hpp
    ${WTREE_HEADER_PREFIX}/detail/node_manager.tpp
    ${WTREE_HEADER_PREFIX}/detail/locator.hpp
    ${WTREE_HEADER_PREFIX}/detail/locator.tpp
    ${WTREE_HEADER_PREFIX}/detail/tree.hpp
    ${WTREE_HEADER_PREFIX}/detail/tree.tpp
    #detail/containers
    ${WTREE_HEADER_PREFIX}/detail/containers.hpp
    ${WTREE_HEADER_PREFIX}/detail/unique_container.hpp
    ${WTREE_HEADER_PREFIX}/detail/multi_container.hpp
    # optional/
    ${WTREE_HEADER_PREFIX}/optional/print.hpp
    ${WTREE_HEADER_PREFIX}/optional/utils.hpp
    ${WTREE_HEADER_PREFIX}/optional/profile.hpp
)

target_sources(wtree_container INTERFACE
    $<BUILD_INTERFACE:${WTREE_HEADERS}>
)

target_include_directories(wtree_container
    INTERFACE
        $<BUILD_INTERFACE:${WTREE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(wtree_container
    INTERFACE
        cxx_std_${WTREE_CXX_STANDARD}
)

target_compile_options(wtree_container
    INTERFACE
        $<$<CONFIG:Release>:${COMMON_CXX_FLAGS};${RELEASE_CXX_FLAGS}>
        $<$<CONFIG:Debug>:${COMMON_CXX_FLAGS};${DEBUG_CXX_FLAGS}>
)

# === Subdirectories ===
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation configuration
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install the interface library
install(TARGETS wtree_container
    EXPORT WTreeContainerTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install headers with proper directory structure
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING 
        PATTERN "*.h" 
        PATTERN "*.hpp"
        PATTERN "*.tpp"
)

# Create and install package config files
set(CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/WTreeContainer)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/WTreeContainerConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/WTreeContainerConfig.cmake"
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/WTreeContainerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/WTreeContainerConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/WTreeContainerConfigVersion.cmake"
    DESTINATION ${CONFIG_INSTALL_DIR}
)

install(EXPORT WTreeContainerTargets
    FILE WTreeContainerTargets.cmake
    NAMESPACE WTree::
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "WTreeContainer")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "sebastian.pacheco@uach.cl")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)
