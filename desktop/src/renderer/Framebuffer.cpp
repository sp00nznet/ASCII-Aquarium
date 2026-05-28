#include "renderer/Framebuffer.h"

#include <algorithm>
#include <cstring>

namespace aq {

namespace {
inline int abs_i(int v) { return v < 0 ? -v : v; }
}  // namespace

Framebuffer::Framebuffer(int width, int height)
    : width_(width),
      height_(height),
      pixels_(std::make_unique<std::uint16_t[]>(
          static_cast<std::size_t>(width) * static_cast<std::size_t>(height))) {
}

void Framebuffer::fillScreen(std::uint16_t color) {
    const std::size_t n = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    // memset only works for 0x0000 / 0xFFFF when the two bytes happen to match;
    // a small loop is fast enough and simpler than splitting cases.
    std::uint16_t* p = pixels_.get();
    for (std::size_t i = 0; i < n; ++i) p[i] = color;
}

void Framebuffer::drawPixel(int x, int y, std::uint16_t color) {
    if (!in_bounds(x, y)) return;
    pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_)
            + static_cast<std::size_t>(x)] = color;
}

std::uint16_t Framebuffer::readPixel(int x, int y) const {
    if (!in_bounds(x, y)) return 0;
    return pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_)
                   + static_cast<std::size_t>(x)];
}

void Framebuffer::drawHLine(int x, int y, int w, std::uint16_t color) {
    if (y < 0 || y >= height_ || w <= 0) return;
    int x0 = std::max(x, 0);
    int x1 = std::min(x + w, width_);
    if (x0 >= x1) return;
    std::uint16_t* row = pixels_.get() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width_);
    for (int i = x0; i < x1; ++i) row[i] = color;
}

void Framebuffer::drawVLine(int x, int y, int h, std::uint16_t color) {
    if (x < 0 || x >= width_ || h <= 0) return;
    int y0 = std::max(y, 0);
    int y1 = std::min(y + h, height_);
    if (y0 >= y1) return;
    for (int j = y0; j < y1; ++j) {
        pixels_[static_cast<std::size_t>(j) * static_cast<std::size_t>(width_)
                + static_cast<std::size_t>(x)] = color;
    }
}

void Framebuffer::drawLine(int x0, int y0, int x1, int y1, std::uint16_t color) {
    // Bresenham. Specialized H/V cases for the common UI lines.
    if (y0 == y1) { drawHLine(std::min(x0, x1), y0, abs_i(x1 - x0) + 1, color); return; }
    if (x0 == x1) { drawVLine(x0, std::min(y0, y1), abs_i(y1 - y0) + 1, color); return; }

    const int dx = abs_i(x1 - x0);
    const int dy = -abs_i(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    int x = x0, y = y0;
    for (;;) {
        drawPixel(x, y, color);
        if (x == x1 && y == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

void Framebuffer::fillRect(int x, int y, int w, int h, std::uint16_t color) {
    if (w <= 0 || h <= 0) return;
    const int x0 = std::max(x, 0);
    const int y0 = std::max(y, 0);
    const int x1 = std::min(x + w, width_);
    const int y1 = std::min(y + h, height_);
    if (x0 >= x1 || y0 >= y1) return;
    for (int j = y0; j < y1; ++j) {
        std::uint16_t* row = pixels_.get() + static_cast<std::size_t>(j) * static_cast<std::size_t>(width_);
        for (int i = x0; i < x1; ++i) row[i] = color;
    }
}

void Framebuffer::drawRect(int x, int y, int w, int h, std::uint16_t color) {
    if (w <= 0 || h <= 0) return;
    drawHLine(x, y, w, color);
    drawHLine(x, y + h - 1, w, color);
    drawVLine(x, y, h, color);
    drawVLine(x + w - 1, y, h, color);
}

// ---- Adafruit-GFX-style rounded-rect helpers ----
//
// drawCircleHelper / fillCircleHelper draw quarter-arcs identified by a
// 4-bit `corners` mask. The mapping matches Adafruit_GFX so the visual
// result is identical to TFT_eSPI (which inherits the same convention).

void Framebuffer::drawCircleHelper(int x0, int y0, int r,
                                   std::uint8_t corners, std::uint16_t color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (corners & 0x4) { drawPixel(x0 + x, y0 + y, color); drawPixel(x0 + y, y0 + x, color); }
        if (corners & 0x2) { drawPixel(x0 + x, y0 - y, color); drawPixel(x0 + y, y0 - x, color); }
        if (corners & 0x8) { drawPixel(x0 - y, y0 + x, color); drawPixel(x0 - x, y0 + y, color); }
        if (corners & 0x1) { drawPixel(x0 - y, y0 - x, color); drawPixel(x0 - x, y0 - y, color); }
    }
}

void Framebuffer::fillCircleHelper(int x0, int y0, int r,
                                   std::uint8_t corners, int delta,
                                   std::uint16_t color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;
    int px = x;
    int py = y;
    delta++;
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (x < (y + 1)) {
            if (corners & 1) drawVLine(x0 + x, y0 - y, 2 * y + delta, color);
            if (corners & 2) drawVLine(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (corners & 1) drawVLine(x0 + py, y0 - px, 2 * px + delta, color);
            if (corners & 2) drawVLine(x0 - py, y0 - px, 2 * px + delta, color);
            py = y;
        }
        px = x;
    }
}

void Framebuffer::drawRoundRect(int x, int y, int w, int h, int r,
                                std::uint16_t color) {
    if (w <= 0 || h <= 0) return;
    const int max_r = std::min(w, h) / 2;
    if (r > max_r) r = max_r;
    if (r < 0) r = 0;
    drawHLine(x + r,         y,             w - 2 * r, color);
    drawHLine(x + r,         y + h - 1,     w - 2 * r, color);
    drawVLine(x,             y + r,         h - 2 * r, color);
    drawVLine(x + w - 1,     y + r,         h - 2 * r, color);
    drawCircleHelper(x + r,         y + r,         r, 1, color);
    drawCircleHelper(x + w - r - 1, y + r,         r, 2, color);
    drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
    drawCircleHelper(x + r,         y + h - r - 1, r, 8, color);
}

void Framebuffer::fillRoundRect(int x, int y, int w, int h, int r,
                                std::uint16_t color) {
    if (w <= 0 || h <= 0) return;
    const int max_r = std::min(w, h) / 2;
    if (r > max_r) r = max_r;
    if (r < 0) r = 0;
    fillRect(x + r, y, w - 2 * r, h, color);
    fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    fillCircleHelper(x + r,         y + r, r, 2, h - 2 * r - 1, color);
}

// ---- Text ----

void Framebuffer::setTextFont(int font) {
    text_font_ = (font == 2) ? FontId::Font2 : FontId::Font1;
}

void Framebuffer::setTextSize(int size) {
    text_size_ = (size < 1) ? 1 : size;
}

void Framebuffer::setTextDatum(int datum) {
    text_datum_ = datum;
}

void Framebuffer::setTextColor(std::uint16_t fg) {
    text_fg_ = fg;
    text_has_bg_ = false;
}

void Framebuffer::setTextColor(std::uint16_t fg, std::uint16_t bg) {
    text_fg_ = fg;
    text_bg_ = bg;
    text_has_bg_ = true;
}

int Framebuffer::textWidth(const char* str) {
    return font::text_width(text_font_, str, text_size_);
}

int Framebuffer::drawString(const char* str, int x, int y) {
    if (str == nullptr) return 0;
    const int w = font::text_width(text_font_, str, text_size_);
    const int h = font::glyph_height(text_font_) * text_size_;

    // Horizontal datum bias.
    switch (text_datum_) {
        case kTopCentre: case kMidCentre: case kBotCentre: x -= w / 2; break;
        case kTopRight:  case kMidRight:  case kBotRight:  x -= w;     break;
        default: break;  // left
    }
    // Vertical datum bias.
    switch (text_datum_) {
        case kMidLeft: case kMidCentre: case kMidRight: y -= h / 2; break;
        case kBotLeft: case kBotCentre: case kBotRight: y -= h;     break;
        default: break;  // top
    }

    int cx = x;
    for (const char* p = str; *p; ++p) {
        cx += font::draw_char(*this, text_font_, *p, cx, y,
                              text_fg_, text_bg_, text_has_bg_, text_size_);
    }
    return w;
}

int Framebuffer::drawChar(std::uint16_t uni_code, int x, int y) {
    return font::draw_char(*this, text_font_, static_cast<char>(uni_code & 0xFF),
                           x, y, text_fg_, text_bg_, text_has_bg_, text_size_);
}

void Framebuffer::println(const char* str) {
    if (str != nullptr) {
        int cx = cursor_x_;
        for (const char* p = str; *p; ++p) {
            cx += font::draw_char(*this, text_font_, *p, cx, cursor_y_,
                                  text_fg_, text_bg_, text_has_bg_, text_size_);
        }
    }
    cursor_y_ += font::glyph_height(text_font_) * text_size_;
}

void Framebuffer::pushImage(int x, int y, int w, int h,
                            const std::uint16_t* data) {
    if (w <= 0 || h <= 0 || data == nullptr) return;
    const int sx0 = std::max(x, 0);
    const int sy0 = std::max(y, 0);
    const int sx1 = std::min(x + w, width_);
    const int sy1 = std::min(y + h, height_);
    if (sx0 >= sx1 || sy0 >= sy1) return;
    for (int j = sy0; j < sy1; ++j) {
        const std::uint16_t* src_row = data + (j - y) * w + (sx0 - x);
        std::uint16_t* dst_row = pixels_.get()
            + static_cast<std::size_t>(j) * static_cast<std::size_t>(width_)
            + static_cast<std::size_t>(sx0);
        std::memcpy(dst_row, src_row, static_cast<std::size_t>(sx1 - sx0) * sizeof(std::uint16_t));
    }
}

}  // namespace aq
