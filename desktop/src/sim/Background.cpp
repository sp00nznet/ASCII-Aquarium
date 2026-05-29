#include "sim/Background.h"

#include <cmath>

#include "renderer/Color.h"
#include "renderer/Framebuffer.h"
#include "sim/Aquarium.h"  // kScreenW / kScreenH / kSeaLevelY

namespace aq {

namespace {

constexpr std::uint16_t kBgColor = TFT_BLACK;
constexpr float kTwoPi = 6.28318f;

template <typename T>
T clampVal(T v, T lo, T hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

int color5To8(int v5) { return (v5 << 3) | (v5 >> 2); }
int color6To8(int v6) { return (v6 << 2) | (v6 >> 4); }

int bayer8Threshold(int x, int y, int scale) {
    static const std::uint8_t kBayer8x8[64] = {
        0, 48, 12, 60, 3, 51, 15, 63,
        32, 16, 44, 28, 35, 19, 47, 31,
        8, 56, 4, 52, 11, 59, 7, 55,
        40, 24, 36, 20, 43, 27, 39, 23,
        2, 50, 14, 62, 1, 49, 13, 61,
        34, 18, 46, 30, 33, 17, 45, 29,
        10, 58, 6, 54, 9, 57, 5, 53,
        42, 26, 38, 22, 41, 25, 37, 21};
    int sx = x / scale;
    int sy = y / scale;
    return kBayer8x8[(sy & 7) * 8 + (sx & 7)] << 2;
}

void gradientRgbAtT(const std::uint16_t* colors, const std::uint8_t* stops, int count,
                    int t255, int& r8, int& g8, int& b8) {
    if (count <= 0) {
        r8 = g8 = b8 = 0;
        return;
    }
    if (t255 <= stops[0]) {
        r8 = color5To8((colors[0] >> 11) & 0x1F);
        g8 = color6To8((colors[0] >> 5) & 0x3F);
        b8 = color5To8(colors[0] & 0x1F);
        return;
    }
    if (t255 >= stops[count - 1]) {
        r8 = color5To8((colors[count - 1] >> 11) & 0x1F);
        g8 = color6To8((colors[count - 1] >> 5) & 0x3F);
        b8 = color5To8(colors[count - 1] & 0x1F);
        return;
    }
    for (int i = 1; i < count; ++i) {
        if (t255 <= stops[i]) {
            int t0 = stops[i - 1];
            int t1 = stops[i];
            int seg = t1 - t0;
            int blend = (seg > 0) ? ((t255 - t0) * 255) / seg : 255;
            int inv = 255 - blend;
            int c0r = color5To8((colors[i - 1] >> 11) & 0x1F);
            int c0g = color6To8((colors[i - 1] >> 5) & 0x3F);
            int c0b = color5To8(colors[i - 1] & 0x1F);
            int c1r = color5To8((colors[i] >> 11) & 0x1F);
            int c1g = color6To8((colors[i] >> 5) & 0x3F);
            int c1b = color5To8(colors[i] & 0x1F);
            r8 = (c0r * inv + c1r * blend) / 255;
            g8 = (c0g * inv + c1g * blend) / 255;
            b8 = (c0b * inv + c1b * blend) / 255;
            return;
        }
    }
}

// Gradient stops + palettes, copied from drawBackground() in the sketch.
const std::uint8_t kGradientStops[] = {0, 18, 42, 74, 112, 156, 204, 232, 255};
const std::uint16_t kBlueGradientColors[] = {
    RGB565(0, 8, 255), RGB565(0, 6, 228), RGB565(0, 5, 198), RGB565(0, 4, 164),
    RGB565(0, 3, 126), RGB565(0, 2, 90),  RGB565(0, 1, 58),  RGB565(0, 0, 30),
    kBgColor};
const std::uint16_t kPurpleGradientColors[] = {
    RGB565(108, 6, 220), RGB565(92, 4, 194), RGB565(74, 3, 166), RGB565(58, 2, 136),
    RGB565(42, 2, 104),  RGB565(28, 1, 72),  RGB565(18, 0, 46),  RGB565(8, 0, 22),
    kBgColor};
constexpr int kGradientStopCount = 9;

// Default flower layout (kDefaultPixelFlowers in the sketch).
struct DefaultFlower { int cx, cy, radius; float rotation; std::uint16_t color; };
const DefaultFlower kDefaultFlowers[3] = {
    {70, 70, 58, 6.28318f, RGB565(0, 26, 76)},
    {248, 72, 58, 6.82318f, RGB565(0, 22, 66)},
    {202, 156, 28, 3.14159f, RGB565(116, 108, 18)},
};

void drawThickLine(Framebuffer& fb, int x0, int y0, int x1, int y1, std::uint16_t color) {
    fb.drawLine(x0, y0, x1, y1, color);
    fb.drawLine(x0 + 1, y0, x1 + 1, y1, color);
    fb.drawLine(x0, y0 + 1, x1, y1 + 1, color);
    fb.drawLine(x0 - 1, y0, x1 - 1, y1, color);
    fb.drawLine(x0, y0 - 1, x1, y1 - 1, color);
}

}  // namespace

Background::Background(Rng& rng) : rng_(rng) {
    for (int i = 0; i < kFlowerCount; ++i) {
        flowers_[i].cx = kDefaultFlowers[i].cx;
        flowers_[i].cy = kDefaultFlowers[i].cy;
        flowers_[i].radius = kDefaultFlowers[i].radius;
        flowers_[i].rotation = kDefaultFlowers[i].rotation;
        flowers_[i].color = kDefaultFlowers[i].color;
    }
}

void Background::rebuildGradientBand(BackgroundMode mode) {
    const std::uint16_t* colors = (mode == BackgroundMode::PurpleGradient)
                                      ? kPurpleGradientColors
                                      : kBlueGradientColors;
    const int ditherScale = 4;
    const int ditherAmplitude = 28;
    for (int y = 0; y < kGradientBandH; ++y) {
        int baseT255 = (y * 255) / (kGradientBandH - 1);
        for (int x = 0; x < kScreenWidth; ++x) {
            int threshold = bayer8Threshold(x, y, ditherScale) - 128;
            int sampleT255 = clampVal(baseT255 + (threshold * ditherAmplitude) / 128, 0, 255);
            int r8, g8, b8;
            gradientRgbAtT(colors, kGradientStops, kGradientStopCount, sampleT255, r8, g8, b8);
            band_[y * kScreenWidth + x] = rgb565From888(r8, g8, b8);
        }
    }
}

void Background::rebuildFlowerGeometry() {
    if (!flowerGeometryDirty_) return;
    for (int fi = 0; fi < kFlowerCount; ++fi) {
        const FlowerSpec& f = flowers_[fi];
        for (int i = 0; i <= kFlowerSegments; ++i) {
            float theta = f.rotation + (kTwoPi * i) / kFlowerSegments;
            float petal = 0.5f + 0.5f * std::sin(theta * 5.0f);
            float r = f.radius * (0.58f + 0.42f * petal);
            flowerPoints_[fi][i].x =
                static_cast<std::int16_t>(f.cx + static_cast<int>(std::cos(theta) * r));
            flowerPoints_[fi][i].y =
                static_cast<std::int16_t>(f.cy + static_cast<int>(std::sin(theta) * r));
        }
    }
    flowerGeometryDirty_ = false;
}

void Background::randomizeFlowers() {
    auto colorFor = [&](int index) -> std::uint16_t {
        if (index == kFlowerCount - 1) {
            return rgb565(static_cast<std::uint8_t>(rng_.random(92, 146)),
                          static_cast<std::uint8_t>(rng_.random(84, 126)),
                          static_cast<std::uint8_t>(rng_.random(8, 28)));
        }
        return rgb565(static_cast<std::uint8_t>(rng_.random(0, 10)),
                      static_cast<std::uint8_t>(rng_.random(18, 38)),
                      static_cast<std::uint8_t>(rng_.random(46, 96)));
    };
    flowers_[0].cx = static_cast<int>(rng_.random(42, 98));
    flowers_[0].cy = static_cast<int>(rng_.random(50, 86));
    flowers_[0].radius = static_cast<int>(rng_.random(50, 63));
    flowers_[0].rotation = rng_.frand(0.0f, kTwoPi);
    flowers_[0].color = colorFor(0);

    flowers_[1].cx = static_cast<int>(rng_.random(214, 278));
    flowers_[1].cy = static_cast<int>(rng_.random(48, 90));
    flowers_[1].radius = static_cast<int>(rng_.random(50, 63));
    flowers_[1].rotation = rng_.frand(0.0f, kTwoPi);
    flowers_[1].color = colorFor(1);

    flowers_[2].cx = static_cast<int>(rng_.random(162, 248));
    flowers_[2].cy = static_cast<int>(rng_.random(124, kSeaLevelY - 18));
    flowers_[2].radius = static_cast<int>(rng_.random(20, 34));
    flowers_[2].rotation = rng_.frand(0.0f, kTwoPi);
    flowers_[2].color = colorFor(2);

    flowerGeometryDirty_ = true;
}

void Background::draw(Framebuffer& fb, BackgroundMode mode) {
    switch (mode) {
        case BackgroundMode::BlueGradient:
        case BackgroundMode::PurpleGradient: {
            int m = static_cast<int>(mode);
            if (cachedMode_ != m) {
                rebuildGradientBand(mode);
                cachedMode_ = m;
            }
            fb.fillScreen(kBgColor);
            fb.pushImage(0, 0, kScreenWidth, kGradientBandH, band_);
            break;
        }
        case BackgroundMode::Flowers:
            fb.fillScreen(kBgColor);
            rebuildFlowerGeometry();
            for (int fi = 0; fi < kFlowerCount; ++fi) {
                for (int i = 1; i <= kFlowerSegments; ++i) {
                    const FlowerPoint& a = flowerPoints_[fi][i - 1];
                    const FlowerPoint& b = flowerPoints_[fi][i];
                    drawThickLine(fb, a.x, a.y, b.x, b.y, flowers_[fi].color);
                }
            }
            break;
        case BackgroundMode::Black:
        default:
            fb.fillScreen(kBgColor);
            break;
    }
}

}  // namespace aq
