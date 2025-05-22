ExternalProject_Add(
  libjpeg_turbo
  GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
  GIT_TAG 3.1.0
  PREFIX ${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo
  CMAKE_ARGS 
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install
    -DENABLE_SHARED=OFF
    -DENABLE_STATIC=ON
    -DCMAKE_BUILD_TYPE=Release
  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  INSTALL_COMMAND cmake --build . --target install
)

add_library(jpeg STATIC IMPORTED GLOBAL)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install/include)

if(MSVC)
  set(TURBOJPEG_LIB_NAME "turbojpeg-static")
else()
  set(TURBOJPEG_LIB_NAME "turbojpeg")
endif()

set_target_properties(jpeg PROPERTIES
  IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${TURBOJPEG_LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX}"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install/include"
)

add_dependencies(jpeg libjpeg_turbo)