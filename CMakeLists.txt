cmake_minimum_required(VERSION 3.16...3.31)
project(vulkit-root)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(define_option varname default unused_opt_type)
  if(NOT DEFINED ${varname})
    set(${varname}
        ${default}
        PARENT_SCOPE)
  endif()
endfunction()

define_option(CMAKE_BUILD_TYPE "Debug" "Vulkit - Build")

define_option(VULKIT_ENABLE_INSTANCE OFF "Vulkit - Setup")
define_option(VULKIT_ENABLE_PHYSICAL_DEVICE OFF "Vulkit - Setup")
define_option(VULKIT_ENABLE_LOGICAL_DEVICE OFF "Vulkit - Setup")

define_option(VULKIT_ENABLE_ALLOCATOR OFF "Vulkit - Feature")
define_option(VULKIT_ENABLE_BUFFER OFF "Vulkit - Feature")

define_option(VULKIT_ENABLE_DESCRIPTORS OFF "Vulkit - Feature")

define_option(VULKIT_ENABLE_SHADERS OFF "Vulkit - Feature")
define_option(VULKIT_ENABLE_PIPELINE_LAYOUT OFF "Vulkit - Feature")
define_option(VULKIT_ENABLE_PIPELINE_JOB OFF "Vulkit - Feature")
define_option(VULKIT_ENABLE_GRAPHICS_PIPELINE OFF "Vulkit - Feature")
define_option(VULKIT_ENABLE_COMPUTE_PIPELINE OFF "Vulkit - Feature")

define_option(VULKIT_ENABLE_COMMAND_POOL OFF "Vulkit - Feature")
define_option(VULKIT_ENABLE_IMAGES OFF "Vulkit - Feature")
define_option(VULKIT_ENABLE_RENDER_PASS OFF "Vulkit - Feature")
define_option(VULKIT_ENABLE_SWAP_CHAIN OFF "Vulkit - Feature")

set(VULKIT_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# Compiler flags, linker flags etcetera, those should all be handled when making
# available the toolkit library through the global CMAKE flag variables
add_subdirectory(vulkit)
