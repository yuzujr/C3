set(SPDLOG_BUILD_PIC ON)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_STATIC ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)
FetchContent_MakeAvailable(spdlog)