cmake_minimum_required(VERSION 3.16)
project(vulkit)

set(SOURCES
    vkit/core/pch.cpp
    vkit/core/core.cpp
    vkit/vulkan/loader.cpp
    vkit/vulkan/vulkan.cpp
    vkit/vulkan/instance.cpp
    vkit/device/physical_device.cpp
    vkit/device/logical_device.cpp
    vkit/execution/queue.cpp
    vkit/memory/allocator.cpp
    vkit/resource/device_buffer.cpp
    vkit/resource/sampler.cpp
    vkit/state/descriptor_pool.cpp
    vkit/state/descriptor_set_layout.cpp
    vkit/state/descriptor_set.cpp
    vkit/state/shader.cpp
    vkit/state/pipeline_layout.cpp
    vkit/state/graphics_pipeline.cpp
    vkit/state/compute_pipeline.cpp
    vkit/execution/command_pool.cpp
    vkit/resource/device_image.cpp
    vkit/state/render_pass.cpp
    vkit/presentation/swap_chain.cpp)

add_library(vulkit STATIC ${SOURCES})
target_compile_definitions(vulkit PUBLIC VKIT_VERSION=\"v0.12.x\")

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
    message(
      FATAL_ERROR
        "VULKIT - Unsupported Vulkan SDK version: ${VULKAN_SDK_VERSION}")
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

tkit_default_configure(vulkit)
# No need for more: toolkit handles the rest. As it is a library I develop, I
# will be reusing its macros/configurations
