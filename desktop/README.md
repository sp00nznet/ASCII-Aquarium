# ASCII Aquarium — Desktop Port

Native x86 Linux/Windows port of [POWER-PILL/ASCII-Aquarium][upstream],
intended for small PCs with small screens running as unattended kiosks.

See the top-level [README](../README.md) for fork context and credits.

## Status

Early scaffold. Only the build skeleton is wired up so far — running it just
prints SDL version info and exits. Tracking progress in the task list at the
top of this fork.

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

## Kiosk usage (planned)

Will land in a later task. Sketch of the planned CLI:

```
ascii-aquarium [--fullscreen] [--scale N] [--width W --height H]
               [--hide-cursor] [--capture-dir PATH] [--settings PATH]
```

Linux autostart will use a systemd `--user` unit; Windows will use the
Startup folder or Task Scheduler. Docs land alongside the kiosk-polish task.

[upstream]: https://github.com/POWER-PILL/ASCII-Aquarium
[vcpkg]: https://vcpkg.io/
[sdl]: https://github.com/libsdl-org/SDL/releases
