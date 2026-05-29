# ASCII Aquarium — Desktop Port

Native x86 Linux/Windows port of [POWER-PILL/ASCII-Aquarium][upstream],
intended for small PCs with small screens running as unattended kiosks.

See the top-level [README](../README.md) for fork context and credits.

## Status

Early port, in progress. Currently:

- ✅ CMake + SDL2 build skeleton
- ✅ Window + vsync'd main loop at 320×240, integer-scaled, with
  `--scale N`, `--fullscreen`, `--hide-cursor` flags and Escape / F11 keys
- ✅ Renderer primitives (`drawPixel`/`drawLine`/`drawRect`/`fillRect`/
  `drawRoundRect`/`fillRoundRect`/`fillScreen`/`pushImage`/`readPixel`) and
  the full TFT_eSPI RGB565 color palette
- ✅ Bitmap fonts vendored from TFT_eSPI (Font 1 GLCD 6×8 fixed-width,
  Font 2 16px proportional) with a TFT_eSPI-style text API
  (`setTextFont`/`setTextDatum`/`setTextColor`/`drawString`/`drawChar`/
  `textWidth`/`setCursor`/`println`)
- ✅ Aquarium core simulation (`src/sim/`): schooling fish with depth tint
  and swim-wave animation, rising bubbles, sinking food flakes, swaying
  seaweed, and the two occasional visitors (octopus, seahorse) — ported
  faithfully from the upstream sketch's update/draw math
- ⬜ Background modes (black, blue/purple gradient, flowers)
- ⬜ HUD + settings panels, on-screen keyboard
- ⬜ Settings persistence, clock, BMP capture, kiosk autostart docs

Right now if you build and run, the window opens at 640×480 (2x scale)
showing the live aquarium over a flat navy sea (real backgrounds land next):
fish school and avoid each other, bubbles rise, seaweed sways, and
**left-clicking drops a food flake** the fish swim toward and eat. The
octopus and seahorse appear on a timer (~once an hour by default). Escape
quits, F11 toggles fullscreen.

## Build

### Linux (primary target)

Install build deps (Debian/Ubuntu/Mint/Pop/etc.):

```bash
sudo apt install build-essential cmake libsdl2-dev
```

Configure and build:

```bash
cd desktop
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/ascii-aquarium
```

Fedora/RHEL: `sudo dnf install gcc-c++ cmake SDL2-devel`
Arch:        `sudo pacman -S base-devel cmake sdl2`
Alpine:      `sudo apk add build-base cmake sdl2-dev`

### Windows (secondary target)

Recommended: install SDL2 via [vcpkg][vcpkg]:

```powershell
vcpkg install sdl2:x64-windows
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
.\build\Release\ascii-aquarium.exe
```

Or download the SDL2 development libraries from [libsdl.org][sdl] and point
CMake at them with `-DCMAKE_PREFIX_PATH=C:/path/to/SDL2`.

## Layout

```
desktop/
  CMakeLists.txt
  README.md            <-- you are here
  src/
    main.cpp           SDL bootstrap + main loop (CLI args land here)
    renderer/          SDL2-backed pixel surface + draw primitives + fonts
    input/             SDL mouse / touch / keyboard -> tap events
    aquarium/          fish, bubbles, flakes, seaweed, octopus, seahorse
    ui/                HUD, settings panel, keyboard, capture panel, wifi
                       panel (kept for parity, no-op on desktop)
    config/            JSON-backed persistent settings (replaces Preferences)
    time/              system clock + 12/24h + ASCII clock
    capture/           BMP screenshot + frame-sequence writer
  assets/              vendored fonts and any other static data
```

Many of these directories start out empty (just `.gitkeep`); they fill in as
the port progresses. See the top-level task list for the order.

## Kiosk usage

Current CLI:

```
ascii-aquarium [--scale N] [--fullscreen] [--hide-cursor] [--help]
```

Runtime keys: `Escape` quits, `F11` toggles fullscreen.

Planned (later tasks): `--capture-dir PATH`, `--settings PATH`, plus a
systemd `--user` unit example for Linux autostart and a Startup folder /
Task Scheduler recipe for Windows.

[upstream]: https://github.com/POWER-PILL/ASCII-Aquarium
[vcpkg]: https://vcpkg.io/
[sdl]: https://github.com/libsdl-org/SDL/releases
