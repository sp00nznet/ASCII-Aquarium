#include "sim/Clock.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "renderer/Color.h"
#include "renderer/Font.h"

namespace aq {

namespace {

constexpr int kScreenW = 320;
constexpr int kScreenH = 240;

// ASCII clock layout (upstream constants).
constexpr int kAsciiClockRows = 6;
constexpr int kAsciiClockGlyphGap = 1;
constexpr int kAsciiClockCharW = 6;
constexpr int kAsciiClockRowH = 10;
constexpr int kAsciiClockY = 68;

constexpr int kFlipSpriteW = 192;
constexpr int kFlipSpriteH = 20;
constexpr std::uint16_t kClockFlipTransparent = 0x0801;  // key colour outside the palette

const std::uint16_t kDefaultSmallClockColor = TFT_WHITE;
const std::uint16_t kDefaultAsciiClockColor = RGB565(0, 20, 95);

// Selectable clock text colors (upstream kClockColorPalette).
const std::uint16_t kClockColorPalette[] = {
    kDefaultAsciiClockColor, TFT_WHITE, TFT_LIGHTGREY, TFT_DARKGREY,
    RGB565(255, 40, 40),   RGB565(255, 128, 20), RGB565(255, 220, 40), RGB565(160, 255, 40),
    RGB565(40, 220, 80),   RGB565(0, 180, 120),  RGB565(0, 220, 220),  RGB565(80, 180, 255),
    RGB565(40, 80, 255),   RGB565(120, 80, 255), RGB565(220, 60, 255), RGB565(255, 120, 200),
    RGB565(100, 0, 0),     RGB565(130, 56, 0),   RGB565(116, 108, 18), RGB565(0, 82, 30),
    RGB565(0, 76, 76),     RGB565(0, 0, 156),    RGB565(58, 20, 120),  RGB565(156, 120, 255)};
constexpr int kClockColorCount =
    static_cast<int>(sizeof(kClockColorPalette) / sizeof(kClockColorPalette[0]));

template <typename T>
T clampVal(T v, T lo, T hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

struct AsciiClockGlyph {
    char c;
    const char* rows[kAsciiClockRows];
};

// FIGlet-style digits, ':', space, and am/pm letters (upstream table verbatim).
const AsciiClockGlyph kAsciiGlyphs[] = {
    {'0', {"  ___", " / _ \\", "| | | |", "| |_| |", " \\___/", ""}},
    {'1', {" _", "/ |", "| |", "| |", "|_|", ""}},
    {'2', {" ____", "|___ \\", "  __) |", " / __/", "|_____|", ""}},
    {'3', {" _____", "|___ /", "  |_ \\", " ___) |", "|____/", ""}},
    {'4', {" _  _", "| || |", "| || |_", "|__   _|", "   |_|", ""}},
    {'5', {" ____", "| ___|", "|___ \\", " ___) |", "|____/", ""}},
    {'6', {"  __", " / /_", "| '_ \\", "| (_) |", " \\___/", ""}},
    {'7', {" _____", "|___  |", "   / /", "  / /", " /_/", ""}},
    {'8', {"  ___", " ( _ )", " / _ \\", "| (_) |", " \\___/", ""}},
    {'9', {"  ___", " / _ \\", "| (_) |", " \\__, |", "  /_/", ""}},
    {':', {" ", " _", "(_)", " _", "(_)", " "}},
    {' ', {" ", " ", " ", " ", " ", " "}},
    {'a', {"", "  __ _", " / _` |", "| (_| |", " \\__,_|", ""}},
    {'m', {"", " _ __ ___", "| '_ ` _ \\", "| | | | | |", "|_| |_| |_|", ""}},
    {'p', {"", " _ __", "| '_ \\", "| |_) |", "| .__/", "|_|"}},
};
constexpr int kAsciiGlyphCount = static_cast<int>(sizeof(kAsciiGlyphs) / sizeof(kAsciiGlyphs[0]));

const AsciiClockGlyph& asciiGlyphFor(char c) {
    for (int i = 0; i < kAsciiGlyphCount; ++i) {
        if (kAsciiGlyphs[i].c == c) return kAsciiGlyphs[i];
    }
    return kAsciiGlyphs[11];  // space
}

int asciiGlyphWidth(const AsciiClockGlyph& glyph) {
    int width = 1;
    for (int row = 0; row < kAsciiClockRows; ++row) {
        int rowWidth = static_cast<int>(std::strlen(glyph.rows[row]));
        if (rowWidth > width) width = rowWidth;
    }
    return width;
}

int asciiTextCols(const char* text) {
    int cols = 0;
    for (std::size_t i = 0; text[i] != '\0'; ++i) {
        if (i > 0) cols += kAsciiClockGlyphGap;
        cols += asciiGlyphWidth(asciiGlyphFor(text[i]));
    }
    return cols;
}

void appendCharSafe(char* out, std::size_t outCap, char c) {
    std::size_t len = std::strlen(out);
    if (len + 1 >= outCap) return;
    out[len] = c;
    out[len + 1] = '\0';
}

void appendAsciiGlyphRow(char* out, std::size_t outCap, const AsciiClockGlyph& glyph, int row) {
    int width = asciiGlyphWidth(glyph);
    int rowLen = static_cast<int>(std::strlen(glyph.rows[row]));
    for (int col = 0; col < width; ++col) {
        appendCharSafe(out, outCap, col < rowLen ? glyph.rows[row][col] : ' ');
    }
}

void trimTrailingSpaces(char* out) {
    int len = static_cast<int>(std::strlen(out));
    while (len > 0 && out[len - 1] == ' ') out[--len] = '\0';
}

char mirroredClockChar(char c) {
    switch (c) {
        case '/':  return '\\';
        case '\\': return '/';
        case '(':  return ')';
        case ')':  return '(';
        case '[':  return ']';
        case ']':  return '[';
        case '{':  return '}';
        case '}':  return '{';
        case '<':  return '>';
        case '>':  return '<';
        default:   return c;
    }
}

void mirrorClockTextInPlace(char* text) {
    int len = static_cast<int>(std::strlen(text));
    for (int i = 0; i < len / 2; ++i) {
        char left = mirroredClockChar(text[i]);
        char right = mirroredClockChar(text[len - 1 - i]);
        text[i] = right;
        text[len - 1 - i] = left;
    }
    if (len % 2 == 1) text[len / 2] = mirroredClockChar(text[len / 2]);
}

}  // namespace

int Clock::paletteCount() { return kClockColorCount; }

std::uint16_t Clock::paletteColor(int i) {
    if (i < 0 || i >= kClockColorCount) return kDefaultSmallClockColor;
    return kClockColorPalette[i];
}

Clock::Clock()
    : smallTextColor_(kDefaultSmallClockColor),
      asciiTextColor_(kDefaultAsciiClockColor),
      flipSprite_(kFlipSpriteW, kFlipSpriteH) {}

void Clock::update() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    month_ = local.tm_mon + 1;
    day_ = local.tm_mday;
    hour_ = local.tm_hour;
    minute_ = local.tm_min;
}

void Clock::formatClockDisplay(char* out, std::size_t outCap) const {
    static const char* monthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (use24Hour_) {
        std::snprintf(out, outCap, "%s %02d  %02d:%02d", monthNames[month_ - 1], day_, hour_, minute_);
    } else {
        int h = hour_ % 12;
        if (h == 0) h = 12;
        std::snprintf(out, outCap, "%s %02d  %d:%02d %s", monthNames[month_ - 1], day_, h, minute_,
                      hour_ < 12 ? "AM" : "PM");
    }
}

void Clock::formatClockTimeOnly(char* out, std::size_t outCap, bool includeMeridiem) const {
    if (use24Hour_) {
        std::snprintf(out, outCap, "%02d:%02d", hour_, minute_);
    } else {
        int h = hour_ % 12;
        if (h == 0) h = 12;
        if (includeMeridiem) {
            std::snprintf(out, outCap, "%d:%02d %s", h, minute_, hour_ < 12 ? "am" : "pm");
        } else {
            std::snprintf(out, outCap, "%d:%02d", h, minute_);
        }
    }
}

void Clock::drawBackgroundLayer(Framebuffer& fb) {
    if (!visible_ || style_ != ClockStyle::Ascii) return;

    char timeText[16];
    formatClockTimeOnly(timeText, sizeof(timeText), false);
    int artCols = asciiTextCols(timeText);
    int artPixelW = artCols * kAsciiClockCharW;
    int x = (kScreenW - artPixelW) / 2;
    if (x < 0) x = 0;
    int y = kAsciiClockY;

    fb.setTextFont(1);
    fb.setTextSize(1);
    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(asciiTextColor_);

    for (int row = 0; row < kAsciiClockRows; ++row) {
        char rowBuf[96] = "";
        for (std::size_t i = 0; timeText[i] != '\0'; ++i) {
            if (i > 0) {
                for (int gap = 0; gap < kAsciiClockGlyphGap; ++gap)
                    appendCharSafe(rowBuf, sizeof(rowBuf), ' ');
            }
            appendAsciiGlyphRow(rowBuf, sizeof(rowBuf), asciiGlyphFor(timeText[i]), row);
        }
        if (flipHorizontal_) mirrorClockTextInPlace(rowBuf);
        trimTrailingSpaces(rowBuf);
        if (rowBuf[0] != '\0') fb.drawString(rowBuf, x, y + row * kAsciiClockRowH);
    }

    fb.setTextFont(2);
}

void Clock::drawMirroredSmallClock(Framebuffer& fb, const char* line, int y) {
    flipSprite_.setTextFont(2);
    flipSprite_.setTextSize(1);
    flipSprite_.setTextDatum(TL_DATUM);
    flipSprite_.fillScreen(kClockFlipTransparent);
    std::uint16_t transparentKey = flipSprite_.readPixel(0, kFlipSpriteH - 1);
    flipSprite_.setTextColor(smallTextColor_, kClockFlipTransparent);
    flipSprite_.drawString(line, 0, 0);

    int textW = flipSprite_.textWidth(line);
    if (textW <= 0) return;
    int drawW = clampVal(textW, 1, kFlipSpriteW);
    int destX = (kScreenW - drawW) / 2;

    for (int py = 0; py < kFlipSpriteH; ++py) {
        int destY = y + py;
        if (destY < 0 || destY >= kScreenH) continue;
        for (int px = 0; px < drawW; ++px) {
            std::uint16_t color = flipSprite_.readPixel(px, py);
            if (color != transparentKey) {
                fb.drawPixel(destX + drawW - 1 - px, destY, color);
            }
        }
    }
}

void Clock::drawOverlay(Framebuffer& fb) {
    if (!visible_ || style_ != ClockStyle::SmallText) return;
    char line[32];
    formatClockDisplay(line, sizeof(line));
    fb.setTextFont(2);
    fb.setTextSize(1);
    fb.setTextDatum(TC_DATUM);
    fb.setTextColor(smallTextColor_);
    int y = (smallPosition_ == ClockSmallPosition::Top) ? 4 : (kScreenH - 18);
    if (flipHorizontal_) {
        drawMirroredSmallClock(fb, line, y);
    } else {
        fb.drawString(line, kScreenW / 2, y);
    }
}

}  // namespace aq
