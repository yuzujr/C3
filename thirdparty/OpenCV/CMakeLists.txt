find_package(OpenCV REQUIRED)

add_library(OpenCV_wrapper INTERFACE)

target_link_libraries(OpenCV_wrapper INTERFACE
    opencv_core
    opencv_imgproc
    opencv_imgcodecs
)

add_library(thirdparty::OpenCV_wrapper ALIAS OpenCV_wrapper)