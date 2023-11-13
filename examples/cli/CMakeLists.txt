add_executable(monkas)

target_sources(monkas
    PRIVATE
        main.cpp
)

target_compile_definitions(monkas PRIVATE DOCTEST_CONFIG_DISABLE)
target_link_libraries(monkas
    PRIVATE
        monka::lib
)
