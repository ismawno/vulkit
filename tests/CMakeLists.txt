cmake_minimum_required(VERSION 3.16)
project(vulkit-tests)

set(NAME vulkit-tests)

include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.6.0
)

FetchContent_MakeAvailable(Catch2)

set(SOURCES
    tests/utils.cpp
    tests/backend/instance.cpp
    tests/backend/physical_device.cpp
)

add_executable(vulkit-tests ${SOURCES})

target_link_libraries(vulkit-tests PRIVATE Catch2::Catch2WithMain vulkit)
target_include_directories(vulkit-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
    tkit_configure_compile_link_options(vulkit-tests FLAGS /W4 /WX)
else()
    tkit_configure_compile_link_options(vulkit-tests FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()

include(CTest)
include(Catch)
Catch_discover_tests(vulkit-tests)
