// On-screen clock — small-text and big ASCII-art styles, ported from the
// upstream sketch's clock rendering.
//
// Desktop divergence: the device hand-rolls a clock from NTP/WiFi + a manual
// set-time UI. A desktop kiosk already has a correct system clock, so this
// reads OS local time each frame and feeds the *same* format/draw code. The
// per-field manual time-setting UI and NTP machinery are intentionally dropped;
// timezone follows the host's local time.

#pragma once

#include <cstdint>

#include "renderer/Framebuffer.h"

namespace aq {

enum class ClockStyle : int { SmallText = 0, Ascii = 1, Count = 2 };
enum class ClockSmallPosition : int { Top = 0, Bottom = 1, Count = 2 };

class Clock {
public:
    Clock();

    // Refresh the displayed time from the OS local clock. Call once per frame.
    void update();

    // Big ASCII-art clock sits behind the fish (drawn right after the
    // background); the small text clock is an overlay drawn after the entities.
    void drawBackgroundLayer(Framebuffer& fb);
    void drawOverlay(Framebuffer& fb);

    // ---- Settings hooks ----
    bool visible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }
    void toggleVisible() { visible_ = !visible_; }

    ClockStyle style() const { return style_; }
    void setStyle(ClockStyle s) { style_ = s; }
    void cycleStyle() {
        style_ = (style_ == ClockStyle::SmallText) ? ClockStyle::Ascii : ClockStyle::SmallText;
    }

    ClockSmallPosition smallPosition() const { return smallPosition_; }
    void setSmallPosition(ClockSmallPosition p) { smallPosition_ = p; }

    bool use24Hour() const { return use24Hour_; }
    void setUse24Hour(bool v) { use24Hour_ = v; }

    bool flipHorizontal() const { return flipHorizontal_; }
    void setFlipHorizontal(bool v) { flipHorizontal_ = v; }

    std::uint16_t activeTextColor() const {
        return (style_ == ClockStyle::Ascii) ? asciiTextColor_ : smallTextColor_;
    }
    void setActiveTextColor(std::uint16_t c) {
        if (style_ == ClockStyle::Ascii) asciiTextColor_ = c; else smallTextColor_ = c;
    }

    // Direct per-style color access (for persistence).
    std::uint16_t smallTextColor() const { return smallTextColor_; }
    std::uint16_t asciiTextColor() const { return asciiTextColor_; }
    void setSmallTextColor(std::uint16_t c) { smallTextColor_ = c; }
    void setAsciiTextColor(std::uint16_t c) { asciiTextColor_ = c; }

    // Selectable text-color palette (for the Settings color picker grid).
    static int paletteCount();
    static std::uint16_t paletteColor(int i);

private:
    void formatClockDisplay(char* out, std::size_t outCap) const;
    void formatClockTimeOnly(char* out, std::size_t outCap, bool includeMeridiem) const;
    void drawMirroredSmallClock(Framebuffer& fb, const char* line, int y);

    // Current time, refreshed from the OS in update().
    int month_ = 1;   // 1..12
    int day_ = 1;     // 1..31
    int hour_ = 0;    // 0..23
    int minute_ = 0;  // 0..59

    bool visible_ = false;
    ClockStyle style_ = ClockStyle::SmallText;
    ClockSmallPosition smallPosition_ = ClockSmallPosition::Top;
    bool use24Hour_ = false;
    bool flipHorizontal_ = false;
    std::uint16_t smallTextColor_;
    std::uint16_t asciiTextColor_;

    // Off-screen scratch surface for the true horizontal pixel-flip.
    Framebuffer flipSprite_;
};

}  // namespace aq
