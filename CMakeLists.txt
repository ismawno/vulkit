cmake_minimum_required(VERSION 3.16)
project(vulkit-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(define_option varname default)
    if (NOT DEFINED ${varname})
        set(${varname} ${default} PARENT_SCOPE)
    endif()
endfunction()

define_option(CMAKE_BUILD_TYPE "Debug")
define_option(VULKIT_BUILD_TESTS OFF)

set(VULKIT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "VULKIT: Build tests: ${VULKIT_BUILD_TESTS}")

# Compiler flags, linker flags etcetera, those should all be handled when making available the toolkit library through the global CMAKE flag variables
add_subdirectory(vulkit)

if (VULKIT_BUILD_TESTS)
    add_subdirectory(tests)
endif()