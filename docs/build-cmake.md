## Client build

### Xmake Recommended
xmake is a modern and efficient C/C++ build system that automatically downloads dependencies and builds the C3 project at blazing fast speed.

[-> Getting started with xmake](./build-xmake.md)

### Prerequisites

- **CMake** 3.16 or later - [Download](https://cmake.org/download/)
- **C++20 Compatible Compiler**:
  - Windows: MSVC (Visual Studio 2019+)
  - Linux: GCC 10+ or Clang 12+
- **System libraries** (Ubuntu/Debian):
  ```bash
  sudo apt install libx11-dev libxinerama-dev libssl-dev libcurl4-openssl-dev
  ```

---

### Third-party Libraries

The following libraries are automatically downloaded and built by CMake:

- **cpr** - HTTP client library
- **libjpeg-turbo** - High-performance JPEG codec
- **nlohmann_json** - JSON library for Modern C++
- **spdlog** - Fast C++ logging library
- **IXWebSocket** - WebSocket and HTTP client library
- **googletest** - C++ testing framework

> **Network Requirements**: Internet connection required during first build for dependency download. Configure proxy settings if needed:
> ```bash
> export http_proxy=http://proxy.company.com:8080
> export https_proxy=https://proxy.company.com:8080
> ```

## Standard Build

Build and run the normal (non–hardcoded) binary:

1. **Configure**
```bash
mkdir build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

**CMake Configuration Options**

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Debug` | Build configuration (Debug/Release) |
| `USE_HARDCODED_CONFIG` | `OFF` | Enable hardcoded configuration |
| `USE_WIN32_GUI` | `OFF` | Windows GUI mode (no console) |
| `BUILD_TESTS` | `ON` | Build unit tests |


2. **Build**
```bash
cmake --build build --config Release -j$(nproc)
```

3. **Test**
```bash
cd build
ctest
```

## Hardcoded Configuration Build

Embed all runtime configuration into the binary (no external config files). **Before configuring**, you must generate the header file:

1. **Edit Build Presets**

   `config/build-presets.json`

2. **Generate header**

   * **Linux/macOS**:

     ```bash
     ./scripts/generate-hardcoded-config.sh -p development
     ```
   * **Windows PowerShell**:

     ```powershell
     .\scripts\generate-hardcoded-config.ps1 -p development
     ```

   This will produce (for example) `src/core/include/core/HardcodedConfig.h` with all your settings.

3. **Configure**

   ```bash
   cmake -S . -B build-hardcoded -DUSE_HARDCODED_CONFIG=ON -DCMAKE_BUILD_TYPE=Release
   ```

4. **Build**

   ```bash
   cmake --build build-hardcoded --config Release -j
   ```