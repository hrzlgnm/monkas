cmake_minimum_required(VERSION 3.18)
project(monkaS LANGUAGES CXX C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libmnl REQUIRED IMPORTED_TARGET libmnl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PRIVATE
        main.cpp
        monka.cpp
        monka.h
)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        PkgConfig::libmnl
)
