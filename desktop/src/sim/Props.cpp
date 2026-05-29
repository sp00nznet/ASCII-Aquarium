#include "sim/Props.h"

#include <cmath>
#include <cstring>

#include "renderer/Color.h"
#include "renderer/Font.h"
#include "renderer/Framebuffer.h"

namespace aq {

namespace {

constexpr int kScreenH = 240;
constexpr int kCellW = 6;   // Font 1 advance width
constexpr int kRowH = 8;

// Draw fixed-width ASCII art, one Font-1 glyph per cell, top-left at (x, y).
void drawArt(Framebuffer& fb, const char* const* rows, int nRows, int x, int y,
             std::uint16_t color) {
    fb.setTextFont(1);
    fb.setTextSize(1);
    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(color);
    for (int r = 0; r < nRows; ++r) {
        const char* line = rows[r];
        for (int c = 0; line[c] != '\0'; ++c) {
            if (line[c] != ' ') {
                fb.drawChar(static_cast<std::uint16_t>(line[c]),
                            x + c * kCellW, y + r * kRowH);
            }
        }
    }
    fb.setTextFont(2);
}

// ---- art blocks ----
const char* kCastle[] = {
    "[^]_[^]",
    "|     |",
    "| [#] |",
    "|_____|",
};
const char* kChest[] = {
    " ____ ",
    "|$$$$|",
    "|_##_|",
};
const char* kCoral[] = {
    "\\|/",
    " Y ",
    " | ",
};
const char* kRock[] = {
    " __ ",
    "(##)",
};

// Bottom-aligned placements on the sand.
constexpr int kCastleX = 130, kCastleRows = 4;
constexpr int kChestX = 256,  kChestRows = 3;
constexpr int kCoralX = 22,   kCoralRows = 3;
constexpr int kRockX = 92,    kRockRows = 2;

int bottomY(int rows) { return kScreenH - 2 - rows * kRowH; }

}  // namespace

Props::Props() {
    for (int i = 0; i < kChestBubbles; ++i) {
        bubbleY_[i] = i * 9.0f;  // stagger their starting heights
    }
}

void Props::update(float dt) {
    if (!enabled_) return;
    tSec_ += dt;
    for (int i = 0; i < kChestBubbles; ++i) {
        bubbleY_[i] += (10.0f + i * 1.5f) * dt;   // rise
        if (bubbleY_[i] > 34.0f) bubbleY_[i] -= 34.0f;
    }
}

void Props::draw(Framebuffer& fb) const {
    if (!enabled_) return;

    drawArt(fb, kCoral,  kCoralRows,  kCoralX,  bottomY(kCoralRows),  TFT_PINK);
    drawArt(fb, kRock,   kRockRows,   kRockX,   bottomY(kRockRows),   TFT_DARKGREY);
    drawArt(fb, kCastle, kCastleRows, kCastleX, bottomY(kCastleRows), aq::rgb565(210, 180, 120));
    drawArt(fb, kChest,  kChestRows,  kChestX,  bottomY(kChestRows),  TFT_GOLD);

    // Bubbles drifting up out of the chest mouth.
    const int chestTop = bottomY(kChestRows);
    const int mouthX = kChestX + 3 * kCellW;
    fb.setTextFont(2);
    fb.setTextDatum(MC_DATUM);
    fb.setTextColor(aq::rgb565(120, 180, 220));
    for (int i = 0; i < kChestBubbles; ++i) {
        int bx = mouthX + static_cast<int>(std::sin(tSec_ * 1.6f + i) * 3.0f);
        int by = chestTop - static_cast<int>(bubbleY_[i]);
        fb.drawString("o", bx, by);
    }
}

}  // namespace aq
