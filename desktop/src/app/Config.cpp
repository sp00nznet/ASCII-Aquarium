#include "app/Config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "sim/Aquarium.h"
#include "sim/Background.h"
#include "sim/Clock.h"

namespace fs = std::filesystem;

namespace aq {
namespace config {

bool Settings::operator==(const Settings& o) const {
    return fish == o.fish && bubbles == o.bubbles && octopusFreq == o.octopusFreq &&
           seahorseFreq == o.seahorseFreq && sway == o.sway && length == o.length &&
           randomness == o.randomness && bgMode == o.bgMode && clockOn == o.clockOn &&
           clock24h == o.clock24h && clockFlip == o.clockFlip && clockStyle == o.clockStyle &&
           clockPos == o.clockPos && clockSmallColor == o.clockSmallColor &&
           clockAsciiColor == o.clockAsciiColor;
}

namespace {

fs::path configDir() {
#if defined(_WIN32)
    const char* base = std::getenv("APPDATA");
    fs::path dir = (base && base[0]) ? fs::path(base) : fs::path(".");
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    const char* home = std::getenv("HOME");
    fs::path dir;
    if (xdg && xdg[0]) dir = fs::path(xdg);
    else if (home && home[0]) dir = fs::path(home) / ".config";
    else dir = fs::path(".");
#endif
    return dir / "ascii-aquarium";
}

}  // namespace

std::string filePath() {
    return (configDir() / "config.ini").string();
}

bool load(const std::string& path, Settings& out) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    while (std::getline(in, line)) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = line.substr(0, eq);
        const std::string val = line.substr(eq + 1);
        if (val.empty()) continue;
        try {
            if (key == "fish") out.fish = std::stoi(val);
            else if (key == "bubbles") out.bubbles = std::stoi(val);
            else if (key == "octopus_freq") out.octopusFreq = std::stoi(val);
            else if (key == "seahorse_freq") out.seahorseFreq = std::stoi(val);
            else if (key == "sway") out.sway = std::stof(val);
            else if (key == "length") out.length = std::stof(val);
            else if (key == "randomness") out.randomness = std::stof(val);
            else if (key == "bg_mode") out.bgMode = std::stoi(val);
            else if (key == "clock_on") out.clockOn = (std::stoi(val) != 0);
            else if (key == "clock_24h") out.clock24h = (std::stoi(val) != 0);
            else if (key == "clock_flip") out.clockFlip = (std::stoi(val) != 0);
            else if (key == "clock_style") out.clockStyle = std::stoi(val);
            else if (key == "clock_pos") out.clockPos = std::stoi(val);
            else if (key == "clock_small_color")
                out.clockSmallColor = static_cast<std::uint16_t>(std::stoul(val));
            else if (key == "clock_ascii_color")
                out.clockAsciiColor = static_cast<std::uint16_t>(std::stoul(val));
        } catch (...) {
            // Skip malformed values, keep the existing default.
        }
    }
    return true;
}

bool save(const std::string& path, const Settings& s) {
    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    out << "# ASCII Aquarium settings\n";
    out << "fish=" << s.fish << "\n";
    out << "bubbles=" << s.bubbles << "\n";
    out << "octopus_freq=" << s.octopusFreq << "\n";
    out << "seahorse_freq=" << s.seahorseFreq << "\n";
    out << "sway=" << s.sway << "\n";
    out << "length=" << s.length << "\n";
    out << "randomness=" << s.randomness << "\n";
    out << "bg_mode=" << s.bgMode << "\n";
    out << "clock_on=" << (s.clockOn ? 1 : 0) << "\n";
    out << "clock_24h=" << (s.clock24h ? 1 : 0) << "\n";
    out << "clock_flip=" << (s.clockFlip ? 1 : 0) << "\n";
    out << "clock_style=" << s.clockStyle << "\n";
    out << "clock_pos=" << s.clockPos << "\n";
    out << "clock_small_color=" << static_cast<unsigned>(s.clockSmallColor) << "\n";
    out << "clock_ascii_color=" << static_cast<unsigned>(s.clockAsciiColor) << "\n";
    return static_cast<bool>(out);
}

Settings snapshot(const Aquarium& aquarium, const Clock& clock, BackgroundMode bgMode) {
    Settings s;
    s.fish = aquarium.fishTarget();
    s.bubbles = aquarium.bubbleTarget();
    s.octopusFreq = aquarium.octopusFrequency();
    s.seahorseFreq = aquarium.seahorseFrequency();
    s.sway = aquarium.seaweedSway();
    s.length = aquarium.seaweedLength();
    s.randomness = aquarium.seaweedRandomness();
    s.bgMode = static_cast<int>(bgMode);
    s.clockOn = clock.visible();
    s.clock24h = clock.use24Hour();
    s.clockFlip = clock.flipHorizontal();
    s.clockStyle = static_cast<int>(clock.style());
    s.clockPos = static_cast<int>(clock.smallPosition());
    s.clockSmallColor = clock.smallTextColor();
    s.clockAsciiColor = clock.asciiTextColor();
    return s;
}

void apply(const Settings& s, Aquarium& aquarium, Clock& clock, BackgroundMode& bgMode) {
    aquarium.setFishTarget(s.fish);
    aquarium.setBubbleTarget(s.bubbles);
    aquarium.setOctopusFrequency(s.octopusFreq);
    aquarium.setSeahorseFrequency(s.seahorseFreq);
    aquarium.setSeaweedSway(s.sway);
    aquarium.setSeaweedLength(s.length);
    aquarium.setSeaweedRandomness(s.randomness);

    int bg = s.bgMode;
    if (bg < 0 || bg >= static_cast<int>(BackgroundMode::Count)) bg = 1;
    bgMode = static_cast<BackgroundMode>(bg);

    clock.setVisible(s.clockOn);
    clock.setUse24Hour(s.clock24h);
    clock.setFlipHorizontal(s.clockFlip);
    int style = s.clockStyle;
    if (style < 0 || style >= static_cast<int>(ClockStyle::Count)) style = 0;
    clock.setStyle(static_cast<ClockStyle>(style));
    int pos = s.clockPos;
    if (pos < 0 || pos >= static_cast<int>(ClockSmallPosition::Count)) pos = 0;
    clock.setSmallPosition(static_cast<ClockSmallPosition>(pos));
    clock.setSmallTextColor(s.clockSmallColor);
    clock.setAsciiTextColor(s.clockAsciiColor);
}

}  // namespace config
}  // namespace aq
