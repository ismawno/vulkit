cmake_minimum_required(VERSION 3.16)
project(vulkit-tests)

set(NAME vulkit-tests)

include(FetchContent)

set(SOURCES tests/device.cpp tests/execution.cpp)

find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
  message(STATUS "VULKIT - Catch2 not found. Fetching source...")
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.6.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE)
  FetchContent_MakeAvailable(Catch2)
  tkit_configure_compile_link_options(Catch2 NO_EXCEPTIONS NO_RTTI)
else()
  message(STATUS "VULKIT - Catch2 found")
endif()

add_executable(vulkit-tests ${SOURCES})

target_link_libraries(vulkit-tests PRIVATE Catch2::Catch2WithMain vulkit)
target_include_directories(vulkit-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

tkit_default_configure(vulkit-tests NO_RTTI)

include(CTest)
include(Catch)
catch_discover_tests(vulkit-tests)
