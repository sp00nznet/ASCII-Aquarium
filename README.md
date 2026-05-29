# ASCII Aquarium — Desktop Port (fork)

> **This is a fork of [POWER-PILL/ASCII-Aquarium](https://github.com/POWER-PILL/ASCII-Aquarium).**
> All of the original ESP32 / Cheap Yellow Display work, art, animation
> behavior, and feature design is by **POWER-PILL** and contributors. Every
> swimming punctuation mark, the tap-to-feed flakes, the visiting octopus and
> seahorse, the settings UI, the ASCII clock — none of that would exist
> without their effort, and full credit for it stays with them. Please go give
> the upstream repo a ⭐ and check out their [Hackaday write-up](https://hackaday.com/2026/05/24/adorable-ascii-aquarium-lives-on-your-desk).
>
> The upstream README (kept intact below) describes the firmware as it ships
> on the CYD board. This fork's README adds the section directly below.

## Why this fork exists

The original ASCII Aquarium is a firmware image for a specific piece of
hardware — the ESP32-2432S028R "Cheap Yellow Display." It is wonderful on
that board and you should buy one. But a lot of us have small x86 PCs
attached to small screens (mini-PCs, old netbooks, repurposed POS terminals,
tablets running Linux, single-board x86 boxes) where running an ESP32-only
build is not an option.

This fork ports the aquarium to run as a native desktop application on
**Linux and Windows on x86**, so the same tank can live on those tiny
screens too.

## Downloads

Prebuilt **Linux / Windows / macOS** binaries — built by CI on every push to
`main` — are at **<https://sp00.nz/releases/ASCII-Aquarium/>** (grab the `main/`
folder, or a tagged release). Or build from source — see
[`desktop/README.md`](desktop/README.md).

## What this fork adds

A native desktop build of the aquarium lives under [`desktop/`](desktop/) — a
C++/SDL2 rewrite (no Arduino/ESP32) that runs on x86 **Linux, Windows, and
macOS**. It reproduces the device's look and behavior, then layers on extras
that suit an always-on desktop or kiosk display.

**A faithful port of the device** — the upstream sketch is treated as the
behavioral spec:

- The full simulation: schooling fish (wander / alignment / cohesion /
  separation) with depth tinting and the per-character swim wave, rising
  bubbles, sinking food flakes, swaying multi-segment seaweed, and the visiting
  octopus and seahorse — same math, same magic numbers.
- A small RGB565 renderer reimplementing the slice of TFT_eSPI the sketch uses,
  plus the vendored Font 1 / Font 2 bitmap glyphs, drawn into a 320×240 canvas.
- The on-screen UI: HUD, tap-to-feed, the tabbed Settings panel and the clock
  style/colour pop-overs.
- All four backgrounds (black, dithered blue/purple gradient, pixel flowers)
  and both clock styles (small text + big ASCII art).

**Desktop replacements for the hardware bits:**

- **Display** → SDL2 at the native 320×240 with `--scale N` integer scaling and
  `--fullscreen`; DPI-aware so it stays crisp on scaled displays.
- **Touch (XPT2046)** → SDL mouse **and** touchscreen events, debounced, with
  optional cursor hide.
- **Settings (ESP32 NVS)** → a `config.ini` in the platform config dir,
  debounce-autosaved and restored on launch.
- **SD-card BMP capture** → screenshots and frame sequences to a folder
  (`--capture-dir`), with sequences auto-encoded to **mp4/gif** via ffmpeg when
  it's available.
- **Wi-Fi + NTP clock** → the host system clock. The device's Wi-Fi / NTP /
  on-screen-keyboard flow is dropped — a desktop already knows the time.

**New on desktop, beyond the device:**

- **Day/night cycle** — ambient light/tint follows the OS clock — plus drifting
  caustic light rays.
- **More life** — a sand-scuttling crab, a drifting jellyfish, a rare shark
  fly-through that scatters the school, and sand props (sandcastle, a bubbling
  treasure chest, coral, rock).
- **Fish hunger** — fish fill up when fed, mellow when sated, and graze the
  weeds when starving.
- **Kiosk care** — LCD burn-in protection (slow whole-frame drift) and an
  optional background auto-cycle — all toggled and persisted in a **Scene**
  settings tab.

**Build & ship:**

- One-command CMake build: Linux uses the system SDL2; Windows/macOS fetch and
  static-link SDL2 into a standalone binary (no vcpkg, no loose DLLs).
- A **GitLab CI/CD pipeline** (Linux / Windows / macOS shell runners) that
  builds all three and drops the binaries on a release share.
- Kiosk autostart docs (systemd user unit / `cage` on Linux; Startup folder /
  Task Scheduler on Windows).

## What this fork does differently from upstream

- **Target**: x86 Linux and Windows desktops, not the ESP32 CYD board.
- **Firmware artifacts** (`.bin` files under `docs/firmware/`, the web
  flasher under `docs/`, the Arduino sketch under `ASCII_Aquarium_CYD/`,
  and the `User_Setup*.h` TFT_eSPI configs) are **kept in this repo
  unchanged** so this fork can continue to track upstream releases. The
  desktop port lives in its own `desktop/` subtree and does not modify any
  of those files.
- The desktop binary is intended to be visually faithful to the device but
  is not a bit-exact emulator — it uses the host's clock, host's storage,
  and host's input system.

## Status

Feature-complete and cross-platform (Linux / Windows / macOS), built in CI.
See [`desktop/README.md`](desktop/README.md) for build/run instructions, the
full keyboard-shortcut list, and [`desktop/docs/KIOSK.md`](desktop/docs/KIOSK.md)
for unattended/kiosk setup.

## Upstream sync

```bash
git remote add upstream https://github.com/POWER-PILL/ASCII-Aquarium.git  # one time
git fetch upstream
git merge upstream/main
```

## Credits

- **[POWER-PILL](https://github.com/POWER-PILL)** and ASCII Aquarium
  contributors — original firmware, simulation, art, UI, and the whole
  project concept. This fork is downstream of, and dependent on, their work.
- **[TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)** by Bodmer — the
  graphics library the upstream firmware draws through. The desktop port
  reimplements a subset of its API on top of SDL2; the bitmap fonts used by
  the desktop renderer are derived from TFT_eSPI's built-in glyph tables.
- **[XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)**
  by Paul Stoffregen — touch driver used upstream; mapped to SDL events
  here.
- **[SDL2](https://www.libsdl.org/)** — windowing, input, and the pixel
  surface the desktop port pushes its frames to.
- The 3D-printable cases linked in the upstream README are by their
  respective creators (PowerPill.Prints, annaglyph) and are not affected by
  this fork.

If anything in this fork misattributes work, please open an issue and it
will be fixed.

---

# Upstream README (POWER-PILL/ASCII-Aquarium)

> The text from here on is the upstream README, kept verbatim for credit and
> hardware-build reference.

## Meet the ASCII Aquarium ><(((°> 
<table>
  <tr>
    <td width="45%" valign="top">
<p>
  A tiny animated ASCII fish tank for the ESP32-2432S028R Cheap Yellow Display.
</p>
      <p>
      Flash it from the browser, tap to feed the fish, tune the tank, sync the clock over Wi-Fi, and let the punctuation swim.
      </p>
      <p>
        <a href="https://power-pill.github.io/ASCII-Aquarium/">
          Flash ASCII Aquarium CYD
        </a>
      </p>
      <p>
        ASCII Aquarium turns the common 320x240 CYD touchscreen into a living little
desktop aquarium with swimming ASCII fish, rising bubbles, swaying seaweed,
tap-to-feed flakes, occasional octopus and seahorse visitors, touch controls,
Wi-Fi time sync, persistent settings, and SD-card screenshot capture.
</p>
<p>
        It is not a video loop. The aquarium is rendered live on the ESP32, with fish
that wander, school, turn around, change brightness, avoid each other, and chase
food when you tap the glass.
      </p>
    </td>
    <td width="55%" valign="top">
      <img
        src="https://github.com/user-attachments/assets/34200303-25c9-45c5-a6eb-1e53a6c267d7"
        alt="ASCII Aquarium Title Screen"
        width="100%">
    </td>
  </tr>
</table>

Check out the article on Hackaday! https://hackaday.com/2026/05/24/adorable-ascii-aquarium-lives-on-your-desk

## GIFs of the ASCII AQUARIUM in Action }>{{{{• >

<table>
  <tr>
    <td width="50%" valign="top">
    <img
        src="https://github.com/user-attachments/assets/b350f4ad-5aa9-4560-84a4-927dffa96d35"
        alt="Settings"
        width="100%">
    </td>
    <td width="50%" valign="top">
      <img
        src="https://github.com/user-attachments/assets/12696457-80b7-4ba0-9382-38a2e72ea84d"
        alt="Feeding the Fish"
        width="100%">
    </td>
  </tr>
</table>

## ASCII Aquarium Web Flasher >)))'>

The easiest way to install ASCII Aquarium
 is with the browser flasher:

[Flash ASCII Aquarium CYD](https://power-pill.github.io/ASCII-Aquarium/)

You will need:

- A supported [CYD board](https://www.aliexpress.com/item/1005004971720824.html) connected by a USB data cable.
- Chrome or Edge on a desktop computer.
- The Arduino IDE Serial Monitor closed, if it was open.

Open the flasher page, click **Flash ASCII Aquarium**, choose the CYD serial
port, and let the installer finish.

## Supported Hardware ><(((º>

This firmware is built for the [ESP32-2432S028R "Cheap Yellow Display" board](https://www.aliexpress.com/item/1005004971720824.html):

[https://www.aliexpress.com/item/1005004971720824.html](https://www.aliexpress.com/item/1005004971720824.html)

- ESP32
- ILI9341 320x240 display
- XPT2046 resistive touchscreen
- Optional SD card support for BMP screenshots and frame capture

Other CYD-style boards may look similar but use different display, touch, or SD
hardware.

## 3D Printed 2.8" CYD Cases ><((((>`
- [Basic snapfit case by PowerPill.Prints](https://makerworld.com/en/models/2835243)
- [CYD Desk Buddy by annaglyph](https://makerworld.com/en/models/2787810) 

## Features >(°)>

- Animated ASCII fish with multiple glyph species, varied colors, depth shading,
  smooth wraparound, schooling, wandering, and separation behavior.
- Tap-to-feed flakes that nearby fish chase down.
- Configurable fish population from 6 to 36.
- Configurable bubble count from 0 to 50.
- Animated bubbles and seaweed with adjustable sway, length, and randomness.
- Visiting octopus and seahorse characters with selectable spawn rates.
- Fish steer around visitors and each other.
- Background styles: black, blue fade, purple fade, and randomized Spongebob style flower backdrop.
- Touch settings menu with Tank, Seaweed, Clock, and Background tabs.
- Optional on-screen clock with manual time or internet time.
- 12-hour and 24-hour clock formats.
- Timezone selection, small top or bottom clock, large ASCII clock style, and clock color picker.
- Wi-Fi panel with network scan, saved credentials, on-screen keyboard, reconnect handling, and NTP time sync.
- Persistent settings using ESP32 Preferences.
- SD-card BMP screenshots and frame sequence capture.
- Hidden HUD controls for setup, capture, Wi-Fi, settings, quick creature tests, respawn, and randomize.

## Basic Controls ><((((*>

<table>
  <tr>
    <td width="50%" valign="top">
 <p>• Tap the top-left corner to reveal the hidden HUD.</p>
 <p>• Tap the tank to drop food.</p>
 <p>• Use the settings panel to tune fish, bubbles, visitors, seaweed, clock, and
  backgrounds.</p>
 <p>• Use the Wi-Fi panel to connect to a network and sync internet time.</p>
 <p>• Use the capture panel to save BMP sequences to the SD Card. BEWARE - this is EXTREMELY slow since the fishtank simulation is slowed down to allow every frame to be captured.</p>
 <p>• Press and hold the BOOT button on the back of the CYD to save BMP screenshots to the SD card.</p>
    </td>
    <td width="50%" valign="top">
      <img
        src="https://github.com/user-attachments/assets/3a448574-69ee-40fb-a141-50961f769b09"
        alt="ASCII Aquarium Settings"
        width="100%">
    </td>
  </tr>
</table>

## Using the Beam Splitter cube Display ><>

This build can also use a 50 mm beam splitter cube to give the aquarium a little "floating in glass" look. 

A beam splitter cube is made from two glass prisms joined together with a partially reflective diagonal layer inside. When the CYD screen shines into the cube, some of that light passes through and some of it reflects off the internal 45-degree surface. To your eyes, the aquarium appears to hover inside the cube instead of just sitting flat on the display. It is basically a tiny optical tide pool.

If using the clock with the beam splitter cube, you will need to enable “flip clock” in the clock style settings window.

<table>
  <tr>
    <td width="50%" valign="top">
<p>• Use the 50 mm cube size for this printed stand.</p>
<p>• Handle the cube by the edges and wipe fingerprints with a clean micro-fiber cloth. Smudges are the enemy of premium fish.</p>
<p>• Seat the cube squarely in the holder so the clear face points toward the viewer.</p>
<p>• Keep the display bright and the surrounding room a little dimmer if you want the fish to really pop.</p>
<p>• A dark base or darker background behind the cube helps the reflection look cleaner.</p>
<p>• If the image looks faint, doubled, or not quite centred, rotate or flip the cube and try again. Beam splitters can be a bit fin-icky about orientation.</p>
    </td>
    <td width="50%" valign="top">
      <img
        src="https://github.com/user-attachments/assets/36b6a33d-72d2-48c5-ae38-65e8de0e304c"
        alt="ASCII Aquarium Holo Cube"
        width="100%">
    </td>
  </tr>
</table>

The cube does not create the animation by itself; the CYD is still doing all the swimming, bubbling, clocking, and snack-chasing. The cube just splits and redirects the light so the tank feels more like a miniature glass aquarium and less like a bare screen. No water required, unless you count the tears of anyone who bought a real aquarium and then learned about water changes.

## Build From Source ><>

The main Arduino sketch lives here:

```text
ASCII_Aquarium_CYD/ASCII_Aquarium_CYD.ino
```

The sketch expects the CYD display and touch configuration used by the included
TFT_eSPI setup files:

```text
User_Setup.h
User_Setup_Select_CYD.h
```

To build manually:

1. Open `ASCII_Aquarium_CYD.ino` in the Arduino IDE.
2. Select the same ESP32 board/settings used for your CYD.
3. Make sure TFT_eSPI is using the included CYD setup.
4. Compile and upload through the Arduino IDE.

For browser flashing releases, use Arduino IDE's **Export Compiled Binary** and
publish the generated merged firmware binary.


## Project Notes >°>

ASCII Aquarium CYD is part clock, part screensaver, part tiny art object, and
part excuse to make fish-shaped punctuation swim around like it has somewhere
important to be.

No water changes. No tank cycling. No surprise snails. Just plug it in and let
the current take care of itself.
