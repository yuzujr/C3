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
add_subdirectory(app)


# common source files
target_sources(ScreenUploader PRIVATE
    main.cpp
    WinMain.cpp
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


# link modules: app
target_link_libraries(ScreenUploader PRIVATE 
    app
)