cmake_minimum_required(VERSION 3.14)
project(ScreenUploader)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)


# third-party libraries
# If these libraries cannot be found
# 1. you can try setting "xx_DIR" to the path of the library's CMake configuration file.
# For example:
# set(OpenCV_DIR "path/to/opencv")
# 2. or adding it to CMAKE_PREFIX_PATH.
# For example:
# set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};path/to/your/libraries")
if(UNIX)
    find_package(X11 REQUIRED)
endif()
find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)


# common source files
file(GLOB SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
# platform-specific source files
# Windows: src/platform/windows/*.cpp
# Linux: src/platform/linux/*.cpp
if(WIN32)
  file(GLOB_RECURSE WIN_SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/platform/windows/*.cpp")
  list(APPEND SRC ${WIN_SRC})
elseif(UNIX)
  file(GLOB_RECURSE LINUX_SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/platform/linux/*.cpp")
  list(APPEND SRC ${LINUX_SRC})
endif()


add_executable(ScreenUploader ${SRC})


# link common libraries
target_link_libraries(ScreenUploader PRIVATE CURL::libcurl ${OpenCV_LIBS} nlohmann_json::nlohmann_json)
# link platform-specific libraries
if(WIN32)
    target_link_libraries(ScreenUploader PRIVATE Shcore.lib)
elseif(UNIX)
    target_link_libraries(ScreenUploader PRIVATE X11::X11)
endif()


# set include directories
target_include_directories(ScreenUploader PRIVATE ${CMAKE_SOURCE_DIR}/include/ScreenUploader)