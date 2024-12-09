cmake_minimum_required(VERSION 3.16)
project(vulkit)

set(SOURCES
    vkit/core/pch.cpp
    vkit/backend/logical_device.cpp
    vkit/backend/physical_device.cpp
    vkit/backend/instance.cpp
    vkit/backend/system.cpp
    vkit/backend/command_pool.cpp
    vkit/backend/swap_chain.cpp
    vkit/buffer/buffer.cpp
    vkit/pipeline/compute_pipeline.cpp
    vkit/pipeline/graphics_pipeline.cpp
    vkit/pipeline/pipeline_layout.cpp
    vkit/pipeline/shader.cpp
    vkit/descriptors/descriptor_pool.cpp
    vkit/descriptors/descriptor_set_layout.cpp
    vkit/descriptors/descriptor_writer.cpp
)

add_library(vulkit STATIC ${SOURCES})

include(FetchContent)
FetchContent_Declare(
    toolkit
    GIT_REPOSITORY https://github.com/ismawno/toolkit.git
    GIT_TAG main
)
FetchContent_MakeAvailable(toolkit)

FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.1.0
)
FetchContent_MakeAvailable(vma)

# It is set to required for now even though there is a fallback mechanism with FetchContent (the latter seems to provide very few extensions and I am not sure if it will work well for now)
# TODO: Check if the fallback mechanism works well
find_package(Vulkan REQUIRED)
message(STATUS "VULKIT: Vulkan SDK found")

find_program(GLSLC_EXECUTABLE glslc REQUIRED)
message(STATUS "VULKIT: Found glslc: ${GLSLC_EXECUTABLE}")

target_include_directories(vulkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Vulkan and GLFW are already linked with imgui
target_link_libraries(vulkit PUBLIC toolkit Vulkan::Vulkan VulkanMemoryAllocator)

target_precompile_headers(vulkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vkit/core/pch.hpp)
target_compile_definitions(vulkit PUBLIC VKIT_ROOT_PATH="${VULKIT_ROOT_PATH}" VKIT_GLSL_BINARY="${GLSLC_EXECUTABLE}")

if (MSVC)
    configure_compile_link_options(vulkit FLAGS /W4 /WX)
else()
    configure_compile_link_options(vulkit FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()

# No need for more: toolkit handles the rest. As it is a library I develop, I will be reusing its macros/configurations