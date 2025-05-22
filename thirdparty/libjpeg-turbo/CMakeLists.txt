ExternalProject_Add(
  libjpeg_turbo
  GIT_REPOSITORY https://github.com/libjpeg-turbo/libjpeg-turbo.git
  GIT_TAG 3.1.0
  PREFIX ${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo
  CMAKE_ARGS 
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install
    -DENABLE_SHARED=ON           # 生成动态库
    -DENABLE_STATIC=OFF          # 静态库
    -DENABLE_TEST=OFF
    -DENABLE_DOC=OFF
  UPDATE_COMMAND ""
  PATCH_COMMAND ""
  INSTALL_COMMAND cmake --build . --target install
)

add_library(jpeg SHARED IMPORTED GLOBAL)

set_target_properties(jpeg PROPERTIES
  IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install/bin/turbojpeg.dll"
  IMPORTED_IMPLIB "${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install/lib/jpeg.lib"
  IMPORTED_IMPLIB "${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install/lib/turbojpeg.lib"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/_deps/libjpeg_turbo/install/include"
)

add_dependencies(jpeg libjpeg_turbo)