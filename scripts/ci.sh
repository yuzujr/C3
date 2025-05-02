cmake -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -S . -B build
cmake --build build --config Release --target ScreenUploader
cp /root/ScreenUploader/config.json build