# C3

C3 (command-control-c++) is a minimal C++ command and control framework for remote system administration.

## Features

- Remote shell access via WebSocket
- Automated screenshot capture and upload
- HTTPS/WSS encryption with certificate validation options
- Web-based control interface with real-time communication
- Cross-platform client (Windows/Linux)
- Hardcoded configuration builds for deployment

## Quick Setup

**Server:**
```bash
cd server && npm install && node server.js
```

**Client:**
```bash
cmake -S . -B build && cmake --build build
./build/src/C3
```

## Configuration

Create `config.json`:
```json
{
  "api": {
    "hostname": "127.0.0.1",
    "port": 3000,
    "use_ssl": false,
    "skip_ssl_verification": false,
    "interval_seconds": 60,
    "max_retries": 3,
    "retry_delay_ms": 1000,
    "add_to_startup": false,
    "client_id": ""
  }
}
```


## Build Options

- `USE_HARDCODED_CONFIG=ON` - Embed configuration at compile time
- `BUILD_TESTS=OFF` - Skip test compilation
- `USE_WIN32_GUI=ON` - Build as Windows GUI application

## Hardcoded Builds

Generate hardcoded configuration:
```bash
# Windows
powershell scripts/generate-hardcoded-config.ps1 -preset production

# Linux/macOS  
./scripts/generate-hardcoded-config.sh -p production
```

Then build with hardcoded config:
```bash
cmake -S . -B build-hardcoded -DUSE_HARDCODED_CONFIG=ON
cmake --build build-hardcoded --config Release
```

## Documentation

- [Build Instructions](docs/BUILD.md) - Compilation options and deployment
- [Setup Guide](docs/SETUP.md) - Complete installation and server configuration

## Legal

Educational use only. User assumes all responsibility for compliance with applicable laws.
