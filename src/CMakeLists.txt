# build ScreenUploader
option(USE_WIN32_GUI "Build as a Windows GUI app" OFF)

if (WIN32 AND USE_WIN32_GUI)
    add_executable(ScreenUploader WIN32)
else()
    add_executable(ScreenUploader)
endif()


# add modules: core net
add_subdirectory(core)
add_subdirectory(net)


# common source files
target_sources(ScreenUploader PRIVATE
    main.cpp
    platform.cpp
)


# enable strict compiler warnings
target_compile_options(ScreenUploader PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/WX>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Werror>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Werror>
)


# link modules: core net
target_link_libraries(ScreenUploader PRIVATE 
    core 
    net
)