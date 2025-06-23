set(BUILD_SHARED_LIBS OFF)

# Configure IXWebSocket options - disable zlib to avoid conflicts
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
set(USE_DEFLATE OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG        v11.4.6
)

FetchContent_MakeAvailable(ixwebsocket)