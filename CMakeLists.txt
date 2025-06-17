cmake_minimum_required(VERSION 3.18)
project(
    monkas
    DESCRIPTION "Monkas - A C++ library for monitoring network interfaces"
    HOMEPAGE_URL "https://github.com/hrzlgnm/monkaS"
    LANGUAGES
        CXX
        C
)

option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)

cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libmnl REQUIRED IMPORTED_TARGET libmnl)
pkg_check_modules(gflags REQUIRED IMPORTED_TARGET gflags)

if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

if(BUILD_TESTS)
    include(CTest)
    add_subdirectory(external/doctest)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

add_subdirectory(src)
