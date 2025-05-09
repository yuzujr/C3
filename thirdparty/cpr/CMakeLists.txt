set(BUILD_SHARED_LIBS ON)

if (UNIX)
set(CPR_USE_SYSTEM_CURL ON)
endif()

FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.11.2
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(cpr)