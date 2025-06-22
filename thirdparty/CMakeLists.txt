include(FetchContent)
include(ExternalProject)
# make cache variables for install destinations
include(GNUInstallDirs)

# Use cached content if available
option(FETCHCONTENT_TRY_DISCONNECTED "Use cached content if available" ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ${FETCHCONTENT_TRY_DISCONNECTED})


# cpr: https://github.com/libcpr/cpr/releases
add_subdirectory(cpr)

# nlohmann_json: https://github.com/nlohmann/json/releases
add_subdirectory(nlohmann_json)

# spdlog: https://github.com/gabime/spdlog
add_subdirectory(spdlog)

# Google Test: https://github.com/google/googletest
add_subdirectory(googletest)

# libjpeg-turbo: https://github.com/libjpeg-turbo/libjpeg-turbo
add_subdirectory(libjpeg-turbo)

# ixwebsocket: https://github.com/machinezone/IXWebSocket/
add_subdirectory(ixwebsocket)

# Linux: install with package manager
if(UNIX)
add_subdirectory(X11)
endif()