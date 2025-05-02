cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -S . -B build
cmake --build build --config Release --target ScreenUploader
cp F:\libs\opencv\build-no-world\x64\vc17\bin\opencv_core4100.dll F:\libs\opencv\build-no-world\x64\vc17\bin\opencv_imgcodecs4100.dll F:\libs\opencv\build-no-world\x64\vc17\bin\opencv_imgproc4100.dll build\Release