## Client build

## Prerequisites

- **xmake** - [Download](https://xmake.io/guide/quick-start.html)
- **C++20-compatible compiler**  
  - Windows: MSVC (Visual Studio 2019+)  
  - Linux: GCC 10+ or Clang 12+  
- **System libraries** (Ubuntu/Debian):  
  ```bash
  sudo apt install libx11-dev libxinerama-dev
  ```

---

## Standard Build

Build and run the normal (non–hardcoded) binary:

1. **Configure**

   ```bash
   xmake f -m release --gui=n --hardcode_config=n
   ```

   > If prompted to download dependencies, type `y` to agree.

## Xmake Configuration Options

| Option              | Default | Description                                                |
| ------------------- | ------- | ---------------------------------------------------------- |
| `-m, --mode`        | release   | Build mode: `debug` or `release`                           |
| `--gui`             | n       | Windows GUI mode (`y` = no console, `n` = console enabled) |
| `--hardcode_config` | n       | Embed runtime config into the binary                       |


2. **Build**

   ```bash
   xmake
   ```

3. **Test**

   ```bash
   xmake test
   ```

---

## Hardcoded Configuration Build

Embed all runtime configuration into the binary (no external config files). **Before configuring**, you must generate the header file:

1. **Edit Build Presets**

   `config/build-presets.json`

2. **Generate header**

   * **Linux/macOS**:

     ```bash
     ./scripts/generate-hardcoded-config.sh -p development
     ```
   * **Windows PowerShell**:

     ```powershell
     .\scripts\generate-hardcoded-config.ps1 -p development
     ```

   This will produce (for example) `src/core/include/core/HardcodedConfig.h` with all your settings.

3. **Configure**

   ```bash
   # --hardcode_config=y : embed config into the binary  
   xmake f -m release -o build-xmake --hardcode_config=y
   ```

4. **Build**

   ```bash
   xmake
   ```

5. **Test**

   ```bash
   xmake test
   ```
