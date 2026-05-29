#include "sim/Aquarium.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "renderer/Color.h"
#include "renderer/Font.h"
#include "renderer/Framebuffer.h"

namespace aq {

namespace {

// ---- Tunables, copied verbatim from the upstream sketch ----
constexpr int   kMinFish = 6;
constexpr int   kMaxFish = 36;
constexpr int   kDefaultFish = 16;
constexpr int   kMinBubbles = 0;
constexpr int   kDefaultBubbles = 10;
constexpr int   kDefaultOctopusFrequency = 1;
constexpr int   kDefaultSeahorseFrequency = 1;
constexpr float kDefaultSway = 1.10f;
constexpr float kDefaultSeaweedLength = 1.35f;
constexpr float kDefaultSeaweedLengthRandomness = 0.35f;
constexpr float kMinSway = 0.25f;
constexpr float kMaxSway = 2.5f;
constexpr float kMinSeaweedLength = 0.80f;
constexpr float kMaxSeaweedLength = 1.60f;
constexpr float kMinSeaweedRandomness = 0.00f;
constexpr float kMaxSeaweedRandomness = 0.50f;

constexpr float kFishSwimWaveAmplitude = 1.5f;
constexpr float kFishSwimWaveSpeed = 5.6f;
constexpr float kFishSwimWaveSpacing = 0.85f;

constexpr float kFishAvoidRadiusX = 52.0f;
constexpr float kFishAvoidRadiusY = 20.0f;
constexpr float kFishAvoidStrength = 4.2f;
constexpr float kFishCenterYOffset = 7.0f;

constexpr float kOctopusExitPad = 42.0f;
constexpr float kOctopusCenterYOffset = 8.0f;
constexpr float kOctopusFishAvoidRadiusX = 76.0f;
constexpr float kOctopusFishAvoidRadiusY = 34.0f;
constexpr float kOctopusFishAvoidStrength = 8.0f;
constexpr float kOctopusFishClearRadiusX = 46.0f;
constexpr float kOctopusFishClearRadiusY = 22.0f;

constexpr float kSeahorseExitPad = 48.0f;
constexpr float kSeahorseCenterXOffset = 15.0f;
constexpr float kSeahorseCenterYOffset = 24.0f;
constexpr float kSeahorseFishAvoidRadiusX = 58.0f;
constexpr float kSeahorseFishAvoidRadiusY = 38.0f;
constexpr float kSeahorseFishAvoidStrength = 6.0f;
constexpr float kSeahorseFishClearRadiusX = 34.0f;
constexpr float kSeahorseFishClearRadiusY = 28.0f;
constexpr float kSeahorseSpeedBoost = 1.18f;
constexpr float kVisitorClearRadiusX = 56.0f;
constexpr float kVisitorClearRadiusY = 38.0f;

constexpr float kTwoPi = 6.28318f;

template <typename T>
T clampVal(T v, T lo, T hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

// Visitor spawn frequency options (per hour) and nearest-match normalization.
const int kOctopusFreqOptions[] = {1, 2, 4, 6, 12, 60};
const int kSeahorseFreqOptions[] = {1, 2, 4, 6, 12, 60};

int normalizeFrequency(int value, const int* opts, int count) {
    int best = opts[0];
    int bestDiff = std::abs(value - best);
    for (int i = 1; i < count; ++i) {
        int diff = std::abs(value - opts[i]);
        if (diff < bestDiff) {
            best = opts[i];
            bestDiff = diff;
        }
    }
    return best;
}

struct FishSpecies {
    const char* right;
    std::uint16_t baseColor;
};

// Printable ASCII only (Font 2 safe), faces right; mirrored at boot for vx < 0.
const FishSpecies kFishSpecies[kGlyphCount] = {
    {/* Small Green */ "><>", RGB565(80, 200, 120)},
    {/* Blue Dart */ ">)))'>", RGB565(0, 150, 255)},
    {/* Pink Bubble */ "oO0", TFT_PINK},
    {/* Golden Emperor */ "><((( '>", RGB565(255, 184, 0)},
    {/* Purple Jellyfish */ "~~{o}", TFT_VIOLET},
    {/* Red Snapper */ "><(((o>", TFT_RED},
    {/* Orange Wrasse */ "><((((>`", TFT_ORANGE},
    {/* Teal Glider */ "><((( '>", RGB565(0, 180, 170)},
    {/* Royal Indigo */ "}>{{{{* >", RGB565(75, 0, 156)},
    {/* Lilac Starfish */ "><((( *>", RGB565(200, 120, 255)},
    {/* Pink Tetra */ ">(')>", RGB565(255, 158, 200)},
    {/* Yellow Minnow */ ">'>", TFT_YELLOW},
};

// Occasional non-canonical hue for variety (~1 in 5 fish).
const std::uint16_t kAltFishColors[] = {
    TFT_CYAN, TFT_MAGENTA, TFT_WHITE, TFT_SKYBLUE, TFT_GOLD,
    TFT_ORANGE, TFT_GREENYELLOW, TFT_DARKGREY};
constexpr int kAltFishColorCount =
    static_cast<int>(sizeof(kAltFishColors) / sizeof(kAltFishColors[0]));

char mirrorAsciiBracket(char c) {
    switch (c) {
        case '>': return '<';
        case '<': return '>';
        case '(': return ')';
        case ')': return '(';
        case '{': return '}';
        case '}': return '{';
        case '[': return ']';
        case ']': return '[';
        default:  return c;
    }
}

bool buildMirroredGlyph(const char* right, char* leftOut, std::size_t outCap) {
    std::size_t n = std::strlen(right);
    if (n == 0 || n + 1 > outCap) return false;
    for (std::size_t i = 0; i < n; ++i) {
        unsigned char u = static_cast<unsigned char>(right[i]);
        if (u < 32 || u > 126) return false;
        leftOut[i] = mirrorAsciiBracket(right[n - 1 - i]);
    }
    leftOut[n] = '\0';
    return true;
}

char mirrorSeahorseGlyph(char glyph) {
    switch (glyph) {
        case '/':  return '\\';
        case '\\': return '/';
        case '[':  return ']';
        case ']':  return '[';
        case '(':  return ')';
        case ')':  return '(';
        case '<':  return '>';
        case '>':  return '<';
        default:   return glyph;
    }
}

std::uint16_t randomBubbleColor(Rng& rng) {
    return rgb565(static_cast<std::uint8_t>(rng.random(0, 5)),
                  static_cast<std::uint8_t>(rng.random(8, 24)),
                  static_cast<std::uint8_t>(rng.random(45, 106)));
}

std::uint16_t randomFoodColor(Rng& rng) {
    return rgb565(static_cast<std::uint8_t>(rng.random(220, 256)),
                  static_cast<std::uint8_t>(rng.random(118, 166)),
                  static_cast<std::uint8_t>(rng.random(0, 34)));
}

// Sample a point along a seaweed blade at parameter u in [0, 1].
void seaweedPointAt(float u, float bx, int y0, float bladeHeight, float sway,
                    float tSec, int bladeIndex, float swaySpeed,
                    float& x, float& y) {
    u = clampVal(u, 0.0f, 1.0f);
    float bodyWave = std::sin(tSec * (1.05f + bladeIndex * 0.025f) * swaySpeed -
                              u * 5.1f + bladeIndex * 0.72f);
    float ripple = std::sin(tSec * 0.72f * swaySpeed + u * 9.0f + bladeIndex * 1.31f);
    float bend = sway * u * (0.20f + u * 0.80f);
    float travel = bodyWave * (1.5f + bladeHeight * 0.055f) * u * u;
    float detail = ripple * 1.2f * u;
    x = bx + bend + travel + detail;
    y = y0 - bladeHeight * u;
}

}  // namespace

Aquarium::Aquarium(Rng& rng)
    : rng_(rng),
      fishTargetCount_(kDefaultFish),
      bubbleTargetCount_(kDefaultBubbles),
      octopusFrequency_(kDefaultOctopusFrequency),
      seahorseFrequency_(kDefaultSeahorseFrequency),
      seaweedSwaySpeed_(kDefaultSway),
      seaweedLength_(kDefaultSeaweedLength),
      seaweedLengthRandomness_(kDefaultSeaweedLengthRandomness) {
    std::memset(fishMirroredLeft_, 0, sizeof(fishMirroredLeft_));
}

// ---------------------------- glyph metrics --------------------------------

void Aquarium::initFishMirrors() {
    for (int i = 0; i < kGlyphCount; ++i) {
        if (!buildMirroredGlyph(kFishSpecies[i].right, fishMirroredLeft_[i], kFishGlyphBuf)) {
            std::strncpy(fishMirroredLeft_[i], kFishSpecies[i].right, kFishGlyphBuf - 1);
            fishMirroredLeft_[i][kFishGlyphBuf - 1] = '\0';
        }
    }
}

void Aquarium::initFishGlyphMetrics() {
    // Mirrors cacheGlyphMetrics: per-prefix Font 2 widths give per-char offsets.
    auto cache = [](const char* txt, std::uint8_t& lenOut, std::int16_t& widthOut,
                    std::int16_t* offsetsOut) {
        char prefix[kFishGlyphBuf];
        std::size_t len = std::strlen(txt);
        if (len >= kFishGlyphBuf) len = kFishGlyphBuf - 1;
        for (std::size_t c = 0; c < len; ++c) {
            std::memcpy(prefix, txt, c);
            prefix[c] = '\0';
            offsetsOut[c] =
                static_cast<std::int16_t>(font::text_width(FontId::Font2, prefix, 1));
        }
        lenOut = static_cast<std::uint8_t>(len);
        widthOut = static_cast<std::int16_t>(font::text_width(FontId::Font2, txt, 1));
    };

    for (int i = 0; i < kGlyphCount; ++i) {
        cache(kFishSpecies[i].right, fishGlyphLenRight_[i], fishGlyphWidthRight_[i],
              fishGlyphOffsetRight_[i]);
        cache(fishMirroredLeft_[i], fishGlyphLenLeft_[i], fishGlyphWidthLeft_[i],
              fishGlyphOffsetLeft_[i]);
    }
}

const char* Aquarium::fishGlyphDrawing(const Fish& f) const {
    return (f.vx >= 0.0f) ? kFishSpecies[f.type].right : fishMirroredLeft_[f.type];
}

const std::int16_t* Aquarium::fishGlyphOffsets(const Fish& f) const {
    return (f.vx >= 0.0f) ? fishGlyphOffsetRight_[f.type] : fishGlyphOffsetLeft_[f.type];
}

std::uint8_t Aquarium::fishGlyphLength(const Fish& f) const {
    return (f.vx >= 0.0f) ? fishGlyphLenRight_[f.type] : fishGlyphLenLeft_[f.type];
}

int Aquarium::fishVisualWidth(const Fish& f) const {
    if (f.visualWidth > 0) return f.visualWidth;
    return static_cast<int>(std::strlen(fishGlyphDrawing(f))) * 12;
}

int Aquarium::activeFishLimit() const {
    return clampVal(fishTargetCount_, 0, kMaxFishPool);
}

int Aquarium::activeBubbleLimit() const {
    return clampVal(bubbleTargetCount_, 0, kMaxBubbles);
}

// ---------------------------- population -----------------------------------

float Aquarium::randomFishDepthBrightness() {
    int roll = static_cast<int>(rng_.random(100));
    if (roll < 28) return rng_.frand(0.48f, 0.64f);
    if (roll < 70) return rng_.frand(0.66f, 0.84f);
    return rng_.frand(0.88f, 1.0f);
}

void Aquarium::refreshFishRenderColor(Fish& f) {
    f.renderColor = scaleRgb565(f.displayColor, f.depthBrightness);
}

void Aquarium::refreshFishDepth(Fish& f) {
    f.depthBrightness = randomFishDepthBrightness();
    refreshFishRenderColor(f);
}

void Aquarium::activateFish(Fish& f, bool activeNow) {
    f.active = activeNow;
    if (!activeNow) return;
    f.type = static_cast<int>(rng_.random(0, kGlyphCount));
    int rightWidth = fishGlyphWidthRight_[f.type];
    int leftWidth = fishGlyphWidthLeft_[f.type];
    f.visualWidth = (rightWidth > leftWidth) ? rightWidth : leftWidth;
    if (f.visualWidth <= 0)
        f.visualWidth = static_cast<int>(std::strlen(kFishSpecies[f.type].right)) * 12;
    f.displayColor = kFishSpecies[f.type].baseColor;
    if (rng_.random(100) < 20) {
        f.displayColor = kAltFishColors[rng_.random(0, kAltFishColorCount)];
    }
    refreshFishDepth(f);
    f.x = rng_.frand(-42, kScreenW + 12);
    f.y = rng_.frand(20, kSeaLevelY - 10);
    f.vx = rng_.frand(-1.0f, 1.0f);
    f.vy = rng_.frand(-0.5f, 0.5f);
    f.speed = rng_.frand(14.0f, 30.0f);
    f.phase = rng_.frand(0.0f, kTwoPi);
    f.wanderBias = rng_.frand(0.4f, 1.3f);
}

void Aquarium::applyFishPopulation() {
    fishTargetCount_ = clampVal(fishTargetCount_, kMinFish, kMaxFish);
    for (int i = 0; i < kMaxFishPool; ++i) {
        bool shouldBeActive = (i < fishTargetCount_);
        if (shouldBeActive && !fishPool_[i].active) activateFish(fishPool_[i], true);
        if (!shouldBeActive && fishPool_[i].active) fishPool_[i].active = false;
    }
}

bool Aquarium::fishSpawnClear(int fishIndex, float x, float y,
                              float minGapX, float minGapY) const {
    const Fish& f = fishPool_[fishIndex];
    float centerX = x + f.visualWidth * 0.5f;
    float centerY = y + kFishCenterYOffset;
    int fishCount = activeFishLimit();
    for (int i = 0; i < fishCount; ++i) {
        if (i == fishIndex || !fishPool_[i].active) continue;
        const Fish& other = fishPool_[i];
        float otherCenterX = other.x + other.visualWidth * 0.5f;
        float otherCenterY = other.y + kFishCenterYOffset;
        if (std::fabs(otherCenterX - centerX) < minGapX &&
            std::fabs(otherCenterY - centerY) < minGapY)
            return false;
    }
    return true;
}

void Aquarium::spreadInitialFishLayout() {
    int fishCount = activeFishLimit();
    float minGapX = kFishAvoidRadiusX * 0.92f;
    float minGapY = kFishAvoidRadiusY * 1.05f;
    for (int i = 0; i < fishCount; ++i) {
        Fish& f = fishPool_[i];
        if (!f.active) continue;

        float bestX = f.x;
        float bestY = f.y;
        bool placed = false;
        for (int attempt = 0; attempt < 80; ++attempt) {
            float candidateX = rng_.frand(10.0f, kScreenW - f.visualWidth - 10.0f);
            float candidateY = rng_.frand(18.0f, kSeaLevelY - 18.0f);
            bestX = candidateX;
            bestY = candidateY;
            if (fishSpawnClear(i, candidateX, candidateY, minGapX, minGapY)) {
                placed = true;
                break;
            }
        }

        f.x = bestX;
        f.y = bestY;
        if (!placed) {
            f.y = clampVal(f.y + rng_.frand(-8.0f, 8.0f), 18.0f,
                           static_cast<float>(kSeaLevelY) - 18.0f);
        }
        f.vx = (rng_.random(100) < 50) ? -1.0f : 1.0f;
        f.vy = rng_.frand(-0.22f, 0.22f);
    }
}

void Aquarium::respawnFishPopulation() {
    fishTargetCount_ = clampVal(fishTargetCount_, kMinFish, kMaxFish);
    int fishCount = activeFishLimit();
    for (int i = 0; i < kMaxFishPool; ++i) {
        activateFish(fishPool_[i], i < fishCount);
    }
    spreadInitialFishLayout();
}

void Aquarium::resetBubble(Bubble& b, bool spreadOut) {
    b.active = true;
    b.baseX = rng_.frand(8.0f, kScreenW - 8.0f);
    b.x = b.baseX;
    b.y = spreadOut ? rng_.frand(4.0f, kScreenH + 48.0f)
                    : rng_.frand(kScreenH - 4.0f, kScreenH + 48.0f);
    b.vy = rng_.frand(12.0f, 28.0f);
    b.phase = rng_.frand(0.0f, kTwoPi);
    b.swayAmp = rng_.frand(2.0f, 7.0f);
    b.color = randomBubbleColor(rng_);
}

void Aquarium::applyBubblePopulation(bool spreadNew) {
    bubbleTargetCount_ = clampVal(bubbleTargetCount_, kMinBubbles, kMaxBubbles);
    for (int i = 0; i < kMaxBubbles; ++i) {
        bool shouldBeActive = (i < bubbleTargetCount_);
        if (shouldBeActive && !bubbles_[i].active) resetBubble(bubbles_[i], spreadNew);
        if (!shouldBeActive && bubbles_[i].active) bubbles_[i].active = false;
    }
}

void Aquarium::spawnFlake(float x, float y) {
    for (int i = 0; i < kMaxFlakes; ++i) {
        if (!flakes_[i].active) {
            flakes_[i].active = true;
            flakes_[i].x = x;
            flakes_[i].y = y;
            flakes_[i].vy = rng_.frand(22.0f, 48.0f);
            flakes_[i].color = randomFoodColor(rng_);
            return;
        }
    }
}

void Aquarium::setFishTarget(int n) {
    fishTargetCount_ = clampVal(n, kMinFish, kMaxFish);
    applyFishPopulation();
}

void Aquarium::setBubbleTarget(int n) {
    bubbleTargetCount_ = clampVal(n, kMinBubbles, kMaxBubbles);
    applyBubblePopulation(false);
}

void Aquarium::nudgeFish(int delta) {
    fishTargetCount_ = clampVal(fishTargetCount_ + delta, kMinFish, kMaxFish);
    applyFishPopulation();
}

void Aquarium::nudgeBubbles(int delta) {
    bubbleTargetCount_ = clampVal(bubbleTargetCount_ + delta, kMinBubbles, kMaxBubbles);
    applyBubblePopulation(false);
}

void Aquarium::cycleOctopusFrequency(int delta) {
    int current = normalizeFrequency(octopusFrequency_, kOctopusFreqOptions, 6);
    int index = 0;
    for (int i = 0; i < 6; ++i) {
        if (kOctopusFreqOptions[i] == current) { index = i; break; }
    }
    index += delta;
    if (index < 0) index = 5;
    if (index >= 6) index = 0;
    octopusFrequency_ = kOctopusFreqOptions[index];
    if (!octopus_.active) octopus_.nextSpawnMs = 0;
}

void Aquarium::cycleSeahorseFrequency(int delta) {
    int current = normalizeFrequency(seahorseFrequency_, kSeahorseFreqOptions, 6);
    int index = 0;
    for (int i = 0; i < 6; ++i) {
        if (kSeahorseFreqOptions[i] == current) { index = i; break; }
    }
    index += delta;
    if (index < 0) index = 5;
    if (index >= 6) index = 0;
    seahorseFrequency_ = kSeahorseFreqOptions[index];
    if (!seahorse_.active) seahorse_.nextSpawnMs = 0;
}

void Aquarium::nudgeSeaweedSway(float delta) {
    seaweedSwaySpeed_ = clampVal(seaweedSwaySpeed_ + delta, kMinSway, kMaxSway);
}

void Aquarium::nudgeSeaweedLength(float delta) {
    seaweedLength_ = clampVal(seaweedLength_ + delta, kMinSeaweedLength, kMaxSeaweedLength);
}

void Aquarium::nudgeSeaweedRandomness(float delta) {
    seaweedLengthRandomness_ =
        clampVal(seaweedLengthRandomness_ + delta, kMinSeaweedRandomness, kMaxSeaweedRandomness);
}

// ---------------------------- init -----------------------------------------

void Aquarium::init() {
    initFishMirrors();
    initFishGlyphMetrics();
    applyFishPopulation();
    spreadInitialFishLayout();
    applyBubblePopulation(true);

    for (int i = 0; i < kSeaweedRoots; ++i) {
        seaweedBaseX_[i] = 10 + i * (kScreenW - 20.0f) / (kSeaweedRoots - 1);
        seaweedAmp_[i] = 5.0f + (i % 4) * 2.0f;
        seaweedHeightNoise_[i] = std::sin(i * 2.173f + 0.61f);
    }
}

// ---------------------------- per-frame updates ----------------------------

void Aquarium::updateFlakes(float dt) {
    float t = tSec_;
    for (int i = 0; i < kMaxFlakes; ++i) {
        if (!flakes_[i].active) continue;
        flakes_[i].y += flakes_[i].vy * dt;
        flakes_[i].x += std::sin(t * 1.2f + i) * 8.0f * dt;
        if (flakes_[i].y > kSeaLevelY) flakes_[i].active = false;
    }
}

void Aquarium::updateBubbles(float dt) {
    float t = tSec_;
    int bubbleCount = activeBubbleLimit();
    for (int i = 0; i < bubbleCount; ++i) {
        if (!bubbles_[i].active) continue;
        bubbles_[i].y -= bubbles_[i].vy * dt;
        bubbles_[i].x = bubbles_[i].baseX + std::sin(t * 1.8f + bubbles_[i].phase) * bubbles_[i].swayAmp;
        if (bubbles_[i].y < -10.0f) resetBubble(bubbles_[i], false);
    }
}

int Aquarium::closestFlakeForFish(const Fish& f, float maxDist) const {
    int best = -1;
    float bestD2 = maxDist * maxDist;
    for (int i = 0; i < kMaxFlakes; ++i) {
        if (!flakes_[i].active) continue;
        float dx = flakes_[i].x - f.x;
        float dy = flakes_[i].y - f.y;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestD2) {
            bestD2 = d2;
            best = i;
        }
    }
    return best;
}

void Aquarium::steerFishAwayFromOctopus(Fish& f, float fishCenterX, float fishCenterY,
                                        float dt) const {
    if (!octopus_.active) return;
    float dx = fishCenterX - octopus_.x;
    float dy = fishCenterY - (octopus_.y + kOctopusCenterYOffset);
    float sx = dx / kOctopusFishAvoidRadiusX;
    float sy = dy / kOctopusFishAvoidRadiusY;
    float scaledD2 = sx * sx + sy * sy;
    if (scaledD2 <= 0.0001f || scaledD2 >= 1.0f) return;
    float dist = std::sqrt(dx * dx + dy * dy) + 0.0001f;
    float push = 1.0f - scaledD2;
    push *= push;
    f.vx += (dx / dist) * push * kOctopusFishAvoidStrength * dt;
    f.vy += (dy / dist) * push * kOctopusFishAvoidStrength * dt;
}

void Aquarium::steerFishAwayFromSeahorse(Fish& f, float fishCenterX, float fishCenterY,
                                         float dt) const {
    if (!seahorse_.active) return;
    float horseCenterX = seahorse_.x + kSeahorseCenterXOffset;
    float horseCenterY = seahorse_.y + kSeahorseCenterYOffset;
    float dx = fishCenterX - horseCenterX;
    float dy = fishCenterY - horseCenterY;
    float sx = dx / kSeahorseFishAvoidRadiusX;
    float sy = dy / kSeahorseFishAvoidRadiusY;
    float scaledD2 = sx * sx + sy * sy;
    if (scaledD2 <= 0.0001f || scaledD2 >= 1.0f) return;
    float dist = std::sqrt(dx * dx + dy * dy) + 0.0001f;
    float push = 1.0f - scaledD2;
    push *= push;
    f.vx += (dx / dist) * push * kSeahorseFishAvoidStrength * dt;
    f.vy += (dy / dist) * push * kSeahorseFishAvoidStrength * dt;
}

void Aquarium::keepFishOutsideOctopus(Fish& f) const {
    if (!octopus_.active) return;
    float fishCenterX = f.x + f.visualWidth * 0.5f;
    float fishCenterY = f.y + kFishCenterYOffset;
    float octoCenterY = octopus_.y + kOctopusCenterYOffset;
    float dx = fishCenterX - octopus_.x;
    float dy = fishCenterY - octoCenterY;
    float sx = dx / kOctopusFishClearRadiusX;
    float sy = dy / kOctopusFishClearRadiusY;
    float scaledD2 = sx * sx + sy * sy;
    if (scaledD2 >= 1.0f) return;
    if (scaledD2 <= 0.0001f) {
        dx = (f.vx >= 0.0f) ? 1.0f : -1.0f;
        dy = (f.vy >= 0.0f) ? 0.35f : -0.35f;
        scaledD2 = (dx / kOctopusFishClearRadiusX) * (dx / kOctopusFishClearRadiusX) +
                   (dy / kOctopusFishClearRadiusY) * (dy / kOctopusFishClearRadiusY);
    }
    float scale = 1.0f / std::sqrt(scaledD2);
    float targetCenterX = octopus_.x + dx * scale;
    float targetCenterY = octoCenterY + dy * scale;
    f.x += (targetCenterX - fishCenterX) * 0.55f;
    f.y += (targetCenterY - fishCenterY) * 0.55f;
}

void Aquarium::keepFishOutsideSeahorse(Fish& f) const {
    if (!seahorse_.active) return;
    float fishCenterX = f.x + f.visualWidth * 0.5f;
    float fishCenterY = f.y + kFishCenterYOffset;
    float horseCenterX = seahorse_.x + kSeahorseCenterXOffset;
    float horseCenterY = seahorse_.y + kSeahorseCenterYOffset;
    float dx = fishCenterX - horseCenterX;
    float dy = fishCenterY - horseCenterY;
    float sx = dx / kSeahorseFishClearRadiusX;
    float sy = dy / kSeahorseFishClearRadiusY;
    float scaledD2 = sx * sx + sy * sy;
    if (scaledD2 >= 1.0f) return;
    if (scaledD2 <= 0.0001f) {
        dx = (f.vx >= 0.0f) ? 1.0f : -1.0f;
        dy = (f.vy >= 0.0f) ? 0.35f : -0.35f;
        scaledD2 = (dx / kSeahorseFishClearRadiusX) * (dx / kSeahorseFishClearRadiusX) +
                   (dy / kSeahorseFishClearRadiusY) * (dy / kSeahorseFishClearRadiusY);
    }
    float scale = 1.0f / std::sqrt(scaledD2);
    float targetCenterX = horseCenterX + dx * scale;
    float targetCenterY = horseCenterY + dy * scale;
    f.x += (targetCenterX - fishCenterX) * 0.45f;
    f.y += (targetCenterY - fishCenterY) * 0.45f;
}

void Aquarium::updateFish(float dt) {
    const float t = tSec_;
    int fishCount = activeFishLimit();
    float centerX[kMaxFishPool];
    float centerY[kMaxFishPool];
    for (int i = 0; i < fishCount; ++i) {
        Fish& f = fishPool_[i];
        if (!f.active) continue;
        centerX[i] = f.x + f.visualWidth * 0.5f;
        centerY[i] = f.y + kFishCenterYOffset;
    }

    for (int i = 0; i < fishCount; ++i) {
        Fish& f = fishPool_[i];
        if (!f.active) continue;

        // Wander behavior.
        float wanderX = std::cos(f.phase + t * 0.9f) * 0.45f * f.wanderBias;
        float wanderY = std::sin(f.phase * 1.7f + t * 0.7f) * 0.22f;
        f.vx += wanderX * dt;
        f.vy += wanderY * dt;

        // Schooling with same type (alignment + cohesion) + separation.
        float avgVX = 0, avgVY = 0, cx = 0, cy = 0;
        int nearCount = 0;
        float repelX = 0.0f, repelY = 0.0f;
        int repelCount = 0;
        float fCenterX = centerX[i];
        float fCenterY = centerY[i];
        for (int j = 0; j < fishCount; ++j) {
            if (i == j || !fishPool_[j].active) continue;
            Fish& n = fishPool_[j];

            float dx = n.x - f.x;
            float dy = n.y - f.y;
            if (n.type == f.type) {
                float d2 = dx * dx + dy * dy;
                if (d2 < 3600.0f) {  // within ~60px
                    avgVX += n.vx;
                    avgVY += n.vy;
                    cx += n.x;
                    cy += n.y;
                    nearCount++;
                }
            }

            float sdx = centerX[j] - fCenterX;
            float sdy = centerY[j] - fCenterY;
            if (sdx > kScreenW * 0.5f) sdx -= kScreenW;
            if (sdx < -kScreenW * 0.5f) sdx += kScreenW;

            float sx = sdx / kFishAvoidRadiusX;
            float sy = sdy / kFishAvoidRadiusY;
            float scaledD2 = sx * sx + sy * sy;
            if (scaledD2 > 0.0001f && scaledD2 < 1.0f) {
                float dist = std::sqrt(sdx * sdx + sdy * sdy) + 0.0001f;
                float push = 1.0f - scaledD2;
                push *= push;
                repelX -= (sdx / dist) * push;
                repelY -= (sdy / dist) * push;
                repelCount++;
            }
        }
        if (nearCount > 0) {
            avgVX /= nearCount;
            avgVY /= nearCount;
            cx /= nearCount;
            cy /= nearCount;
            f.vx += (avgVX - f.vx) * 0.45f * dt;
            f.vy += (avgVY - f.vy) * 0.25f * dt;
            f.vx += (cx - f.x) * 0.0018f;
            f.vy += (cy - f.y) * 0.0012f;
        }

        // Feed-seeking behavior.
        int fi = closestFlakeForFish(f, 140.0f);
        if (fi >= 0) {
            float dx = flakes_[fi].x - f.x;
            float dy = flakes_[fi].y - f.y;
            float d = std::sqrt(dx * dx + dy * dy) + 0.0001f;
            f.vx += (dx / d) * 0.95f * dt;
            f.vy += (dy / d) * 0.95f * dt;
            if (d < 8.0f) flakes_[fi].active = false;  // "eat"
        }

        steerFishAwayFromOctopus(f, fCenterX, fCenterY, dt);
        steerFishAwayFromSeahorse(f, fCenterX, fCenterY, dt);

        // Gentle steering separation, not hard collision.
        if (repelCount > 0) {
            f.vx += (repelX / repelCount) * kFishAvoidStrength * dt;
            f.vy += (repelY / repelCount) * kFishAvoidStrength * dt;
        }

        // Vertical edge avoidance only (horizontal uses wraparound).
        if (f.y < 18) f.vy += 0.8f * dt;
        if (f.y > kSeaLevelY - 8) f.vy -= 0.8f * dt;

        // Normalize velocity and apply speed.
        float mag = std::sqrt(f.vx * f.vx + f.vy * f.vy);
        if (mag < 0.0001f) {
            f.vx = 1.0f;
            f.vy = 0.0f;
            mag = 1.0f;
        }
        f.vx /= mag;
        f.vy /= mag;

        float fishSpeed = f.speed + std::sin(t * 3.2f + f.phase) * 4.0f;
        f.x += f.vx * fishSpeed * dt;
        f.y += f.vy * fishSpeed * dt;
        keepFishOutsideOctopus(f);
        keepFishOutsideSeahorse(f);

        // Horizontal wrap keeps fish flowing off-screen and re-entering.
        int w = fishVisualWidth(f);
        float wrapPad = static_cast<float>(w) + 10.0f;
        if (f.x > kScreenW + wrapPad) {
            f.x = -wrapPad;
            refreshFishDepth(f);
        }
        if (f.x < -wrapPad) {
            f.x = kScreenW + wrapPad;
            refreshFishDepth(f);
        }
        f.y = clampVal(f.y, 14.0f, static_cast<float>(kSeaLevelY) - 6.0f);
    }
}

// ---------------------------- visitors -------------------------------------

namespace {
bool timeReached(unsigned long now, unsigned long target) {
    return static_cast<long>(now - target) >= 0;
}
}  // namespace

unsigned long Aquarium::octopusSpawnIntervalMs() const {
    int freq = normalizeFrequency(octopusFrequency_, kOctopusFreqOptions, 6);
    return 3600000UL / static_cast<unsigned long>(freq);
}

unsigned long Aquarium::seahorseSpawnIntervalMs() const {
    int freq = normalizeFrequency(seahorseFrequency_, kSeahorseFreqOptions, 6);
    return 3600000UL / static_cast<unsigned long>(freq);
}

void Aquarium::scheduleOctopusSpawn(unsigned long now) {
    octopus_.nextSpawnMs = now + octopusSpawnIntervalMs();
}

void Aquarium::spawnOctopus(unsigned long now) {
    bool fromLeft = (rng_.random(100) < 50);
    octopus_.active = true;
    octopus_.vx = fromLeft ? rng_.frand(4.5f, 8.0f) : -rng_.frand(4.5f, 8.0f);
    octopus_.x = fromLeft ? -kOctopusExitPad : (kScreenW + kOctopusExitPad);
    octopus_.baseY = rng_.frand(36.0f, static_cast<float>(kSeaLevelY) - 48.0f);
    octopus_.y = octopus_.baseY;
    octopus_.phase = rng_.frand(0.0f, kTwoPi);
    octopus_.colorPhase = rng_.frand(0.0f, kTwoPi);
    scheduleOctopusSpawn(now);
}

void Aquarium::spawnOctopusAtCenter() {
    octopus_.active = true;
    octopus_.x = kScreenW * 0.5f;
    octopus_.baseY = kSeaLevelY * 0.55f;
    octopus_.y = octopus_.baseY;
    octopus_.vx = (rng_.random(100) < 50) ? -rng_.frand(3.8f, 6.5f) : rng_.frand(3.8f, 6.5f);
    octopus_.phase = rng_.frand(0.0f, kTwoPi);
    octopus_.colorPhase = rng_.frand(0.0f, kTwoPi);
    scheduleOctopusSpawn(nowMs_);
}

void Aquarium::updateOctopus(unsigned long now, float dt) {
    if (!octopus_.active) {
        if (octopus_.nextSpawnMs == 0) {
            scheduleOctopusSpawn(now);
        } else if (timeReached(now, octopus_.nextSpawnMs)) {
            spawnOctopus(now);
        }
        return;
    }
    float t = now * 0.001f;
    octopus_.x += octopus_.vx * dt;
    octopus_.y = octopus_.baseY + std::sin(t * 0.45f + octopus_.phase) * 6.0f;
    if ((octopus_.vx > 0.0f && octopus_.x > kScreenW + kOctopusExitPad) ||
        (octopus_.vx < 0.0f && octopus_.x < -kOctopusExitPad)) {
        octopus_.active = false;
    }
}

void Aquarium::scheduleSeahorseSpawn(unsigned long now) {
    seahorse_.nextSpawnMs = now + seahorseSpawnIntervalMs();
}

void Aquarium::spawnSeahorse(unsigned long now) {
    bool fromLeft = (rng_.random(100) < 50);
    seahorse_.active = true;
    seahorse_.facingRight = fromLeft;
    seahorse_.vx = fromLeft ? rng_.frand(1.6f, 2.9f) * kSeahorseSpeedBoost
                            : -rng_.frand(1.6f, 2.9f) * kSeahorseSpeedBoost;
    seahorse_.x = fromLeft ? -kSeahorseExitPad : (kScreenW + kSeahorseExitPad);
    seahorse_.baseY = rng_.frand(34.0f, static_cast<float>(kSeaLevelY) - 56.0f);
    seahorse_.y = seahorse_.baseY;
    seahorse_.phase = rng_.frand(0.0f, kTwoPi);
    seahorse_.finPhase = rng_.frand(0.0f, kTwoPi);
    scheduleSeahorseSpawn(now);
}

void Aquarium::spawnSeahorseAtCenter() {
    seahorse_.active = true;
    seahorse_.facingRight = (rng_.random(100) < 50);
    seahorse_.x = kScreenW * 0.5f - 16.0f;
    seahorse_.baseY = kSeaLevelY * 0.46f;
    seahorse_.y = seahorse_.baseY;
    seahorse_.vx = seahorse_.facingRight ? rng_.frand(1.4f, 2.4f) * kSeahorseSpeedBoost
                                         : -rng_.frand(1.4f, 2.4f) * kSeahorseSpeedBoost;
    seahorse_.phase = rng_.frand(0.0f, kTwoPi);
    seahorse_.finPhase = rng_.frand(0.0f, kTwoPi);
    scheduleSeahorseSpawn(nowMs_);
}

void Aquarium::updateSeahorse(unsigned long now, float dt) {
    if (!seahorse_.active) {
        if (seahorse_.nextSpawnMs == 0) {
            scheduleSeahorseSpawn(now);
        } else if (timeReached(now, seahorse_.nextSpawnMs)) {
            spawnSeahorse(now);
        }
        return;
    }
    float t = now * 0.001f;
    float pulse = 1.0f + std::sin(t * 0.55f + seahorse_.phase) * 0.18f;
    seahorse_.x += seahorse_.vx * pulse * dt;
    seahorse_.y = seahorse_.baseY + std::sin(t * 0.82f + seahorse_.phase) * 4.5f +
                  std::sin(t * 2.15f + seahorse_.phase * 1.7f) * 0.9f;
    if ((seahorse_.vx > 0.0f && seahorse_.x > kScreenW + kSeahorseExitPad) ||
        (seahorse_.vx < 0.0f && seahorse_.x < -kSeahorseExitPad)) {
        seahorse_.active = false;
    }
}

void Aquarium::keepVisitorsSeparated() {
    if (!octopus_.active || !seahorse_.active) return;
    float octoCenterX = octopus_.x;
    float octoCenterY = octopus_.y + kOctopusCenterYOffset;
    float horseCenterX = seahorse_.x + kSeahorseCenterXOffset;
    float horseCenterY = seahorse_.y + kSeahorseCenterYOffset;
    float dx = horseCenterX - octoCenterX;
    float dy = horseCenterY - octoCenterY;
    float sx = dx / kVisitorClearRadiusX;
    float sy = dy / kVisitorClearRadiusY;
    float scaledD2 = sx * sx + sy * sy;
    if (scaledD2 >= 1.0f) return;
    if (scaledD2 <= 0.0001f) {
        dx = (seahorse_.vx >= octopus_.vx) ? 1.0f : -1.0f;
        dy = 0.35f;
        scaledD2 = (dx / kVisitorClearRadiusX) * (dx / kVisitorClearRadiusX) +
                   (dy / kVisitorClearRadiusY) * (dy / kVisitorClearRadiusY);
    }
    float scale = 1.0f / std::sqrt(scaledD2);
    float targetHorseCenterX = octoCenterX + dx * scale;
    float targetHorseCenterY = octoCenterY + dy * scale;
    float pushX = (targetHorseCenterX - horseCenterX) * 0.18f;
    float pushY = (targetHorseCenterY - horseCenterY) * 0.22f;
    seahorse_.x += pushX;
    seahorse_.baseY = clampVal(seahorse_.baseY + pushY, 24.0f,
                               static_cast<float>(kSeaLevelY) - 54.0f);
    seahorse_.y += pushY;
    octopus_.x -= pushX * 0.55f;
    octopus_.baseY = clampVal(octopus_.baseY - pushY * 0.55f, 28.0f,
                              static_cast<float>(kSeaLevelY) - 44.0f);
    octopus_.y -= pushY * 0.55f;
}

void Aquarium::update(float dt, unsigned long nowMs) {
    nowMs_ = nowMs;
    tSec_ = nowMs * 0.001f;
    updateFlakes(dt);
    updateBubbles(dt);
    updateFish(dt);
    updateOctopus(nowMs, dt);
    updateSeahorse(nowMs, dt);
    keepVisitorsSeparated();
}

// ---------------------------- drawing --------------------------------------

void Aquarium::drawSeaweedBranches(Framebuffer& fb, int bladeIndex, float bladeHeight,
                                   float sway, float bx, int y0) const {
    int branchCount = clampVal(static_cast<int>(bladeHeight / 14.0f), 2, 5);
    for (int b = 0; b < branchCount; ++b) {
        float u = 0.30f + b * 0.14f + ((bladeIndex + b) % 3) * 0.018f;
        if (u > 0.88f) u = 0.88f;

        float px, py;
        seaweedPointAt(u, bx, y0, bladeHeight, sway, tSec_, bladeIndex,
                       seaweedSwaySpeed_, px, py);

        float side = ((bladeIndex + b) & 1) ? 1.0f : -1.0f;
        float branchLen = 5.5f + ((bladeIndex * 3 + b * 5) % 5);
        float branchWiggle = std::sin(tSec_ * (1.1f + bladeIndex * 0.03f) * seaweedSwaySpeed_ +
                                      bladeIndex + b * 1.7f) * 1.2f;
        int ex = static_cast<int>(px + side * (branchLen * 0.58f + std::fabs(sway) * 0.05f) + branchWiggle);
        int ey = static_cast<int>(py - branchLen * 0.78f);
        std::uint16_t color = (b & 1) ? TFT_DARKGREEN : TFT_GREEN;
        fb.drawLine(static_cast<int>(px), static_cast<int>(py), ex, ey, color);
    }
}

void Aquarium::drawSeaweed(Framebuffer& fb) const {
    for (int i = 0; i < kSeaweedRoots; ++i) {
        float bx = seaweedBaseX_[i];
        float sway = std::sin(tSec_ * (0.8f + 0.09f * i) * seaweedSwaySpeed_ + i * 0.7f) * seaweedAmp_[i];
        float heightVariation = 1.0f + seaweedLengthRandomness_ * seaweedHeightNoise_[i];
        float bladeHeight = clampVal(32.0f * seaweedLength_ * heightVariation, 18.0f, 72.0f);
        int y0 = kScreenH - 2;

        float prevX = bx, prevY = y0;
        const int segments = 7;
        for (int seg = 1; seg <= segments; ++seg) {
            float u = static_cast<float>(seg) / segments;
            float x, y;
            seaweedPointAt(u, bx, y0, bladeHeight, sway, tSec_, i, seaweedSwaySpeed_, x, y);
            std::uint16_t color = (u < 0.38f) ? TFT_DARKGREEN
                                  : ((u < 0.76f) ? TFT_GREEN : TFT_GREENYELLOW);
            fb.drawLine(static_cast<int>(prevX), static_cast<int>(prevY),
                        static_cast<int>(x), static_cast<int>(y), color);
            if (u < 0.78f) {
                fb.drawLine(static_cast<int>(prevX) + 1, static_cast<int>(prevY),
                            static_cast<int>(x) + 1, static_cast<int>(y), TFT_DARKGREEN);
            }
            prevX = x;
            prevY = y;
        }
        drawSeaweedBranches(fb, i, bladeHeight, sway, bx, y0);
    }
}

void Aquarium::drawFlakes(Framebuffer& fb) const {
    fb.setTextSize(1);
    fb.setTextDatum(MC_DATUM);
    for (int i = 0; i < kMaxFlakes; ++i) {
        if (!flakes_[i].active) continue;
        fb.setTextColor(flakes_[i].color);
        fb.drawString("*", static_cast<int>(flakes_[i].x), static_cast<int>(flakes_[i].y));
    }
}

void Aquarium::drawBubbles(Framebuffer& fb) const {
    fb.setTextSize(1);
    fb.setTextDatum(MC_DATUM);
    int bubbleCount = activeBubbleLimit();
    for (int i = 0; i < bubbleCount; ++i) {
        if (!bubbles_[i].active) continue;
        fb.setTextColor(bubbles_[i].color);
        fb.drawString("o", static_cast<int>(bubbles_[i].x), static_cast<int>(bubbles_[i].y));
    }
}

void Aquarium::drawFish(Framebuffer& fb) const {
    fb.setTextSize(1);
    fb.setTextDatum(TL_DATUM);
    const float t = tSec_;
    const float waveBase = t * kFishSwimWaveSpeed;
    const float waveStepSin = std::sin(kFishSwimWaveSpacing);
    const float waveStepCos = std::cos(kFishSwimWaveSpacing);
    int fishCount = activeFishLimit();
    for (int i = 0; i < fishCount; ++i) {
        const Fish& f = fishPool_[i];
        if (!f.active) continue;
        const char* txt = fishGlyphDrawing(f);
        const std::int16_t* glyphOffsets = fishGlyphOffsets(f);
        std::uint8_t len = fishGlyphLength(f);
        float waveAngle = waveBase + f.phase;
        float wave = std::sin(waveAngle);
        float waveCos = std::cos(waveAngle);
        fb.setTextColor(f.renderColor);
        for (std::uint8_t c = 0; c < len; ++c) {
            if (txt[c] != ' ') {
                float yOffset = wave * kFishSwimWaveAmplitude;
                int charX = static_cast<int>(f.x) + glyphOffsets[c];
                int charY = static_cast<int>(f.y) +
                            static_cast<int>(yOffset + ((yOffset >= 0.0f) ? 0.5f : -0.5f));
                fb.drawChar(static_cast<std::uint16_t>(txt[c]), charX, charY);
            }
            float nextWave = wave * waveStepCos + waveCos * waveStepSin;
            waveCos = waveCos * waveStepCos - wave * waveStepSin;
            wave = nextWave;
        }
    }
}

void Aquarium::drawOctopus(Framebuffer& fb) const {
    if (!octopus_.active) return;
    float t = tSec_;
    int cx = static_cast<int>(octopus_.x);
    int cy = static_cast<int>(octopus_.y);

    int r = 205 + static_cast<int>(42.0f * std::sin(t * 0.18f + octopus_.colorPhase));
    int g = 78 + static_cast<int>(38.0f * std::sin(t * 0.13f + octopus_.colorPhase + 2.1f));
    int b = 178 + static_cast<int>(58.0f * std::sin(t * 0.16f + octopus_.colorPhase + 4.2f));

    fb.setTextSize(1);
    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(rgb565From888(r, g, b));

    float topWave = std::sin(t * 1.25f + octopus_.phase) * 1.4f;
    fb.drawChar('(', cx - 13, cy + static_cast<int>(topWave));
    fb.drawChar('.', cx - 3, cy - 5);
    fb.drawChar('.', cx + 7, cy - 5);
    fb.drawChar(')', cx + 16, cy - static_cast<int>(topWave));

    static const char tentacleGlyphs[6] = {'(', '(', '(', ')', ')', ')'};
    static const int tentacleX[6] = {-24, -16, -8, 2, 10, 18};
    for (int i = 0; i < 6; ++i) {
        float wave = std::sin(t * 1.75f + octopus_.phase + i * 0.72f);
        int x = cx + tentacleX[i] + static_cast<int>(wave * 1.4f);
        int y = cy + 13 + static_cast<int>(wave * 2.2f);
        fb.drawChar(static_cast<std::uint16_t>(tentacleGlyphs[i]), x, y);
    }
}

void Aquarium::drawSeahorse(Framebuffer& fb) const {
    if (!seahorse_.active) return;
    static const char* seahorseLeftRows[] = {
        "  ^^  ",
        " / o) ",
        "[__-/ ",
        "  /|  ",
        " / |  ",
        " \\ |  ",
        "  ( ) ",
        "  \\_/ ",
    };
    const int kRows = static_cast<int>(sizeof(seahorseLeftRows) / sizeof(seahorseLeftRows[0]));
    const int kCols = 6;
    const int kCellW = 5;
    const int kRowH = 6;

    float t = tSec_;
    int x = static_cast<int>(seahorse_.x);
    int y = static_cast<int>(seahorse_.y);
    int sway = static_cast<int>(std::sin(t * 1.15f + seahorse_.phase) * 1.2f);
    int finFlutter = static_cast<int>(std::sin(t * 10.0f + seahorse_.finPhase) * 1.2f);
    const char* finGlyph = (std::sin(t * 12.0f + seahorse_.finPhase) > 0.0f) ? "~" : "-";

    int r = 238 + static_cast<int>(12.0f * std::sin(t * 0.11f + seahorse_.phase));
    int g = 142 + static_cast<int>(18.0f * std::sin(t * 0.16f + seahorse_.phase + 1.4f));
    int b = 48 + static_cast<int>(12.0f * std::sin(t * 0.13f + seahorse_.phase + 2.8f));

    fb.setTextSize(1);
    fb.setTextFont(1);
    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(rgb565From888(r, g, b));

    for (int row = 0; row < kRows; ++row) {
        const char* line = seahorseLeftRows[row];
        int len = static_cast<int>(std::strlen(line));
        int rowSway = (row >= 1 && row <= 3) ? sway : 0;
        for (int col = 0; col < kCols; ++col) {
            char glyph = (col < len) ? line[col] : ' ';
            if (glyph == ' ') continue;
            int drawCol = col;
            if (seahorse_.facingRight) {
                drawCol = kCols - 1 - col;
                glyph = mirrorSeahorseGlyph(glyph);
            }
            fb.drawChar(static_cast<std::uint16_t>(glyph),
                        x + drawCol * kCellW + rowSway, y + row * kRowH);
        }
    }

    fb.setTextColor(rgb565From888(255, 188, 82));
    int finX = seahorse_.facingRight ? x + 5 + finFlutter : x + 20 + finFlutter;
    fb.drawString(finGlyph, finX, y + 24);
    fb.setTextFont(2);
}

void Aquarium::draw(Framebuffer& fb) {
    // The background is painted by the caller (Background::draw) before this,
    // matching the sketch's drawBackground -> drawSeaweed -> ... order.
    fb.setTextFont(2);  // sketch's global default font for the scene

    drawSeaweed(fb);
    drawBubbles(fb);
    drawFlakes(fb);
    drawFish(fb);
    drawOctopus(fb);
    drawSeahorse(fb);
}

}  // namespace aq
