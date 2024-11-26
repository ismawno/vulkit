cmake_minimum_required(VERSION 3.16)
project(vulkit)

set(SOURCES
    vkit/core/pch.cpp
    vkit/core/core.cpp
    vkit/core/device.cpp
    vkit/core/instance.cpp
    vkit/buffer/buffer.cpp
    vkit/buffer/vertex_buffer.cpp
    vkit/buffer/index_buffer.cpp
    vkit/pipeline/compute_pipeline.cpp
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

if (NOT Vulkan_FOUND)
    message(STATUS "VULKIT: A global Vulkan installation was not found. Fetching the Vulkan SDK...")
    message(STATUS "VULKIT: Manually fetching the Vulkan SDK may not provide the necessary extensions. A global installation is recommended")
    set(VULKIT_LOCAL_VULKAN_SDK ON CACHE BOOL "Use the local Vulkan SDK" FORCE)

    FetchContent_Declare(
        VulkanHeaders
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG        sdk-1.3.250.1  # Use the appropriate version tag
    )
    FetchContent_MakeAvailable(VulkanHeaders)

    # Fetch Vulkan-Loader
    FetchContent_Declare(
        VulkanLoader
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
        GIT_TAG        sdk-1.3.250.1  # Use the appropriate version tag
    )
    FetchContent_MakeAvailable(VulkanLoader)
else()
    message(STATUS "VULKIT: Vulkan SDK found")
endif()

find_program(GLSLC_EXECUTABLE glslc REQUIRED)
message(STATUS "VULKIT: Found glslc: ${GLSLC_EXECUTABLE}")

if (VULKIT_LOCAL_VULKAN_SDK)
    target_include_directories(vulkit PUBLIC ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(vulkit PUBLIC ${Vulkan_LIBRARIES} Vulkan::Headers Vulkan::Vulkan)
endif()

target_include_directories(vulkit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Vulkan and GLFW are already linked with imgui
target_link_libraries(vulkit PUBLIC toolkit VulkanMemoryAllocator)

target_precompile_headers(vulkit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vulkit/core/pch.hpp)
target_compile_definitions(vulkit PUBLIC VKIT_ROOT_PATH="${VULKIT_ROOT_PATH}" VKIT_GLSL_BINARY="${GLSLC_EXECUTABLE}")
if (VULKIT_VALIDATION_LAYERS)
    target_compile_definitions(vulkit PUBLIC VKIT_VALIDATION_LAYERS)
endif()

if (MSVC)
    configure_compile_link_options(vulkit FLAGS /W4 /WX)
else()
    configure_compile_link_options(vulkit FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()

# No need for more: toolkit handles the rest. As it is a library I develop, I will be reusing its macros/configurations