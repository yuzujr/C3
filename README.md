# ScreenUploader

ScreenUploader is a comprehensive remote administration platform that combines automated screenshot capture with powerful reverse shell capabilities. The system enables real-time remote monitoring and control through a secure web interface.

The client is implemented in C++ with cross-platform support, while the server uses the JavaScript Express framework with WebSocket integration. It supports both Windows and Linux (X11) with enterprise-grade security features.

**Key Features:**
- **Automated Screenshot Capture**: Periodic screen monitoring with JPEG compression
- **Reverse Shell System**: Full terminal access with persistent sessions
- **Enterprise Security**: JWT authentication with role-based access control
- **Real-Time Web Interface**: WebSocket-powered control panel
- **Flexible Deployment**: Traditional config files or hardcoded builds
- **Cross-Platform**: Windows (PowerShell) and Linux (Bash) support

Primarily built for practicing advanced client/server architecture, real-time communication, and secure remote administration.



## Usage
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/yuzujr/ScreenUploader)

### Server Setup

1. **Install Dependencies**

   Install [node.js](https://nodejs.org/).

   ```bash
   cd server
   npm install
   ```

2. **Configure Authentication (Optional)**

   1. environment method:
   ```bash
   cp .env.example .env
   ```
   Edit .env with your settings.
   
   2. config file method:
   ```bash
   node setup-auth.js
   ```
   Follow prompts to set up admin credentials and JWT secret.

3. **Start Server**

   ```bash
   node server.js
   ```

   Server runs on:
   - **HTTP Server**: Port 4000 (web interface)
   - **WebSocket Server**: Port 8080 (real-time communication)

### Client Configuration

**Option 1: Traditional Config File**

Edit `config.json`:

- `upload_url` is server's upload endpoint. (Default: `http://127.0.0.1:4000`)
- `ws_url` is server's WebSocket endpoint. (Default: `ws://127.0.0.1:8080`)
- `interval_seconds` is the screenshot capture interval. (Default: 60s)
- `max_retries` is the number of retry attempts if upload fails. (Default: 3)
- `retry_delay_ms` is the delay between retry attempts in milliseconds. (Default: 1000ms)
- `add_to_startup` determines whether to add the application to system startup. (Default: false)
- `client_id` is a unique identifier for the client (optional, auto-generated if not set).

Example `config.json`:
```json
{
  "api": {
    "server_url": "http://127.0.0.1:4000",
    "ws_url": "ws://127.0.0.1:8080",
    "interval_seconds": 60,
    "max_retries": 3,
    "retry_delay_ms": 1000,
    "add_to_startup": false,
    "client_id": ""
  }
}
```

**Option 2: Hardcoded Build**

For secure deployment without config files:
```bash
# Edit presets as needed: config/build-presets.json

# Generate hardcoded configuration
./scripts/generate-hardcoded-config.sh
# Or on Windows: ./scripts/generate-hardcoded-config.ps1

# Build with hardcoded config
mkdir build-hardcoded
cmake -S . -B build-hardcoded -DUSE_HARDCODED_CONFIG=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-hardcoded --config Release -j
```

### Running the Client

1. Place `config.json` next to the executable (if using traditional config)
2. Run `ScreenUploader` executable
3. Check server terminal for connection logs

### Web Interface

Access the control panel at `http://localhost:4000`:

- **Login**: Use credentials set up during server configuration
- **Screenshot Monitoring**: Real-time screenshot viewing and capture
- **Terminal Access**: Full reverse shell with persistent sessions
- **Client Management**: Monitor connected clients and their status, rename/delete clients

Features:
- **Instant Screenshots**: On-demand screen capture
- **Terminal Sessions**: Interactive command execution
- **Client Dashboard**: Real-time status monitoring
- **Auto-Refresh**: Live updates via WebSocket


### File Organization

- **Screenshots**: Saved in `uploads/` folder organized by client
- **Logs**: Server logs saved in `logs/` folder  
- **Client Config**: Stored as `uploads/{client_id}/config.json`
- **Authentication**: JWT tokens and session data


## Build from Source

### Client Dependencies

**Required Tools:**
- [CMake](https://cmake.org/download/) (version 3.16+)
- C++20 compatible compiler:
  - Windows: MSVC (Visual Studio 2019+)
  - Linux: GCC 10+ or Clang 12+

**Linux System Dependencies:**
```bash
# Ubuntu/Debian
sudo apt install libx11-dev libssl-dev libcurl4-openssl-dev

# CentOS/RHEL
sudo yum install libX11-devel openssl-devel libcurl-devel
```

**Third-party Libraries (Auto-downloaded):**
- **cpr**: HTTP client library
- **libjpeg-turbo**: JPEG compression
- **nlohmann_json**: JSON parsing  
- **spdlog**: Logging framework
- **IXWebSocket**: WebSocket client

### Build Process

**Standard Build:**
```bash
mkdir build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

**Windows GUI Build(avoid console):**
```bash
mkdir build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_WIN32_GUI=ON
cmake --build build --config Release -j
```

**Hardcoded Configuration Build:**
```bash
mkdir build-hardcoded
cmake -S . -B build-hardcoded -DUSE_HARDCODED_CONFIG=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-hardcoded --config Release -j
```

> **Network Requirements**: Internet connection required for dependency download. Configure `http_proxy` and `https_proxy` if behind a proxy.