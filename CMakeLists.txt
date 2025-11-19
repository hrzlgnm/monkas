cmake_minimum_required(VERSION 3.23)
project(
    monkas
    DESCRIPTION "Monkas - A C++ library for monitoring network interfaces"
    HOMEPAGE_URL "https://github.com/hrzlgnm/monkaS"
    LANGUAGES
        CXX
        C
)

include(cmake/spdlog.cmake)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libmnl REQUIRED IMPORTED_TARGET libmnl)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)

if(BUILD_TESTS)
    include(CTest)
    add_subdirectory(external/doctest)
endif()

if(BUILD_EXAMPLES)
    pkg_check_modules(gflags REQUIRED IMPORTED_TARGET gflags)
    add_subdirectory(examples)
endif()

add_subdirectory(src)
