cmake_minimum_required(VERSION 3.18)
project(monkas LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libmnl REQUIRED IMPORTED_TARGET libmnl)
pkg_check_modules(gflags REQUIRED IMPORTED_TARGET gflags)
if(NOT TARGET spdlog)
    # Stand-alone build
    find_package(spdlog REQUIRED)
endif()
add_executable(${PROJECT_NAME})

# TODO: make monka lib and main link with lib
target_sources(${PROJECT_NAME}
    PRIVATE
        ethernet/Address.cpp
        ethernet/Address.h
        ip/Address.cpp
        ip/Address.h
        main.cpp
        monitor/RtnlNetworkMonitor.cpp
        monitor/RtnlNetworkMonitor.h
        network/NetworkAddress.cpp
        network/NetworkAddress.h
        network/NetworkInterfaceStatusTracker.cpp
        network/NetworkInterfaceStatusTracker.h
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        PkgConfig::gflags
        PkgConfig::libmnl
        spdlog::spdlog
)

