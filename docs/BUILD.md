# C3 Build Instructions

This document provides detailed compilation and deployment instructions for C3 client.

## Prerequisites

### Required Tools

- **CMake** 3.16 or later - [Download](https://cmake.org/download/)
- **C++20 Compatible Compiler**:
  - Windows: MSVC (Visual Studio 2019+)
  - Linux: GCC 10+ or Clang 12+

### System Dependencies

**Linux (Ubuntu/Debian)**
```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libx11-dev libssl-dev libcurl4-openssl-dev
```

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


1. **Configure Build**
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


2. **Compile**
```bash
cmake --build build --config Release -j$(nproc)
```

3. **Run**
```bash
./build/src/C3
```


## Hardcoded Configuration Build

For secure deployment without config files:

1. **Edit Build Presets**
   ```bash
   vim config/build-presets.json
   ```

2. **Generate Hardcoded Config**
```bash
# Windows
.\scripts\generate-hardcoded-config.ps1 -p development

# Linux
./scripts/generate-hardcoded-config.sh -p development
```

3. **Build with Hardcoded Config**
   ```bash
   mkdir build-hardcoded
   cmake -S . -B build-hardcoded -DUSE_HARDCODED_CONFIG=ON -DCMAKE_BUILD_TYPE=Release
   cmake --build build-hardcoded --config Release -j
   ```
