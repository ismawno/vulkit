cmake_minimum_required(VERSION 3.16)
project(vulkit-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "Available build types" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

set(TOOLKIT_BUILD_TESTS OFF CACHE BOOL "Build toolkit tests" FORCE)
set(TOOLKIT_BUILD_PERFORMANCE OFF CACHE BOOL "Build toolkit performance tests" FORCE)
set(VULKIT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_BUILD_TYPE STREQUAL "Dist")
    option(VULKIT_BUILD_TESTS "Build the demos" OFF)
else()
    option(VULKIT_BUILD_TESTS "Build the demos" ON)
endif()

message(STATUS "VULKIT: Build tests: ${VULKIT_BUILD_TESTS}")

# Compiler flags, linker flags etcetera, those should all be handled when making available the toolkit library through the global CMAKE flag variables
add_subdirectory(vulkit)

# if (VULKIT_BUILD_TESTS)
#     add_subdirectory(tests)
# endif()