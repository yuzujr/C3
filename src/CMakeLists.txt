option(USE_WIN32_GUI "Build as a Windows GUI app" OFF)
option(USE_HARDCODED_CONFIG "Use hardcoded configuration instead of config.json" OFF)

# Create executable
add_executable(C3)

# Configure as Windows GUI application if requested
if(WIN32 AND USE_WIN32_GUI)
    set_target_properties(C3 PROPERTIES 
        WIN32_EXECUTABLE TRUE
    )
    message(STATUS "Building as Windows GUI application")
else()
    message(STATUS "Building as console application")
endif()

# common source files
target_sources(C3 PRIVATE
    main.cpp
)

# Windows-specific source files
if(WIN32)
    target_sources(C3 PRIVATE
        WinMain.cpp
    )
endif()


# enable strict compiler warnings
target_compile_options(C3 PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/WX>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Werror>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Werror>
)

# add modules: core net
add_subdirectory(core)
add_subdirectory(net)
# add modules: app(dependent on core and net)
add_subdirectory(app)


# hardcoded configuration
if(USE_HARDCODED_CONFIG)
    target_compile_definitions(core PRIVATE USE_HARDCODED_CONFIG)
    target_compile_definitions(app PRIVATE USE_HARDCODED_CONFIG)
    message(STATUS "Building with hardcoded configuration mode")
else()
    message(STATUS "Building with config.json file mode")
endif()

# link modules: app
target_link_libraries(C3 PRIVATE 
    app
)