# module: app
add_library(app STATIC)


# source files
target_sources(app PRIVATE
    C3App.cpp
)


# enable strict compiler warnings
target_compile_options(app PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/WX>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Werror>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Werror>
)


# include directories PUBLIC
target_include_directories(app PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# link other modules PUBLIC
target_link_libraries(app PUBLIC
    core
    net
)