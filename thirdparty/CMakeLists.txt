include(FetchContent)
include(ExternalProject)
# make cache variables for install destinations
include(GNUInstallDirs)


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

# Wayland wlr-screencopy support (optional)
option(ENABLE_WLR_SCREENCOPY "Enable Wayland wlr-screencopy capture backend" ON)
if(ENABLE_WLR_SCREENCOPY)
    find_program(WAYLAND_SCANNER wayland-scanner)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(WAYLAND_CLIENT wayland-client)
    endif()
    if(WAYLAND_SCANNER AND WAYLAND_CLIENT_FOUND)
        add_subdirectory(wayland-protocols)
        message(STATUS "Wayland wlr-screencopy support: ENABLED")
    else()
        message(STATUS "Wayland wlr-screencopy support: DISABLED (missing wayland-scanner or wayland-client)")
        set(ENABLE_WLR_SCREENCOPY OFF CACHE BOOL "" FORCE)
    endif()
endif()
endif()