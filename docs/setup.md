## Client Setup

### 1. Get Client
   - Download from [Releases](https://github.com/yuzujr/C3/releases)  
   - Build from source:  
     - [CMake guide](docs/build-cmake.md)  
     - [Xmake guide](docs/build-xmake.md)

### 2. Configure

Move `config/config.json` next to the executable:

```json
{
  "api": {
    "hostname": "127.0.0.1",
    "port": 3000,
    "base_path": "",
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

### 3. Run

## Configuration Reference

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `hostname` | string | `127.0.0.1` | Server hostname or IP |
| `port` | number | `3000` | Server port |
| `base_path` | string | `""` | Base URL path prefix (e.g., "/c3" for reverse proxy) |
| `use_ssl` | boolean | `false` | Enable HTTPS/WSS |
| `skip_ssl_verification` | boolean | `false` | Skip SSL verification ⚠️ |
| `interval_seconds` | number | `60` | Screenshot interval |
| `max_retries` | number | `3` | Upload retry attempts |
| `retry_delay_ms` | number | `1000` | Retry delay |
| `add_to_startup` | boolean | `false` | Add to system startup |
| `client_id` | string | `""` | Unique client ID |