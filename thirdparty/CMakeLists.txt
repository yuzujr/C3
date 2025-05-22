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

# libjpeg-turbo: https://github.com/libjpeg-turbo/libjpeg-turbo
add_subdirectory(libjpeg-turbo)

# Linux: install with package manager
if(UNIX)
add_subdirectory(X11)
endif()