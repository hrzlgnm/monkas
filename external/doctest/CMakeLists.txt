add_library(doctest INTERFACE)
add_library(doctest::doctest ALIAS doctest)

# HACK(hrzgnm): disable clang-tidy checks on doctest
target_include_directories(doctest SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(doctest_lib lib.cpp)
add_library(doctest::lib ALIAS doctest_lib)
target_link_libraries(doctest_lib PRIVATE doctest::doctest)
add_executable(doctest_exe main.cpp)

target_link_libraries(
    doctest_exe
    PRIVATE
        doctest::doctest
        doctest::lib
        monkas::lib
        monkas::tests
)
add_executable(doctest::exe ALIAS doctest_exe)
set_property(
    TARGET
        doctest_exe
    PROPERTY
        OUTPUT_NAME
            doctest
)
target_compile_features(doctest_exe PRIVATE cxx_std_20)

add_test(
    NAME doctest
    COMMAND
        $<TARGET_FILE:doctest_exe> --no-version --reporters=junit --no-breaks --no-colors --out=junit.xml
)

# HACK(hrzgnm): disable analyzer checks on doctest targets
set_target_properties(
    doctest_exe
    PROPERTIES
        CXX_CPPCHECK
            ""
)
set_target_properties(
    doctest_lib
    PROPERTIES
        CXX_CPPCHECK
            ""
)
