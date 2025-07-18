# module: net
add_library(net STATIC)


# common source files
target_sources(net PRIVATE
    Uploader.cpp
    WebSocketClient.cpp
    URLBuilder.cpp
)


# enable strict compiler warnings
target_compile_options(net PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/WX>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Werror>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Werror>
)


# include directories PUBLIC
target_include_directories(net PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# link module core
target_link_libraries(net PRIVATE
    core
)

# link cpr nlohmann_json publicly
target_link_libraries(net PUBLIC
    cpr::cpr
    nlohmann_json::nlohmann_json
    ixwebsocket::ixwebsocket
)