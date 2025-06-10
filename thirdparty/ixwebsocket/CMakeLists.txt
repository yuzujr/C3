set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG        v11.4.6
)

FetchContent_MakeAvailable(ixwebsocket)