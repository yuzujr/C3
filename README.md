# ScreenUploader

ScreenUploader is a tool that periodically uploads the client's screen to the server. The uploaded images are encoded in JPEG format and stored on the server organized by time; log files are handled in the same way.

The client is implemented in C++, while the server uses the JavaScript Express framework. It supports both Windows and Linux (X11).

Mainly used for practicing server/client projects.



## Usage

1. Run server

   **Dependencies**

   Install [node.js](https://nodejs.org/).

   `cd server`

   `npm install`

   `node server.js`

   Server will run on port `4000` by default.

2. Edit `config.json`:

- `upload_url` is server's api url. (If you run this server on **loop back address**, `upload_url` will be `http://127.0.0.1:4000/upload`)
- `interval_seconds` is the interval that client send upload request. (60s by default)

3. Move `config.json` to where the `ScreenUploader` executable is.

4. Run `ScreenUploader` executable.
5. Check the server terminal, you will see the upload logs.



## Build from source

#### Client

For windows, msvc compiler is recommended.

For Linux, gcc compiler is recommended.

**Dependencies**

1. Install [cmake](https://cmake.org/download/), [vcpkg](https://github.com/microsoft/vcpkg) and [opencv](https://opencv.org/releases/).

2. For linux, install `libx11-dev`.

   Ubuntu:`sudo apt install libx11-dev`

`mkdir build && cd build`

`cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake`

`cmake --build . --config Release`

> On Windows, WIN32 is enabled by default(CMakeLists.txt:24), delete `WIN32` to see the program console.
