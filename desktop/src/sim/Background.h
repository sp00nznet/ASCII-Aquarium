// Aquarium backgrounds — the four modes the upstream sketch offers, drawn
// behind the live scene.
//
// Ported from drawBackground() and friends in the sketch. The dithered vertical
// gradient (blue/purple) is cached once per mode; the pixel-flower mode caches
// its petal geometry until the flowers are re-randomized. BG_COLOR is black.

#pragma once

#include <cstdint>

#include "sim/Rng.h"

namespace aq {

class Framebuffer;

// Order matches the upstream BackgroundMode enum so the Settings panel can
// cycle through them by index.
enum class BackgroundMode : int {
    Black = 0,
    BlueGradient = 1,
    PurpleGradient = 2,
    Flowers = 3,
    Count = 4,
};

class Background {
public:
    explicit Background(Rng& rng);

    // Paint the background for `mode` into `fb` (full-screen). Gradient bands
    // are rebuilt only when the mode changes; flower geometry only when dirty.
    void draw(Framebuffer& fb, BackgroundMode mode);

    // Re-roll the three pixel flowers' positions/sizes/colors (Settings hook).
    void randomizeFlowers();

private:
    struct FlowerSpec {
        int cx, cy, radius;
        float rotation;
        std::uint16_t color;
    };
    struct FlowerPoint {
        std::int16_t x, y;
    };

    static constexpr int kGradientBandH = 240 / 4;  // SCREEN_H / 4
    static constexpr int kScreenWidth = 320;
    static constexpr int kFlowerCount = 3;
    static constexpr int kFlowerSegments = 80;

    void rebuildGradientBand(BackgroundMode mode);
    void rebuildFlowerGeometry();

    Rng& rng_;

    // Cached dithered gradient band (native RGB565), valid for cachedMode_.
    std::uint16_t band_[kScreenWidth * kGradientBandH];
    int cachedMode_ = -1;

    FlowerSpec  flowers_[kFlowerCount];
    FlowerPoint flowerPoints_[kFlowerCount][kFlowerSegments + 1];
    bool flowerGeometryDirty_ = true;
};

}  // namespace aq
