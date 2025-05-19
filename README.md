# ScreenUploader

ScreenUploader is a tool that periodically uploads the client's screen to the server. The uploaded images are encoded in JPEG format and stored on the server organized by time.

The client is implemented in C++, while the server uses the JavaScript Express framework. It supports both Windows and Linux (X11).

Primarily built for practicing client/server architecture and remote device coordination.



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

- `server_url` is the address where the server running. (`http://127.0.0.1:4000` by default)
- `interval_seconds` is the interval that client send upload request. (60s by default)
- `max_retries` is the number of retry attempts if upload fails. (3 by default)
- `retry_delay_ms` is the delay between retry attempts in milliseconds. (1000ms by default)
- `add_to_startup` determines whether to add the application to system startup. (false by default)
- `client_id` is the unique identifier for the client. (automatically filled on first run)

   Example `config.json`:
   ```json
   {
     "api": {
       "server_url": "http://127.0.0.1:4000",
       "interval_seconds": 60,
       "max_retries": 3,
       "retry_delay_ms": 1000,
       "add_to_startup": false,
       "client_id": ""
     }
   }
   ```

3. Place this `config.json` next to the executable.

4. Run `ScreenUploader` executable.

5. Check the server terminal, you will see the upload logs.
   Screenshots is saved in `uploads` folder, and logs are saved in `logs` folder.
   Client's config is saved in `clients_config.json`.

6. (Optional) Edit `clients.json` to alias the client ID to a name. This is useful for identifying clients in the server logs.

   Example `clients.json`:
   ```json
   {
     "client_id": "client_name"
   }
   ```

7. (Optional) Edit `commandQueue.json` to send commands for the client.
   Commands available:
   - `screenshot_now`: Take a screenshot and upload it to the server.
   - `update_config`: Update the client's configuration file.
   - `pause`: Pause the client.
   - `resume`: Resume the client.

   Example `commandQueue.json`:
   ```json
   {
     "client-ozy": [
       {
         "type": "update_config",
         "data": {
           "api": {
             "interval_seconds": 5,
             "max_retries": 5,
             "retry_delay_ms": 1500,
             "add_to_startup": true
           }
         }
       },
       {
         "type": "pause"
       },
       {
         "type": "screenshot_now"
       },
       {
         "type": "resume"
       }
     ]
   }
   ```
   
   Client polls for instructions, and the server returns all instructions for that client.
   In this example,
   "client-ozy" is the client ID (or alias name defined in `clients.json`), and the instructions are in the form of a list.
   The client will execute the instructions in the order they are received. For example, it will first update the configuration, then    pause, take a screenshot, and finally resume.
   Be careful with changing `server_url` and `client_id` in the config, as it may cause the client to lose connection with the server.



## Build from source

### Client

For windows, msvc compiler is recommended.

For Linux, gcc compiler is recommended.

**Dependencies**

1. Install [cmake](https://cmake.org/download/) (version `3.16` minimum required).

2. For windows, install [OpenCV](https://opencv.org/releases/).

3. For linux, install `libopencv`, `libx11`, `libssl`, `libcurl`.

   Ubuntu:

   `sudo apt install libopencv-dev libx11-dev libssl-dev libcurl4-openssl-dev`

**Build**

1. `mkdir build && cd build`

2. `cmake .. -DCMAKE_BUILD_TYPE=Release`
   
   > For windows, you can pass '-DUSE_WIN32_GUI=ON' to avoid console window.
   
3. `cmake --build . --config Release`
