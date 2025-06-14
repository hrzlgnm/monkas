set(TARGET_NAME monkas)

add_library(${TARGET_NAME} OBJECT)
add_library(${TARGET_NAME}::lib ALIAS ${TARGET_NAME})

set(PUBLIC_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/${TARGET_NAME})

set(PUBLIC_HEADERS
    ${PUBLIC_INCLUDE_DIR}/ethernet/Address.hpp
    ${PUBLIC_INCLUDE_DIR}/ip/Address.hpp
    ${PUBLIC_INCLUDE_DIR}/network/NetworkAddress.hpp
    ${PUBLIC_INCLUDE_DIR}/monitor/NetworkInterfaceStatusTracker.hpp
    ${PUBLIC_INCLUDE_DIR}/monitor/RtnlNetworkMonitor.hpp
    ${PUBLIC_INCLUDE_DIR}/observable/Observable.hpp
)

target_compile_definitions(${TARGET_NAME}
    PUBLIC SPDLOG_FMT_EXTERNAL)

target_include_directories(${TARGET_NAME}
    PUBLIC
        ${PUBLIC_INCLUDE_DIR}
)

target_sources(${TARGET_NAME}
    PRIVATE
        ${PUBLIC_HEADERS}
        ethernet/Address.cpp
        ip/Address.cpp
        monitor/NetworkInterfaceStatusTracker.cpp
        monitor/RtnlNetworkMonitor.cpp
        network/Interface.cpp
        network/NetworkAddress.cpp
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        PkgConfig::gflags
        PkgConfig::libmnl
        spdlog::spdlog
)

if (BUILD_TESTS)
  target_sources(${TARGET_NAME}
    PRIVATE
      ethernet/Address.test.cpp
      ip/Address.test.cpp
      network/NetworkAddress.test.cpp
      observable/Observable.test.cpp
  )
  target_link_libraries(${TARGET_NAME}
    PRIVATE
       doctest::doctest
       doctest::lib
  )
endif()

