# C3

C3 (Command-Control-C++) is a minimal command and control framework for remote system administration.

**Tech Stack**: C++ client, Node.js server, PostgreSQL database, Web frontend

## Go Server Rewrite Progress

The Node.js server is currently being rewritten in Go to improve performance, simplify deployment, and enhance overall stability.

### ✅ Completed

* Basic client upload API implemented

### 🔜 Upcoming

* Implement WebSocket communication with clients
* Connect and sync with the web frontend

## Features

- Reverse shell on WebSocket (xterm.js + native pty)
- Automated screenshot capture
- Web-based muti-client management interface
- Cross-platform client support

## Prerequisites

- **Server**: Node.js, PostgreSQL
- **Client**: CMake 3.16+, C++20 compiler
- **Platform**: Unix, Windows

## Quick Start

1. **Get Client**: Download from [Releases](https://github.com/yuzujr/C3/releases) or [Build from Source](docs/BUILD.md)
2. **Setup**: Follow the [Setup Guide](docs/SETUP.md)

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
