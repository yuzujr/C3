# module: core
add_library(core STATIC)


# common source files
target_sources(core PRIVATE
    Config.cpp
    Logger.cpp
    IDGenerator.cpp
    ControlCenter.cpp
    CommandDispatcher.cpp
    ImageEncoder.cpp
    RawImage.cpp
)
# platform-specific source files
if(WIN32)
    target_sources(core PRIVATE
        platform/windows/ScreenCapturer.cpp
        platform/windows/SystemUtils.cpp
        platform/windows/GDIRAIIClasses.cpp
        platform/windows/PtyManager.cpp
    )
elseif(UNIX)
    target_sources(core PRIVATE
        platform/linux/ScreenCapturer.cpp
        platform/linux/SystemUtils.cpp
        platform/linux/X11RAIIClasses.cpp
    )
endif()


# enable strict compiler warnings
target_compile_options(core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/WX>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Werror>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Werror>
)


# include directories PUBLIC
target_include_directories(core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# link third-party libraries PUBLIC
target_link_libraries(core PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    jpeg
)
# link system dependent libraries PRIVATE
if(WIN32)
    target_link_libraries(core PRIVATE 
        Shcore.lib
    )
elseif(UNIX)
    target_link_libraries(core PRIVATE 
        thirdparty::X11_wrapper
    )
endif()