include(FetchContent)

FetchContent_Declare(spdlog GIT_REPOSITORY "https://github.com/gabime/spdlog.git" GIT_TAG "v1.16.0")

FetchContent_MakeAvailable(spdlog)

# HACK(hrzgnm): disable analyzer checks on spdlog library
set_target_properties(
    spdlog
    PROPERTIES
        CXX_CLANG_TIDY
            ""
        CXX_CPPCHECK
            ""
)
