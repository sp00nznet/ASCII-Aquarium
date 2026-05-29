// RGB565 color palette matching TFT_eSPI's named constants so the upstream
// sketch's TFT_BLACK / TFT_NAVY / ... compile and render identically here.
//
// Values track TFT_eSPI master (Bodmer/TFT_eSPI). If a future upstream sync
// uses a constant added since this snapshot, add it here with the matching
// 16-bit value.

#pragma once

#include <cstdint>

namespace aq {

// RGB888 -> RGB565 packing, evaluated at compile time when the inputs are
// constexpr. Used both for the constants below and at call sites.
constexpr std::uint16_t rgb565(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return static_cast<std::uint16_t>(
        ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Multiply a packed RGB565 color's channels by `brightness` in [0, 1]. Used by
// the aquarium to dim fish by depth. Matches the upstream scaleRgb565.
inline std::uint16_t scaleRgb565(std::uint16_t color, float brightness) {
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;
    const auto r = static_cast<std::uint16_t>(((color >> 11) & 0x1F) * brightness);
    const auto g = static_cast<std::uint16_t>(((color >> 5) & 0x3F) * brightness);
    const auto b = static_cast<std::uint16_t>((color & 0x1F) * brightness);
    return static_cast<std::uint16_t>((r << 11) | (g << 5) | b);
}

// Pack an RGB888 triple (channels clamped to [0, 255]) into RGB565. Matches the
// upstream rgb565From888; used for the octopus/seahorse animated tints.
inline std::uint16_t rgb565From888(int r8, int g8, int b8) {
    const auto clamp8 = [](int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); };
    return rgb565(static_cast<std::uint8_t>(clamp8(r8)),
                  static_cast<std::uint8_t>(clamp8(g8)),
                  static_cast<std::uint8_t>(clamp8(b8)));
}

namespace color {

// Values copied from TFT_eSPI's color definitions. Kept verbatim so a `git
// diff` against the upstream library is trivial to check.
inline constexpr std::uint16_t Black        = 0x0000;
inline constexpr std::uint16_t Navy         = 0x000F;
inline constexpr std::uint16_t DarkGreen    = 0x03E0;
inline constexpr std::uint16_t DarkCyan     = 0x03EF;
inline constexpr std::uint16_t Maroon       = 0x7800;
inline constexpr std::uint16_t Purple       = 0x780F;
inline constexpr std::uint16_t Olive        = 0x7BE0;
inline constexpr std::uint16_t LightGrey    = 0xD69A;
inline constexpr std::uint16_t DarkGrey     = 0x7BEF;
inline constexpr std::uint16_t Blue         = 0x001F;
inline constexpr std::uint16_t Green        = 0x07E0;
inline constexpr std::uint16_t Cyan         = 0x07FF;
inline constexpr std::uint16_t Red          = 0xF800;
inline constexpr std::uint16_t Magenta      = 0xF81F;
inline constexpr std::uint16_t Yellow       = 0xFFE0;
inline constexpr std::uint16_t White        = 0xFFFF;
inline constexpr std::uint16_t Orange       = 0xFDA0;
inline constexpr std::uint16_t GreenYellow  = 0xB7E0;
inline constexpr std::uint16_t Pink         = 0xFE19;
inline constexpr std::uint16_t Brown        = 0x9A60;
inline constexpr std::uint16_t Gold         = 0xFEA0;
inline constexpr std::uint16_t Silver       = 0xC618;
inline constexpr std::uint16_t SkyBlue      = 0x867D;
inline constexpr std::uint16_t Violet       = 0x915C;

}  // namespace color

}  // namespace aq

// Upstream sketch uses the TFT_eSPI-style ALL_CAPS names directly. Expose
// them as aliases so ported code reads identically to the original.
#define TFT_BLACK        ::aq::color::Black
#define TFT_NAVY         ::aq::color::Navy
#define TFT_DARKGREEN    ::aq::color::DarkGreen
#define TFT_DARKCYAN     ::aq::color::DarkCyan
#define TFT_MAROON       ::aq::color::Maroon
#define TFT_PURPLE       ::aq::color::Purple
#define TFT_OLIVE        ::aq::color::Olive
#define TFT_LIGHTGREY    ::aq::color::LightGrey
#define TFT_DARKGREY     ::aq::color::DarkGrey
#define TFT_BLUE         ::aq::color::Blue
#define TFT_GREEN        ::aq::color::Green
#define TFT_CYAN         ::aq::color::Cyan
#define TFT_RED          ::aq::color::Red
#define TFT_MAGENTA      ::aq::color::Magenta
#define TFT_YELLOW       ::aq::color::Yellow
#define TFT_WHITE        ::aq::color::White
#define TFT_ORANGE       ::aq::color::Orange
#define TFT_GREENYELLOW  ::aq::color::GreenYellow
#define TFT_PINK         ::aq::color::Pink
#define TFT_BROWN        ::aq::color::Brown
#define TFT_GOLD         ::aq::color::Gold
#define TFT_SILVER       ::aq::color::Silver
#define TFT_SKYBLUE      ::aq::color::SkyBlue
#define TFT_VIOLET       ::aq::color::Violet

// Upstream's RGB565(r,g,b) packing macro, so ported tables (e.g. fishSpecies)
// read identically to the original sketch.
#define RGB565           ::aq::rgb565
