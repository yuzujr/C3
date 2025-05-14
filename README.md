# ScreenUploader

ScreenUploader is a tool that periodically uploads the client's screen to the server. The uploaded images are encoded in JPEG format and stored on the server organized by time; log files are handled in the same way.

The client is implemented in C++, while the server uses the JavaScript Express framework. It supports both Windows and Linux (X11).

Mainly used for practicing server/client projects.



## Usage
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/yuzujr/ScreenUploader)
1. Run server

   **Dependencies**

   Install [node.js](https://nodejs.org/).

   `cd server`

   `npm install`

   `node server.js`

   Server will run on port `4000` by default.

2. Edit `config.json`:

- `server_url` is the address where the server running. (If you run this server on **loop back address**, `server_url` will be `http://127.0.0.1:4000`)
- `interval_seconds` is the interval that client send upload request. (60s by default)
- `max_retries` is the number of retry attempts if upload fails. (3 by default)
- `retry_delay_ms` is the delay between retry attempts in milliseconds. (1000ms by default)
- `add_to_startup` determines whether to add the application to system startup. (false by default)

Example `config.json`:
```json
{
  "api": {
    "server_url": "http://127.0.0.1:4000",
    "interval_seconds": 60,
    "max_retries": 3,
    "retry_delay_ms": 1000,
    "add_to_startup": false
  }
}
```

3. Move `config.json` to where the `ScreenUploader` executable is.

4. Run `ScreenUploader` executable.

5. Check the server terminal, you will see the upload logs.
   Screenshots is saved in `uploads` folder, and logs are saved in `logs` folder.



## Build from source

### Client

For windows, msvc compiler is recommended.

For Linux, gcc compiler is recommended.

**Dependencies**

1. Install [cmake](https://cmake.org/download/) (version `3.16` minimum required).

2. For windows, install [OpenCV](https://opencv.org/releases/).

3. For linux, install `libopencv`, `libx11`, `libssl`, `libcurl`.

   Ubuntu:

   `sudo apt install libopencv-dev`
   
   `sudo apt install libx11-dev`
   
   `sudo apt install libssl-dev`
   
   `sudo apt install libcurl4-openssl-dev`

**Build**

1. `mkdir build && cd build`

2. `cmake .. -DCMAKE_BUILD_TYPE=Release`
   > For windows, you can pass '-DUSE_WIN32_GUI=ON' to avoid console window.

3. `cmake --build . --config Release`
