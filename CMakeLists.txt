cmake_minimum_required(VERSION 3.16)
project(ScreenUploader LANGUAGES CXX)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Build cpr and nlohmann_json with fetch_content
# a proper network connection is required
# or you can download and build them manually, then use "find_package" instead
# cpr: https://github.com/libcpr/cpr/releases
# nlohmann_json: https://github.com/nlohmann/json/releases
add_subdirectory(thirdparty)


# Try to find OpenCV and X11(Linux)
# Installing them with your package manager is recommended
# If you are using Windows, you can download OpenCV from https://opencv.org/releases/
if(UNIX)
    find_package(X11 REQUIRED)
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs)


# build ScreenUploader
add_subdirectory(src)