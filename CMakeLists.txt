cmake_minimum_required(VERSION 3.18)
project(monkas LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libmnl REQUIRED IMPORTED_TARGET libmnl)
pkg_check_modules(gflags REQUIRED IMPORTED_TARGET gflags)

option(BUILD_TESTS OFF)
option(BUILD_EXAMPLES OFF)

if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()
if(BUILD_TESTS)
    add_subdirectory(external/Catch2)
include(CTest)
endif()

if(BUILD_EXAMPLES) 
    add_subdirectory(examples)
endif()
add_subdirectory(src)
