cmake_minimum_required(VERSION 3.16)
project(vulkit-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(define_option varname default)
  if(NOT DEFINED ${varname})
    set(${varname}
        ${default}
        PARENT_SCOPE)
  endif()
endfunction()

define_option(CMAKE_BUILD_TYPE "Debug")

define_option(VULKIT_ENABLE_INSTANCE OFF)
define_option(VULKIT_ENABLE_PHYSICAL_DEVICE OFF)
define_option(VULKIT_ENABLE_LOGICAL_DEVICE OFF)
define_option(VULKIT_ENABLE_ALLOCATOR OFF)

define_option(VULKIT_ENABLE_BUFFER OFF)

define_option(VULKIT_ENABLE_DESCRIPTORS OFF)

define_option(VULKIT_ENABLE_SHADERS OFF)
define_option(VULKIT_ENABLE_PIPELINE_LAYOUT OFF)
define_option(VULKIT_ENABLE_PIPELINE_JOB OFF)
define_option(VULKIT_ENABLE_GRAPHICS_PIPELINE OFF)
define_option(VULKIT_ENABLE_COMPUTE_PIPELINE OFF)

define_option(VULKIT_ENABLE_COMMAND_POOL OFF)
define_option(VULKIT_ENABLE_IMAGES OFF)
define_option(VULKIT_ENABLE_RENDER_PASS OFF)
define_option(VULKIT_ENABLE_SWAP_CHAIN OFF)

set(VULKIT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler flags, linker flags etcetera, those should all be handled when making
# available the toolkit library through the global CMAKE flag variables
add_subdirectory(vulkit)
