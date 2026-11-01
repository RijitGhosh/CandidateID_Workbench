cmake_minimum_required(VERSION 3.21)
project(AetherWorkbench VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(AETHER_BUILD_GUI   "Build the Qt 6 desktop application" ON)
option(AETHER_BUILD_TESTS "Build the GoogleTest suite" ON)

find_package(Threads REQUIRED)

# ---------------------------------------------------------------- core (Qt-free)
add_library(aether_core STATIC
  src/core/thermal/thermal.cpp
  src/core/geometry/stl.cpp
  src/core/csv/temperature_csv.cpp
  src/core/performance/batch.cpp
  src/core/performance/benchmark.cpp
  src/core/neural/backprop.cpp
  src/core/hardware/mock_devices.cpp
  src/core/verification/selfcheck.cpp)
target_include_directories(aether_core PUBLIC src)
target_link_libraries(aether_core PUBLIC Threads::Threads)
if(MSVC)
  target_compile_options(aether_core PRIVATE /W4)
else()
  target_compile_options(aether_core PRIVATE -Wall -Wextra)
endif()
# Recorded in benchmark reports (compiler flags actually used for this configuration)
if(CMAKE_BUILD_TYPE)
  string(TOUPPER "${CMAKE_BUILD_TYPE}" _aether_bt)
else()
  set(_aether_bt RELEASE)
endif()
string(STRIP "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_aether_bt}}" _aether_flags)
string(REPLACE "\"" "" _aether_flags "${_aether_flags}")
target_compile_definitions(aether_core PRIVATE
  AETHER_BUILD_TYPE="${_aether_bt}"
  AETHER_CXX_FLAGS="${_aether_flags}")

# ---------------------------------------------------------------- application layer (Qt-free)
add_library(aether_application STATIC src/application/services/workspace.cpp)
target_link_libraries(aether_application PUBLIC aether_core)

# ---------------------------------------------------------------- command-line tools
add_executable(aether_results src/tools/results_cli.cpp)
target_link_libraries(aether_results PRIVATE aether_core)
add_executable(make_examples src/tools/make_examples.cpp)

# ---------------------------------------------------------------- tests
if(AETHER_BUILD_TESTS)
  enable_testing()
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    message(STATUS "GTest not found on the system; fetching GoogleTest v1.14.0 (needs internet once)")
    include(FetchContent)
    FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()
  add_executable(aether_tests
    tests/test_thermal.cpp tests/test_stl.cpp tests/test_csv.cpp tests/test_batch.cpp
    tests/test_neural.cpp tests/test_hardware.cpp tests/test_application.cpp tests/test_integration.cpp)
  target_link_libraries(aether_tests PRIVATE aether_application GTest::gtest GTest::gtest_main)
  target_compile_definitions(aether_tests PRIVATE AETHER_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
  include(GoogleTest)
  gtest_discover_tests(aether_tests DISCOVERY_TIMEOUT 60)
endif()

# ---------------------------------------------------------------- GUI + launcher
if(AETHER_BUILD_GUI)
  find_package(Qt6 6.4 COMPONENTS Widgets QUIET)
  if(Qt6_FOUND)
    add_subdirectory(src/frontend)
  else()
    message(WARNING "Qt 6 Widgets not found: the desktop application is NOT built (core, tools and tests still are).")
  endif()
endif()
if(WIN32)
  add_executable(Launch launcher/launch.cpp)
  target_link_options(Launch PRIVATE $<$<CXX_COMPILER_ID:GNU>:-mwindows -static>)
  set_target_properties(Launch PROPERTIES WIN32_EXECUTABLE ON MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()
