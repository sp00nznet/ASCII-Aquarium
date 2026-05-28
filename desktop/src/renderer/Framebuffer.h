// RGB565 framebuffer + drawing primitives.
//
// Owns a heap-allocated pixel buffer of the given size. Used both for the
// 320x240 canvas the aquarium draws into each frame and for the smaller
// off-screen sprites the upstream sketch uses (e.g., the 192x20 clock-flip
// sprite). All draw calls clip silently to bounds.
//
// API names match the TFT_eSPI/TFT_eSprite methods used by the upstream
// sketch so ported code can stay as close to the original as is reasonable.

#pragma once

#include <cstdint>
#include <memory>

#include "renderer/Font.h"

namespace aq {

class Framebuffer {
public:
    Framebuffer(int width, int height);

    int width()  const { return width_; }
    int height() const { return height_; }

    std::uint16_t*       data()       { return pixels_.get(); }
    const std::uint16_t* data() const { return pixels_.get(); }

    // Whole-surface fills. fillSprite is the TFT_eSprite name, fillScreen the
    // TFT_eSPI name; both do the same thing here.
    void fillScreen(std::uint16_t color);
    void fillSprite(std::uint16_t color) { fillScreen(color); }

    // Single-pixel access. Out-of-bounds writes are dropped silently; OOB
    // reads return 0 (matches the way TFT_eSprite::readPixel behaves on the
    // clock-flip transparency check in the upstream sketch).
    void          drawPixel(int x, int y, std::uint16_t color);
    std::uint16_t readPixel(int x, int y) const;

    // Lines. drawLine is Bresenham; the sketch's drawThickLine helper stays
    // a free function in the aquarium code (it's just five offset drawLines).
    void drawLine(int x0, int y0, int x1, int y1, std::uint16_t color);

    // Axis-aligned rectangles.
    void fillRect(int x, int y, int w, int h, std::uint16_t color);
    void drawRect(int x, int y, int w, int h, std::uint16_t color);

    // Rounded rectangles. Radius is clamped to min(w, h) / 2.
    void drawRoundRect(int x, int y, int w, int h, int r, std::uint16_t color);
    void fillRoundRect(int x, int y, int w, int h, int r, std::uint16_t color);

    // Blit a tightly-packed RGB565 image into the framebuffer at (x, y).
    void pushImage(int x, int y, int w, int h, const std::uint16_t* data);

    // ---- Stateful text, mirroring the TFT_eSPI / TFT_eSprite API ----
    // Ported sketch code reads identically: setTextFont / setTextDatum /
    // setTextColor / drawString / drawChar / textWidth / setCursor / println.

    void setTextFont(int font);               // 1 or 2
    void setTextSize(int size);               // glyph scale, always 1 upstream
    void setTextDatum(int datum);             // TL_DATUM .. BR_DATUM
    void setTextColor(std::uint16_t fg);                       // transparent bg
    void setTextColor(std::uint16_t fg, std::uint16_t bg);     // opaque bg
    void setTextWrap(bool wrap) { text_wrap_ = wrap; }
    void setCursor(int x, int y) { cursor_x_ = x; cursor_y_ = y; }

    // drawString honors the current datum; returns the drawn width.
    int drawString(const char* str, int x, int y);
    // drawChar draws one glyph top-left at (x, y) ignoring datum; returns width.
    int drawChar(std::uint16_t uni_code, int x, int y);
    // textWidth measures in the current font/size.
    int textWidth(const char* str);
    // println draws at the cursor (top-left) then advances to the next line.
    void println(const char* str);

private:
    void drawHLine(int x, int y, int w, std::uint16_t color);
    void drawVLine(int x, int y, int h, std::uint16_t color);

    // Adafruit-GFX-style quarter-arc helpers. `corners` is a bitmask:
    //   bit 0 = top-left, bit 1 = top-right, bit 2 = bot-right, bit 3 = bot-left.
    void drawCircleHelper(int x0, int y0, int r,
                          std::uint8_t corners, std::uint16_t color);
    void fillCircleHelper(int x0, int y0, int r,
                          std::uint8_t corners, int delta, std::uint16_t color);

    bool in_bounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < width_ && y < height_;
    }

    int width_;
    int height_;
    std::unique_ptr<std::uint16_t[]> pixels_;

    // Text state.
    FontId        text_font_  = FontId::Font1;
    int           text_size_  = 1;
    int           text_datum_ = kTopLeft;
    std::uint16_t text_fg_    = 0xFFFF;
    std::uint16_t text_bg_    = 0x0000;
    bool          text_has_bg_ = false;
    bool          text_wrap_  = true;
    int           cursor_x_   = 0;
    int           cursor_y_   = 0;
};

}  // namespace aq
