#include "renderer/Font.h"

#include "renderer/Framebuffer.h"

#include "fonts/font16.h"
#include "fonts/glcdfont.h"

namespace aq {
namespace font {

namespace {

inline void put_block(Framebuffer& fb, int x, int y, int size, std::uint16_t c) {
    if (size == 1) {
        fb.drawPixel(x, y, c);
    } else {
        fb.fillRect(x, y, size, size, c);
    }
}

// ---- Font 1: GLCD 6x8 fixed-width ----
// 5 data columns per glyph (LSB = top pixel), drawn 6 wide with a trailing
// blank spacer column.
int draw_char_font1(Framebuffer& fb, char c, int x, int y,
                    std::uint16_t fg, std::uint16_t bg, bool has_bg, int size) {
    const auto uc = static_cast<unsigned char>(c);
    const std::uint8_t* glyph = &fonts::kGlcdFont[uc * 5u];
    for (int col = 0; col < 6; ++col) {
        const std::uint8_t bits = (col < 5) ? glyph[col] : 0u;
        for (int row = 0; row < 8; ++row) {
            const bool on = (bits >> row) & 1u;
            if (on) {
                put_block(fb, x + col * size, y + row * size, size, fg);
            } else if (has_bg) {
                put_block(fb, x + col * size, y + row * size, size, bg);
            }
        }
    }
    return 6 * size;
}

// ---- Font 2: 16px proportional ----
// One byte per pixel-line for glyphs up to 8 wide, two bytes per line for
// 9..16 wide. MSB is the leftmost column. widtbl value is the advance width
// (it bakes in one trailing spacer column).
int draw_char_font2(Framebuffer& fb, char c, int x, int y,
                    std::uint16_t fg, std::uint16_t bg, bool has_bg, int size) {
    int idx = static_cast<unsigned char>(c) - fonts::f16::kFirstChar;
    if (idx < 0 || idx >= fonts::f16::kCharCount) {
        idx = 0;  // out of range -> space
    }
    const int w = fonts::f16::widtbl_f16[idx];
    const std::uint8_t* data = fonts::f16::chrtbl_f16[idx];
    const int bytes_per_row = (w + 7) / 8;

    for (int row = 0; row < fonts::f16::kHeight; ++row) {
        const std::uint8_t* row_bytes = data + row * bytes_per_row;
        for (int col = 0; col < w; ++col) {
            const std::uint8_t byte = row_bytes[col / 8];
            const bool on = (byte & (0x80u >> (col % 8))) != 0u;
            if (on) {
                put_block(fb, x + col * size, y + row * size, size, fg);
            } else if (has_bg) {
                put_block(fb, x + col * size, y + row * size, size, bg);
            }
        }
    }
    return w * size;
}

}  // namespace

int glyph_height(FontId f) {
    return (f == FontId::Font2) ? fonts::f16::kHeight : 8;
}

int char_width(FontId f, char c, int size) {
    if (f == FontId::Font2) {
        int idx = static_cast<unsigned char>(c) - fonts::f16::kFirstChar;
        if (idx < 0 || idx >= fonts::f16::kCharCount) idx = 0;
        return fonts::f16::widtbl_f16[idx] * size;
    }
    return 6 * size;
}

int text_width(FontId f, const char* s, int size) {
    if (s == nullptr) return 0;
    int total = 0;
    for (const char* p = s; *p; ++p) {
        total += char_width(f, *p, size);
    }
    return total;
}

int draw_char(Framebuffer& fb, FontId f, char c, int x, int y,
              std::uint16_t fg, std::uint16_t bg, bool has_bg, int size) {
    if (size < 1) size = 1;
    if (f == FontId::Font2) {
        return draw_char_font2(fb, c, x, y, fg, bg, has_bg, size);
    }
    return draw_char_font1(fb, c, x, y, fg, bg, has_bg, size);
}

}  // namespace font
}  // namespace aq
