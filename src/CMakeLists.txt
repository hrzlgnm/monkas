set(TARGET_NAME monkas)

add_library(${TARGET_NAME} STATIC)
add_library(${TARGET_NAME}::lib ALIAS ${TARGET_NAME})
set(PUBLIC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/${TARGET_NAME})

set(PUBLIC_HEADERS
    ${PUBLIC_INCLUDE_DIR}/ethernet/Address.hpp
    ${PUBLIC_INCLUDE_DIR}/ip/Address.hpp
    ${PUBLIC_INCLUDE_DIR}/monitor/NetworkInterfaceStatusTracker.hpp
    ${PUBLIC_INCLUDE_DIR}/monitor/NetworkMonitor.hpp
    ${PUBLIC_INCLUDE_DIR}/network/Address.hpp
    ${PUBLIC_INCLUDE_DIR}/network/Interface.hpp
    ${PUBLIC_INCLUDE_DIR}/util/FlagSet.hpp
)

target_compile_definitions(${TARGET_NAME} PUBLIC SPDLOG_FMT_EXTERNAL)

target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)

target_include_directories(${TARGET_NAME} PUBLIC ${PUBLIC_INCLUDE_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_sources(
    ${TARGET_NAME}
    PRIVATE
        ${PUBLIC_HEADERS}
        ethernet/Address.cpp
        ip/Address.cpp
        monitor/Attributes.cpp
        monitor/NetworkInterfaceStatusTracker.cpp
        monitor/NetworkMonitor.cpp
        network/Address.cpp
        network/Interface.cpp
    PRIVATE
        FILE_SET HEADERS
            FILES monitor/Attributes.hpp
)

target_link_libraries(
    ${TARGET_NAME}
    PRIVATE
        PkgConfig::libmnl
        spdlog::spdlog
)

if(BUILD_TESTS)
    add_library(${TARGET_NAME}_tests OBJECT)
    add_library(${TARGET_NAME}::tests ALIAS ${TARGET_NAME}_tests)
    target_sources(
        ${TARGET_NAME}_tests
        PRIVATE
            ethernet/Address.test.cpp
            ip/Address.test.cpp
            network/Address.test.cpp
            network/Interface.test.cpp
            monitor/NetworkInterfaceStatusTracker.test.cpp
    )
    target_link_libraries(
        ${TARGET_NAME}_tests
        PRIVATE
            doctest::doctest
            doctest::lib
            ${TARGET_NAME}::lib
    )
endif()
