cmake_minimum_required(VERSION 3.16)
project(vulkit)

set(SOURCES vkit/core/pch.cpp vkit/core/core.cpp vkit/vulkan/loader.cpp
            vkit/vulkan/vulkan.cpp)

if(VULKIT_ENABLE_INSTANCE)
  list(APPEND SOURCES vkit/vulkan/instance.cpp)
endif()

if(VULKIT_ENABLE_PHYSICAL_DEVICE)
  list(APPEND SOURCES vkit/device/physical_device.cpp)
endif()

if(VULKIT_ENABLE_LOGICAL_DEVICE)
  list(APPEND SOURCES vkit/device/logical_device.cpp vkit/execution/queue.cpp)
endif()

if(VULKIT_ENABLE_ALLOCATOR)
  list(APPEND SOURCES vkit/memory/allocator.cpp)
endif()

if(VULKIT_ENABLE_BUFFER)
  list(APPEND SOURCES vkit/resource/device_buffer.cpp)
endif()

if(VULKIT_ENABLE_DESCRIPTORS)
  list(APPEND SOURCES vkit/state/descriptor_pool.cpp
       vkit/state/descriptor_set_layout.cpp vkit/state/descriptor_set.cpp)
endif()

if(VULKIT_ENABLE_SHADERS)
  list(APPEND SOURCES vkit/state/shader.cpp)
endif()

if(VULKIT_ENABLE_PIPELINE_LAYOUT)
  list(APPEND SOURCES vkit/state/pipeline_layout.cpp)
endif()

if(VULKIT_ENABLE_PIPELINE_JOB)
  list(APPEND SOURCES vkit/state/pipeline_job.cpp)
endif()

if(VULKIT_ENABLE_GRAPHICS_PIPELINE)
  list(APPEND SOURCES vkit/state/graphics_pipeline.cpp)
endif()

if(VULKIT_ENABLE_COMPUTE_PIPELINE)
  list(APPEND SOURCES vkit/state/compute_pipeline.cpp)
endif()

if(VULKIT_ENABLE_COMMAND_POOL)
  list(APPEND SOURCES vkit/execution/command_pool.cpp)
endif()

if(VULKIT_ENABLE_IMAGE)
  list(APPEND SOURCES vkit/resource/image.cpp)
endif()

if(VULKIT_ENABLE_RENDER_PASS)
  list(APPEND SOURCES vkit/state/render_pass.cpp)
endif()

if(VULKIT_ENABLE_SWAP_CHAIN)
  list(APPEND SOURCES vkit/presentation/swap_chain.cpp)
endif()

add_library(vulkit STATIC ${SOURCES})
target_compile_definitions(vulkit PUBLIC VKIT_VERSION=\"v0.6.0\")

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

if(VULKIT_ENABLE_DEVICE_BUFFER)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_DEVICE_BUFFER)
endif()

if(VULKIT_ENABLE_HOST_BUFFER)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_HOST_BUFFER)
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

if(VULKIT_ENABLE_IMAGE)
  target_compile_definitions(vulkit PUBLIC VKIT_ENABLE_IMAGE)
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
  message(STATUS "VULKIT - VMA not found. Fetching headers...")
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
  message(STATUS "VULKIT - VMA found")
  target_link_libraries(vulkit PUBLIC GPUOpen::VulkanMemoryAllocator)
endif()

find_package(Vulkan QUIET)

if(NOT VULKAN_FOUND)
  message(STATUS "VULKIT - Vulkan not found. Fetching headers...")
  set(VULKAN_SDK_VERSION 1.4.328.0)

  string(REPLACE "." ";" _vk_ver_list "${VULKAN_SDK_VERSION}")
  list(GET _vk_ver_list 1 VK_VER_MINOR)

  if(VK_VER_MINOR EQUAL 4)
    set(SHADERC_VERSION v2025.3)

  elseif(VK_VER_MINOR EQUAL 3)
    set(SHADERC_VERSION v2024.1)

  else()
    message(FATAL_ERROR "Unsupported Vulkan SDK version: ${VULKAN_SDK_VERSION}")
  endif()

  FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG vulkan-sdk-${VULKAN_SDK_VERSION}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
  FetchContent_MakeAvailable(VulkanHeaders)

  target_link_libraries(vulkit PUBLIC Vulkan::Headers)
else()
  message(STATUS "VULKIT - Vulkan found")
  target_include_directories(vulkit PUBLIC ${Vulkan_INCLUDE_DIRS})
endif()

# link dlopen
if(UNIX AND NOT APPLE)
  target_link_libraries(vulkit PUBLIC ${CMAKE_DL_LIBS})
endif()

target_precompile_headers(vulkit PRIVATE
                          ${CMAKE_CURRENT_SOURCE_DIR}/vkit/core/pch.hpp)
target_compile_definitions(vulkit PUBLIC VKIT_ROOT_PATH="${VULKIT_ROOT_PATH}")

tkit_default_configure(vulkit)
# No need for more: toolkit handles the rest. As it is a library I develop, I
# will be reusing its macros/configurations
