# C3 Setup Guide

This guide provides complete installation and configuration instructions for C3.

## Server Setup

### 1. Install Dependencies

Install [Node.js](https://nodejs.org/) (version 16 or later).

```bash
cd server
npm install
```

### 2. Configure Authentication

**Option A: Environment Variables**
```bash
cp .env.example .env
```
Edit `.env` with your settings:
```env
JWT_SECRET=your-secret-key
ADMIN_USERNAME=admin
ADMIN_PASSWORD=your-password
SESSION_EXPIRE_HOURS=24
```

**Option B: Interactive Setup**
```bash
node setup-auth.js
```
Follow prompts to set up admin credentials and JWT secret.

### 3. Start Server

```bash
node server.js
```

Server will run on:
- **HTTP Server**: Port 3000 (web interface)
- **WebSocket Server**: Same port (real-time communication)

Access the web interface at `http://localhost:3000`

## Client Setup

### Traditional Configuration

Create `config.json` next to the executable:

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

#### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `hostname` | string | `127.0.0.1` | Server hostname or IP address |
| `port` | number | `3000` | Server port number |
| `use_ssl` | boolean | `false` | Enable HTTPS/WSS encryption |
| `skip_ssl_verification` | boolean | `false` | Skip SSL certificate validation ⚠️ |
| `interval_seconds` | number | `60` | Screenshot capture interval |
| `max_retries` | number | `3` | Maximum upload retry attempts |
| `retry_delay_ms` | number | `1000` | Delay between retries |
| `add_to_startup` | boolean | `false` | Add to system startup |
| `client_id` | string | `""` | Unique client ID (auto-generated) |

⚠️ **Security Warning**: Never use `skip_ssl_verification: true` in production environments.


### Hardcoded Configuration
See [build guide](BUILD.md) for secure deployment without config files.


## Running the Client

1. Ensure server is running
2. Place `config.json` next to executable (if using traditional config)
3. Run the C3 client:
   ```bash
   ./C3
   ```
4. Check server terminal for connection logs