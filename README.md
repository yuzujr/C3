# C3

C3 (Command-Control-C++) is a lightweight command and control framework for remote system administration.

**Tech Stack**: 
- C++ client
- Go server
- PostgreSQL database
- Web frontend

## Server
The server code is maintained in a separate repository: [C3-server](https://github.com/yuzujr/C3-server).
> **Note:** Versions prior to **v1.9.0** used a Node.js–based implementation, which is now officially deprecated. [Legacy releases](https://github.com/yuzujr/C3/releases) remain available for download if you need them.

## Features

- **Web frontend**: A pretty web interface that provides:
  - Multi-user management
  - Sending real-time commands
  - Viewing screenshots (supports multiple monitors)
  - A reverse shell based on xterm.js (frontend), WebSocket (communication), and pty (backend), offering millisecond-level responsiveness and a native experience

- **WebSocket Communication**: Enables real-time interaction with clients.
- **Database Integration**: Uses PostgreSQL for persistent storage.
- **Environment Configuration**: Uses `.env` files for easy configuration management.

## Quick Start

1. **Get Client**  
   - Download from [Releases](https://github.com/yuzujr/C3/releases)  
   - Build from source:  
     - [CMake guide](docs/build-cmake.md)  
     - [Xmake guide](docs/build-xmake.md)
2. **Setup**: Follow the [Setup Guide](docs/setup.md)

## Legal

**⚠️ DISCLAIMER: Educational and Research Use Only**

This software is provided for educational, research, and authorized system administration purposes only. Users are solely responsible for ensuring compliance with all applicable laws and regulations in their jurisdiction.

**Prohibited Uses:**
- Unauthorized access to computer systems
- Any illegal or malicious activities
- Violation of computer crime laws

**User Responsibilities:**
- Obtain proper authorization before deployment
- Comply with all applicable laws and regulations
- Use only on systems you own or have explicit permission to access
- Respect privacy and data protection laws

The developers assume no liability for misuse of this software.
