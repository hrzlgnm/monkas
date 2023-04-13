add_executable(monkas)

target_sources(monkas
    PRIVATE
        main.cpp
)

target_link_libraries(monkas
    PRIVATE
        monka::monka
)
