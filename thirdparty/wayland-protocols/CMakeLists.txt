# thirdparty/wayland-protocols/CMakeLists.txt
# Generate and compile Wayland protocol sources using wayland-scanner

find_program(WAYLAND_SCANNER wayland-scanner)
if(NOT WAYLAND_SCANNER)
    message(FATAL_ERROR "wayland-scanner not found. Install wayland-dev or wayland-protocols.")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)

set(PROTOCOL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protocols)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})

# List of protocol XML files to generate
set(PROTOCOL_XMLS
    ${PROTOCOL_DIR}/wlr-screencopy-unstable-v1.xml
    ${PROTOCOL_DIR}/xdg-output-unstable-v1.xml
)

set(PROTOCOL_SOURCES "")
set(PROTOCOL_HEADERS "")

foreach(PROTO_XML ${PROTOCOL_XMLS})
    get_filename_component(PROTO_NAME ${PROTO_XML} NAME_WE)

    set(PROTO_HEADER ${GENERATED_DIR}/${PROTO_NAME}-client-protocol.h)
    set(PROTO_SOURCE ${GENERATED_DIR}/${PROTO_NAME}-protocol.c)

    add_custom_command(
        OUTPUT ${PROTO_HEADER}
        COMMAND ${WAYLAND_SCANNER} client-header ${PROTO_XML} ${PROTO_HEADER}
        DEPENDS ${PROTO_XML}
        COMMENT "Generating ${PROTO_NAME} client header"
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${PROTO_SOURCE}
        COMMAND ${WAYLAND_SCANNER} private-code ${PROTO_XML} ${PROTO_SOURCE}
        DEPENDS ${PROTO_XML}
        COMMENT "Generating ${PROTO_NAME} protocol source"
        VERBATIM
    )

    list(APPEND PROTOCOL_HEADERS ${PROTO_HEADER})
    list(APPEND PROTOCOL_SOURCES ${PROTO_SOURCE})
endforeach()

add_library(wayland_protocols STATIC ${PROTOCOL_SOURCES} ${PROTOCOL_HEADERS})
target_include_directories(wayland_protocols PUBLIC
    ${GENERATED_DIR}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
)
target_link_libraries(wayland_protocols PUBLIC ${WAYLAND_CLIENT_LIBRARIES})

# Suppress warnings for generated C code
target_compile_options(wayland_protocols PRIVATE -w)

add_library(thirdparty::wayland_protocols ALIAS wayland_protocols)
