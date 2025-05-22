set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (UNIX)
set(CPR_USE_SYSTEM_CURL ON)
endif()

FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.11.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cpr)