cmake_minimum_required(VERSION 3.16)
project(vulkit)

set(SOURCES vkit/core/pch.cpp vkit/core/core.cpp vkit/vulkan/loader.cpp
            vkit/vulkan/vulkan.cpp)

if(VULKIT_ENABLE_INSTANCE)
  list(APPEND SOURCES vkit/vulkan/instance.cpp)
endif()

if(VULKIT_ENABLE_PHYSICAL_DEVICE)
  list(APPEND SOURCES vkit/vulkan/physical_device.cpp)
endif()

if(VULKIT_ENABLE_LOGICAL_DEVICE)
  list(APPEND SOURCES vkit/vulkan/logical_device.cpp)
endif()

if(VULKIT_ENABLE_ALLOCATOR)
  list(APPEND SOURCES vkit/vulkan/allocator.cpp)
endif()

if(VULKIT_ENABLE_BUFFER)
  list(APPEND SOURCES vkit/buffer/buffer.cpp)
endif()

if(VULKIT_ENABLE_DESCRIPTORS)
  list(APPEND SOURCES vkit/descriptors/descriptor_pool.cpp
       vkit/descriptors/descriptor_set_layout.cpp
       vkit/descriptors/descriptor_set.cpp)
endif()

if(VULKIT_ENABLE_SHADERS)
  list(APPEND SOURCES vkit/pipeline/shader.cpp)
endif()

if(VULKIT_ENABLE_PIPELINE_LAYOUT)
  list(APPEND SOURCES vkit/pipeline/pipeline_layout.cpp)
endif()

if(VULKIT_ENABLE_PIPELINE_JOB)
  list(APPEND SOURCES vkit/pipeline/pipeline_job.cpp)
endif()

if(VULKIT_ENABLE_GRAPHICS_PIPELINE)
  list(APPEND SOURCES vkit/pipeline/graphics_pipeline.cpp)
endif()

if(VULKIT_ENABLE_COMPUTE_PIPELINE)
  list(APPEND SOURCES vkit/pipeline/compute_pipeline.cpp)
endif()

if(VULKIT_ENABLE_COMMAND_POOL)
  list(APPEND SOURCES vkit/rendering/command_pool.cpp)
endif()

if(VULKIT_ENABLE_IMAGES)
  list(APPEND SOURCES vkit/rendering/image.cpp)
endif()

if(VULKIT_ENABLE_RENDER_PASS)
  list(APPEND SOURCES vkit/rendering/render_pass.cpp)
endif()

if(VULKIT_ENABLE_SWAP_CHAIN)
  list(APPEND SOURCES vkit/rendering/swap_chain.cpp)
endif()

add_library(vulkit STATIC ${SOURCES})
target_compile_definitions(vulkit PUBLIC VKIT_VERSION=\"v0.5.0\")

if(VULKIT_ENABLE_INSTANCE)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_INSTANCE)
endif()

if(VULKIT_ENABLE_PHYSICAL_DEVICE)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_PHYSICAL_DEVICE)
endif()

if(VULKIT_ENABLE_LOGICAL_DEVICE)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_LOGICAL_DEVICE)
endif()

if(VULKIT_ENABLE_ALLOCATOR)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_ALLOCATOR)
endif()

if(VULKIT_ENABLE_BUFFER)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_BUFFER)
endif()

if(VULKIT_ENABLE_DESCRIPTORS)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_DESCRIPTORS)
endif()

if(VULKIT_ENABLE_SHADERS)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_SHADERS)
endif()

if(VULKIT_ENABLE_PIPELINE_LAYOUT)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_PIPELINE_LAYOUT)
endif()

if(VULKIT_ENABLE_PIPELINE_JOB)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_PIPELINE_JOB)
endif()

if(VULKIT_ENABLE_GRAPHICS_PIPELINE)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_GRAPHICS_PIPELINE)
endif()

if(VULKIT_ENABLE_COMPUTE_PIPELINE)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_COMPUTE_PIPELINE)
endif()

if(VULKIT_ENABLE_COMMAND_POOL)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_COMMAND_POOL)
endif()

if(VULKIT_ENABLE_IMAGES)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_IMAGES)
endif()

if(VULKIT_ENABLE_RENDER_PASS)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_RENDER_PASS)
endif()

if(VULKIT_ENABLE_SWAP_CHAIN)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_SWAP_CHAIN)
endif()

include(FetchContent)
FetchContent_Declare(
  toolkit
  GIT_REPOSITORY https://github.com/ismawno/toolkit.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE)
FetchContent_MakeAvailable(toolkit)
target_compile_definitions(
  toolkit
  PUBLIC TKIT_USIZE_TYPE=TKit::Alias::u32 TKIT_SSIZE_TYPE=TKit::Alias::i32
         TKIT_DIFFERENCE_TYPE=TKit::Alias::i32)

target_include_directories(vulkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vulkit PUBLIC toolkit)

find_package(VulkanMemoryAllocator QUIET)
if(NOT VulkanMemoryAllocator_FOUND)
  message(STATUS "VULKIT: VMA not found. Fetching headers...")
  FetchContent_Declare(
    vma
    GIT_REPOSITORY
      https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.1.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

  FetchContent_MakeAvailable(vma)
  target_include_directories(vulkit PUBLIC ${vma_SOURCE_DIR}/include)
else()
  message(STATUS "VULKIT: VMA found")
  target_link_libraries(vulkit PUBLIC GPUOpen::VulkanMemoryAllocator)
endif()

find_package(Vulkan QUIET)

if(NOT VULKAN_FOUND)
  message(STATUS "VULKIT: Vulkan not found. Fetching headers...")
  set(VULKAN_SDK_VERSION 1.4.321.0)
  FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG vulkan-sdk-${VULKAN_SDK_VERSION}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
  FetchContent_MakeAvailable(VulkanHeaders)

  target_link_libraries(vulkit PUBLIC Vulkan::Headers)
else()
  message(STATUS "VULKIT: Vulkan found")
  target_include_directories(vulkit PUBLIC ${Vulkan_INCLUDE_DIRS})
endif()

find_program(GLSLC_EXECUTABLE glslc QUIET)

if(NOT GLSLC_EXECUTABLE)
  message(STATUS "VULKIT: GLSLC executable not found. Fetching source...")

  FetchContent_Declare(
    spirv-headers
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
    GIT_TAG vulkan-sdk-${VULKAN_SDK_VERSION}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
  FetchContent_MakeAvailable(spirv-headers)

  FetchContent_Declare(
    spirv-tools
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
    GIT_TAG vulkan-sdk-${VULKAN_SDK_VERSION}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

  set(SPIRV_SKIP_TESTS
      ON
      CACHE BOOL "" FORCE)
  set(SPIRV_TOOLS_BUILD_STATIC
      ON
      CACHE BOOL "" FORCE)
  set(SPIRV_SKIP_EXECUTABLES
      ON
      CACHE BOOL "" FORCE)
  set(SKIP_SPIRV_TOOLS_INSTALL
      ON
      CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(spirv-tools)

  FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
    GIT_TAG vulkan-sdk-${VULKAN_SDK_VERSION}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

  set(SKIP_GLSLANG_INSTALL
      ON
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glslang)

  FetchContent_Declare(
    shaderc
    GIT_REPOSITORY https://github.com/google/shaderc.git
    GIT_TAG v2025.3
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)

  set(SHADERC_SKIP_TESTS
      ON
      CACHE BOOL "" FORCE)
  set(SHADERC_SKIP_INSTALL
      ON
      CACHE BOOL "" FORCE)
  set(SHADERC_ENABLE_SHARED_CRT
      OFF
      CACHE BOOL "" FORCE)
  set(SHADERC_ENABLE_SPIRV_TOOLS_INSTALL
      ON
      CACHE BOOL "" FORCE)
  set(SHADERC_SKIP_EXAMPLES
      ON
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(shaderc)

  set(GLSLC_EXECUTABLE ${shaderc_BINARY_DIR}/glslc)
else()
  message(STATUS "VULKIT: GLSLC executable found")
endif()

# link dlopen
if(UNIX AND NOT APPLE)
  target_link_libraries(vulkit PUBLIC ${CMAKE_DL_LIBS})
endif()

target_precompile_headers(vulkit PRIVATE
                          ${CMAKE_CURRENT_SOURCE_DIR}/vkit/core/pch.hpp)
target_compile_definitions(vulkit PUBLIC VKIT_ROOT_PATH="${VULKIT_ROOT_PATH}"
                                         VKIT_GLSL_BINARY="${GLSLC_EXECUTABLE}")

tkit_default_configure(vulkit)
# No need for more: toolkit handles the rest. As it is a library I develop, I
# will be reusing its macros/configurations
