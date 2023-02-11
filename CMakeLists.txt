cmake_minimum_required(VERSION 3.18)
project(monkaS LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libmnl REQUIRED IMPORTED_TARGET libmnl)
pkg_check_modules(libglog REQUIRED IMPORTED_TARGET libglog)
pkg_check_modules(gflags REQUIRED IMPORTED_TARGET gflags)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
        main.cpp
        RtNetlinkEthernetLinkAndIpAddressMonitor.cpp
        RtNetlinkEthernetLinkAndIpAddressMonitor.h
        NetworkInterfaceDescriptor.cpp
        NetworkInterfaceDescriptor.h
)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        PkgConfig::gflags
        PkgConfig::libglog
        PkgConfig::libmnl
)

