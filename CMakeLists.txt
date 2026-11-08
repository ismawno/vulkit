cmake_minimum_required(VERSION 3.16...3.31)
project(vulkit-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BUILD_TYPE
    "Debug"
    CACHE STRING "")

set(VULKIT_BUILD_TESTS
    OFF
    CACHE BOOL "")

add_subdirectory(vulkit)
if(VULKIT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
