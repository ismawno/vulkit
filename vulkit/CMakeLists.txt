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

if(VULKIT_ENABLE_RENDER_PASS)
  list(APPEND SOURCES vkit/rendering/render_pass.cpp)
endif()

if(VULKIT_ENABLE_SWAP_CHAIN)
  list(APPEND SOURCES vkit/rendering/swap_chain.cpp)
endif()

add_library(vulkit STATIC ${SOURCES})

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
  GIT_TAG main)
FetchContent_MakeAvailable(toolkit)
target_compile_definitions(toolkit PUBLIC TKIT_SIZE_TYPE=TKit::Alias::u32
                                          TKIT_DIFFERENCE_TYPE=TKit::Alias::i32)

FetchContent_Declare(
  vma
  GIT_REPOSITORY
    https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG v3.1.0)
FetchContent_MakeAvailable(vma)

# It is set to required for now even though there is a fallback mechanism with
# FetchContent (the latter seems to provide very few extensions and I am not
# sure if it will work well for now) TODO: Check if the fallback mechanism works
# well
find_package(Vulkan REQUIRED)
message(STATUS "VULKIT: Vulkan SDK found")

function(vkit_add_vulkan_executable_macos_rpath target)
  get_filename_component(VULKAN_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
  set_target_properties(
    ${target}
    PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE
               INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${VULKAN_LIB_DIR}"
               INSTALL_RPATH_USE_LINK_PATH TRUE)
endfunction()

find_program(GLSLC_EXECUTABLE glslc REQUIRED)
message(STATUS "VULKIT: Found glslc: ${GLSLC_EXECUTABLE}")

target_include_directories(vulkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
                                         ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkit PUBLIC toolkit VulkanMemoryAllocator)
# link dlopen
if(UNIX AND NOT APPLE)
  target_link_libraries(vulkit PUBLIC ${CMAKE_DL_LIBS})
endif()

target_precompile_headers(vulkit PRIVATE
                          ${CMAKE_CURRENT_SOURCE_DIR}/vkit/core/pch.hpp)
target_compile_definitions(vulkit PUBLIC VKIT_ROOT_PATH="${VULKIT_ROOT_PATH}"
                                         VKIT_GLSL_BINARY="${GLSLC_EXECUTABLE}")

if(MSVC)
  tkit_configure_compile_link_options(vulkit FLAGS /W4 /WX)
else()
  tkit_configure_compile_link_options(vulkit FLAGS -Wall -Wextra -Wpedantic
                                      -Werror)
endif()

# No need for more: toolkit handles the rest. As it is a library I develop, I
# will be reusing its macros/configurations
