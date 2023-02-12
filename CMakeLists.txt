cmake_minimum_required(VERSION 3.18)
project(monkas LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libmnl REQUIRED IMPORTED_TARGET libmnl)
# TODO: migrate to https://github.com/gabime/spdlog instead of glog
pkg_check_modules(libglog REQUIRED IMPORTED_TARGET libglog)
pkg_check_modules(gflags REQUIRED IMPORTED_TARGET gflags)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
        ethernet/Address.cpp
        ethernet/Address.h
        ip/Address.cpp
        ip/Address.h
        main.cpp
        network/NetworkAddress.cpp
        network/NetworkAddress.h
        RtnlNetworkMonitor.cpp
        RtnlNetworkMonitor.h
        NetworkInterface.cpp
        NetworkInterface.h
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        PkgConfig::gflags
        PkgConfig::libglog
        PkgConfig::libmnl
)

