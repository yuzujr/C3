cmake -S . -B build-lsp -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_TOOLCHAIN_FILE=F:/vcpkg/scripts/buildsystems/vcpkg.cmake
copy build-lsp\compile_commands.json .