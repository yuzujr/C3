find_package(X11 REQUIRED)

add_library(X11_wrapper INTERFACE)
target_link_libraries(X11_wrapper INTERFACE 
    X11::X11
    X11::Xinerama
)

add_library(thirdparty::X11_wrapper ALIAS X11_wrapper)