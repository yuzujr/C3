cmake_minimum_required(VERSION 3.10)
project(ScreenUploader)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

# 设置 OpenCV 路径（仅限 Windows 手动设置）
if(WIN32)
    set(OpenCV_DIR "F:/libs/opencv/build-no-world/x64/vc17/lib")
endif()

if(UNIX)
    find_package(X11 REQUIRED)
endif()
find_package(OpenCV REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

set(SOURCES src/main.cpp 
src/ScreenUploader.cpp src/ScreenUploader.h 
src/Logger.cpp src/Logger.h
src/Config.cpp src/Config.h
)

add_executable(ScreenUploader ${SOURCES})

target_link_libraries(ScreenUploader PRIVATE CURL::libcurl ${OpenCV_LIBS} nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(ScreenUploader PRIVATE Shcore.lib)
endif()

if(UNIX)
    target_link_libraries(ScreenUploader PRIVATE X11::X11)
endif()