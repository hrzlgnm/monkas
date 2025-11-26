# Copyright 2023-2025 hrzlgnm
# SPDX-License-Identifier: MIT-0

add_executable(monka)

target_sources(monka PRIVATE main.cpp)

target_compile_definitions(monka PRIVATE DOCTEST_CONFIG_DISABLE)
target_link_libraries(
    monka
    PRIVATE
        monkas::lib
        PkgConfig::gflags
)
