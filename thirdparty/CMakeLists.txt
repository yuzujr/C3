include(FetchContent)


# Use cached content if available
option(FETCHCONTENT_TRY_DISCONNECTED "Use cached content if available" ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED ${FETCHCONTENT_TRY_DISCONNECTED})


# cpr: https://github.com/libcpr/cpr/releases
add_subdirectory(cpr)

# nlohmann_json: https://github.com/nlohmann/json/releases
add_subdirectory(nlohmann_json)

# spdlog: https://github.com/gabime/spdlog
add_subdirectory(spdlog)

# OpenCV: https://opencv.org/releases/
add_subdirectory(OpenCV)

# Linux: install with package manager
if(UNIX)
add_subdirectory(X11)
endif()