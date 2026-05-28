// Bitmap text rendering for the two TFT_eSPI fonts the upstream sketch uses:
//   Font 1 = GLCD 6x8 fixed-width (default)
//   Font 2 = 16px proportional smooth font
//
// These are free functions operating on a Framebuffer; the stateful
// TFT_eSPI-style text API (setTextFont/Datum/Color/cursor/println) lives on
// Framebuffer itself and delegates here. The bulky vendored glyph tables are
// included only in Font.cpp so they don't leak into every translation unit.

#pragma once

#include <cstdint>

namespace aq {

class Framebuffer;

enum class FontId : int { Font1 = 1, Font2 = 2 };

// Text datum (alignment) values, matching TFT_eSPI's constants. The upstream
// sketch uses TL/TC/TR/ML/MC/MR; the rest are defined for completeness.
enum TextDatum : int {
    kTopLeft = 0,   kTopCentre = 1,   kTopRight = 2,
    kMidLeft = 3,   kMidCentre = 4,   kMidRight = 5,
    kBotLeft = 6,   kBotCentre = 7,   kBotRight = 8,
};

namespace font {

// Pixel height of a glyph cell (8 for Font 1, 16 for Font 2), unscaled.
int glyph_height(FontId f);

// Advance width of one char (includes inter-glyph spacing), scaled by `size`.
int char_width(FontId f, char c, int size);

// Advance width of a whole string, scaled by `size`.
int text_width(FontId f, const char* s, int size);

// Rasterize one glyph with its top-left at (x, y). When `has_bg` is true the
// full glyph cell is painted `bg` first (opaque text); otherwise only lit
// pixels are drawn (transparent text). Returns the advance width so callers
// can position the next glyph.
int draw_char(Framebuffer& fb, FontId f, char c, int x, int y,
              std::uint16_t fg, std::uint16_t bg, bool has_bg, int size);

}  // namespace font
}  // namespace aq

// Bare datum names so ported sketch code (`setTextDatum(MC_DATUM)`) compiles
// unchanged.
#define TL_DATUM ::aq::kTopLeft
#define TC_DATUM ::aq::kTopCentre
#define TR_DATUM ::aq::kTopRight
#define ML_DATUM ::aq::kMidLeft
#define MC_DATUM ::aq::kMidCentre
#define MR_DATUM ::aq::kMidRight
#define BL_DATUM ::aq::kBotLeft
#define BC_DATUM ::aq::kBotCentre
#define BR_DATUM ::aq::kBotRight
