# build spdlog as a shared library
set(SPDLOG_BUILD_SHARED ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_STATIC OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)
FetchContent_MakeAvailable(spdlog)