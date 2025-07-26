cmake_minimum_required(VERSION 3.16)
project(C3 LANGUAGES CXX)

# CMake build options
option(BUILD_TESTS "Build unit tests" ON)

# use MT/MTd runtime for MSVC
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
endif()

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# c++ standard is 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# disable compiler extensions
set(CMAKE_CXX_EXTENSIONS OFF)

# try to get thirdparty libraries
add_subdirectory(thirdparty)

# build C3
add_subdirectory(src)

# build tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
    message(STATUS "Building with tests enabled")
else()
    message(STATUS "Building without tests")
endif()