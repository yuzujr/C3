# build ScreenUploader
option(USE_WIN32_GUI "Build as a Windows GUI app" OFF)
option(USE_HARDCODED_CONFIG "Use hardcoded configuration instead of config.json" OFF)

if (WIN32 AND USE_WIN32_GUI)
    add_executable(ScreenUploader WIN32)
else()
    add_executable(ScreenUploader)
endif()

# add modules: core net
add_subdirectory(core)
add_subdirectory(net)
# add modules: app(denpendent on core and net)
add_subdirectory(app)

# hardcoded configuration
if(USE_HARDCODED_CONFIG)
    target_compile_definitions(core PRIVATE USE_HARDCODED_CONFIG)
    target_compile_definitions(app PRIVATE USE_HARDCODED_CONFIG)
    message(STATUS "Building with hardcoded configuration mode")
else()
    message(STATUS "Building with config.json file mode")
endif()


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