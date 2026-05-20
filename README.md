# ASCII Aquarium

<table>
  <tr>
    <td width="58%" valign="top">
      <h1>ASCII Aquarium</h1>
      <p>
        A tiny animated ASCII fish tank for the ESP32-2432S028R
        Cheap Yellow Display.
      </p>
      <p>
        Flash it from the browser, tap to feed the fish, tune the tank,
        sync the clock over Wi-Fi, and let the punctuation swim.
      </p>
      <p>
        <a href="https://power-pill.github.io/ASCII-Aquarium/">
          Flash ASCII Aquarium CYD
        </a>
      </p>
    </td>
    <td width="42%" valign="top">
      <img
        src="https://github.com/user-attachments/assets/34200303-25c9-45c5-a6eb-1e53a6c267d7"
        alt="ASCII Aquarium running on a CYD display"
        width="100%">
    </td>
  </tr>
</table>

A tiny animated ASCII fish tank for the ESP32-2432S028R "Cheap Yellow Display".

ASCII Aquarium turns the common 320x240 CYD touchscreen into a living little
desktop aquarium with swimming ASCII fish, rising bubbles, swaying seaweed,
tap-to-feed flakes, occasional octopus and seahorse visitors, touch controls,
Wi-Fi time sync, persistent settings, and SD-card screenshot capture.

It is not a video loop. The aquarium is rendered live on the ESP32, with fish
that wander, school, turn around, change brightness, avoid each other, and chase
food when you tap the glass.

<td width="42%" valign="top">
      <img
        src="https://github.com/user-attachments/assets/34200303-25c9-45c5-a6eb-1e53a6c267d7"
        alt="ASCII Aquarium Title Screen Render"
        width="100%">
    </td>

## Web Flasher

The easiest way to install ASCII Aquarium
 is with the browser flasher:

[Flash ASCII Aquarium CYD](https://power-pill.github.io/ASCII-Aquarium/)

You will need:

- A supported [CYD board](https://www.aliexpress.com/item/1005004971720824.html) connected by a USB data cable.
- Chrome or Edge on a desktop computer.
- The Arduino IDE Serial Monitor closed, if it was open.

Open the flasher page, click **Flash ASCII Aquarium**, choose the CYD serial
port, and let the installer finish.

## Supported Hardware

This firmware is built for the [ESP32-2432S028R "Cheap Yellow Display" board](https://www.aliexpress.com/item/1005004971720824.html):

[https://www.aliexpress.com/item/1005004971720824.html](https://www.aliexpress.com/item/1005004971720824.html)

- ESP32
- ILI9341 320x240 display
- XPT2046 resistive touchscreen
- Optional SD card support for BMP screenshots and frame capture

Other CYD-style boards may look similar but use different display, touch, or SD
hardware.

## Features

- Animated ASCII fish with multiple glyph species, varied colors, depth shading,
  smooth wraparound, schooling, wandering, and separation behavior.
- Tap-to-feed flakes that nearby fish chase down.
- Configurable fish population from 6 to 36.
- Configurable bubble count from 0 to 50.
- Animated bubbles and seaweed with adjustable sway, length, and randomness.
- Visiting octopus and seahorse characters with selectable spawn rates.
- Fish steer around visitors and each other.
- Background styles: black, blue fade, purple fade, and randomized pixel-art
  flower backdrop.
- Touch settings menu with Tank, Seaweed, Clock, and Background tabs.
- Optional on-screen clock with manual time or internet time.
- 12-hour and 24-hour clock formats.
- Timezone selection, small top or bottom clock, large ASCII clock style, and
  clock color picker.
- Wi-Fi panel with network scan, saved credentials, on-screen keyboard,
  reconnect handling, and NTP time sync.
- Persistent settings using ESP32 Preferences.
- SD-card BMP screenshots and frame sequence capture.
- Hidden HUD controls for setup, capture, Wi-Fi, settings, quick creature tests,
  respawn, and randomize.
- Double-buffered sprite rendering for smoother animation.

## Basic Controls

- Tap the tank to drop food.
- Tap the top-left corner to reveal the hidden HUD.
- Use the settings panel to tune fish, bubbles, visitors, seaweed, clock, and
  backgrounds.
- Use the Wi-Fi panel to connect to a network and sync internet time.
- Use the capture panel or BOOT button to save BMP screenshots to SD.

 <img width="640" height="480" alt="ASCII_Aquarium_Settings" src="https://github.com/user-attachments/assets/3a448574-69ee-40fb-a141-50961f769b09" style="float: right;"/>


## Build From Source

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


## Project Notes

ASCII Aquarium CYD is part clock, part screensaver, part tiny art object, and
part excuse to make fish-shaped punctuation swim around like it has somewhere
important to be.

No water changes. No tank cycling. No surprise snails. Just plug it in and let
the current take care of itself.
