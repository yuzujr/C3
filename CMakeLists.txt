cmake_minimum_required(VERSION 3.16)
project(ScreenUploader LANGUAGES CXX)

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


# build ScreenUploader
add_subdirectory(src)


# build tests
enable_testing()
add_subdirectory(test/test_pty)