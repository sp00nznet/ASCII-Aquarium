// Settings persistence.
//
// Replaces the device's ESP32 NVS (Preferences) with a plain key=value text
// file in the platform config dir (%APPDATA% on Windows, $XDG_CONFIG_HOME or
// ~/.config elsewhere). Persists the same tank/seaweed/background/clock
// settings the sketch saved, minus the WiFi / internet-time / manual-clock /
// timezone fields that don't apply now that the clock follows OS time.

#pragma once

#include <cstdint>
#include <string>

namespace aq {

class Aquarium;
class Clock;
class Lighting;
class Props;
enum class BackgroundMode : int;

namespace config {

struct Settings {
    int   fish = 16;
    int   bubbles = 10;
    int   octopusFreq = 1;
    int   seahorseFreq = 1;
    float sway = 1.10f;
    float length = 1.35f;
    float randomness = 0.35f;
    int   bgMode = 1;  // BackgroundMode::BlueGradient
    bool  clockOn = false;
    bool  clock24h = false;
    bool  clockFlip = false;
    int   clockStyle = 0;  // ClockStyle::SmallText
    int   clockPos = 0;    // ClockSmallPosition::Top
    std::uint16_t clockSmallColor = 0xFFFF;
    std::uint16_t clockAsciiColor = 0x008B;  // RGB565(0,20,95)
    // Scene toggles (desktop extensions).
    bool  dayNight = true;
    bool  caustics = true;
    bool  props = true;
    bool  burnin = true;
    bool  autocycle = false;

    bool operator==(const Settings& o) const;
    bool operator!=(const Settings& o) const { return !(*this == o); }
};

// Absolute path to the config file, creating its parent directory if needed.
std::string filePath();

// Read settings from `path` into `out` (which should already hold defaults).
// Returns true if the file existed and was read.
bool load(const std::string& path, Settings& out);

// Write settings to `path` (creating the directory). Returns true on success.
bool save(const std::string& path, const Settings& s);

// Snapshot the current live settings / push settings into the live objects.
Settings snapshot(const Aquarium& aquarium, const Clock& clock, BackgroundMode bgMode,
                  const Lighting& lighting, const Props& props, bool burnin, bool autocycle);
void apply(const Settings& s, Aquarium& aquarium, Clock& clock, BackgroundMode& bgMode,
           Lighting& lighting, Props& props, bool& burnin, bool& autocycle);

}  // namespace config
}  // namespace aq
