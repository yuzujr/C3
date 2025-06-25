# C3

C3 (Command-Control-C++) is a minimal command and control framework for remote system administration.

## Features

- Remote shell access via WebSocket
- Automated screenshot capture and upload
- HTTPS/WSS encryption with certificate validation options
- Web-based control interface with real-time communication
- Cross-platform client (Windows/Linux)
- Hardcoded configuration builds for deployment

## Quick Setup

**Server (Native):**
```bash
cd server && npm install && node app.js
```

**Server (Docker):**
```bash
cd server && docker-compose up -d
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


## Documentation

- [Build Instructions](docs/BUILD.md) - Compilation options and deployment
- [Setup Guide](docs/SETUP.md) - Complete installation and server configuration

## Legal

Educational use only. User assumes all responsibility for compliance with applicable laws.
