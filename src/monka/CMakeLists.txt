set(TARGET_NAME monka)

add_library(${TARGET_NAME} OBJECT)
add_library(${TARGET_NAME}::lib ALIAS ${TARGET_NAME})

set(PUBLIC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/${TARGET_NAME})

set(PUBLIC_HEADERS
    ${PUBLIC_INCLUDE_DIR}/ethernet/Address.h
    ${PUBLIC_INCLUDE_DIR}/ip/Address.h
    ${PUBLIC_INCLUDE_DIR}/network/NetworkAddress.h
    ${PUBLIC_INCLUDE_DIR}/monitor/NetworkInterfaceStatusTracker.h
    ${PUBLIC_INCLUDE_DIR}/monitor/RtnlNetworkMonitor.h
)

target_compile_definitions(${TARGET_NAME}
    PUBLIC SPDLOG_FMT_EXTERNAL)

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${PUBLIC_INCLUDE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(${TARGET_NAME}
    PRIVATE
        ethernet/Address.cpp
        ip/Address.cpp
        monitor/NetworkInterfaceStatusTracker.cpp
        monitor/RtnlNetworkMonitor.cpp
        network/NetworkAddress.cpp
        observable/Observable.h
        observable/Observable.test.cpp
        ${PUBLIC_HEADERS}
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        PkgConfig::gflags
        PkgConfig::libmnl
        spdlog::spdlog
        doctest::doctest
        doctest::dll
)

