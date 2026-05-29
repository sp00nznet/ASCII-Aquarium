// Aquarium core simulation — fish, bubbles, flakes, seaweed, and the two
// occasional visitors (octopus, seahorse).
//
// Ported from the upstream ASCII_Aquarium_CYD.ino (the behavioral spec). The
// update/draw math, magic constants, and entity AI are kept faithful to the
// original; what changes is shape: the sketch's globals + free functions become
// this object's members + methods, drawing targets an aq::Framebuffer instead
// of a TFT_eSprite, and randomness comes from an injected aq::Rng.

#pragma once

#include <cstdint>

#include "sim/Rng.h"

namespace aq {

class Framebuffer;

// Canvas geometry, matching the CYD panel the sketch was written for.
inline constexpr int kScreenW = 320;
inline constexpr int kScreenH = 240;
inline constexpr int kSeaLevelY = kScreenH - 36;

// Pool sizes (upstream MAX_* constants).
inline constexpr int kMaxFishPool = 48;
inline constexpr int kMaxBubbles = 50;
inline constexpr int kMaxFlakes = 16;
inline constexpr std::size_t kFishGlyphBuf = 28;
inline constexpr int kGlyphCount = 12;  // number of fish species

struct Flake {
    bool active = false;
    float x = 0, y = 0;
    float vy = 0;
    std::uint16_t color = 0;
};

struct Bubble {
    bool active = false;
    float x = 0, y = 0;
    float baseX = 0;
    float vy = 0;
    float phase = 0;
    float swayAmp = 0;
    std::uint16_t color = 0;
};

struct Fish {
    bool active = false;
    int type = 0;
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float speed = 0;
    float phase = 0;
    float wanderBias = 0;
    int visualWidth = 0;
    std::uint16_t displayColor = 0;
    std::uint16_t renderColor = 0;
    float depthBrightness = 1.0f;
    float fullness = 0.5f;   // 0 = starving (frantic), 1 = sated (calm)
};

struct Octopus {
    bool active = false;
    float x = 0, y = 0;
    float baseY = 0;
    float vx = 0;
    float phase = 0;
    float colorPhase = 0;
    unsigned long nextSpawnMs = 0;
};

struct Seahorse {
    bool active = false;
    bool facingRight = false;
    float x = 0, y = 0;
    float baseY = 0;
    float vx = 0;
    float phase = 0;
    float finPhase = 0;
    unsigned long nextSpawnMs = 0;
};

// A crab that scuttles sideways along the sand.
struct Crab {
    bool active = false;
    float x = 0, y = 0;
    float vx = 0;
    float phase = 0;
    unsigned long nextSpawnMs = 0;
};

// A jellyfish that drifts and bobs while its bell pulses.
struct Jellyfish {
    bool active = false;
    float x = 0, y = 0;
    float baseY = 0;
    float vx = 0;
    float phase = 0;
    unsigned long nextSpawnMs = 0;
};

// A rare large shark that crosses the tank; nearby fish flee.
struct Shark {
    bool active = false;
    bool facingRight = false;
    float x = 0, y = 0;
    float baseY = 0;
    float vx = 0;
    float phase = 0;
    unsigned long nextSpawnMs = 0;
};

class Aquarium {
public:
    explicit Aquarium(Rng& rng);

    // One-time setup: builds fish glyph mirrors + Font 2 metrics, populates the
    // fish school and bubble field, and caches seaweed roots. Call once after
    // the renderer/fonts are ready.
    void init();

    // Advance the simulation. `dt` is seconds since the last frame; `nowMs` is
    // the monotonic animation clock in milliseconds (drives visitor spawns).
    void update(float dt, unsigned long nowMs);

    // Render the full scene (background + seaweed + entities) into `fb`.
    void draw(Framebuffer& fb);

    // Drop a food flake at (x, y) — wired to taps/clicks for feeding.
    void spawnFlake(float x, float y);

    // ---- Settings hooks (used by the Settings panel) ----
    void setFishTarget(int n);
    void setBubbleTarget(int n);
    void setSeaweedSway(float s)        { seaweedSwaySpeed_ = s; }
    void setSeaweedLength(float l)      { seaweedLength_ = l; }
    void setSeaweedRandomness(float r)  { seaweedLengthRandomness_ = r; }

    // Current control values (for display in the Settings panel).
    int   fishTarget() const         { return fishTargetCount_; }
    int   bubbleTarget() const       { return bubbleTargetCount_; }
    int   octopusFrequency() const   { return octopusFrequency_; }
    int   seahorseFrequency() const  { return seahorseFrequency_; }
    float seaweedSway() const        { return seaweedSwaySpeed_; }
    float seaweedLength() const      { return seaweedLength_; }
    float seaweedRandomness() const  { return seaweedLengthRandomness_; }

    // Settings-panel mutators (each clamps to the upstream min/max).
    void nudgeFish(int delta);
    void nudgeBubbles(int delta);
    void cycleOctopusFrequency(int delta);
    void cycleSeahorseFrequency(int delta);
    void setOctopusFrequency(int freq);   // snaps to nearest valid option
    void setSeahorseFrequency(int freq);
    void nudgeSeaweedSway(float delta);
    void nudgeSeaweedLength(float delta);
    void nudgeSeaweedRandomness(float delta);

    // HUD action buttons: re-roll the school, or summon a visitor at center.
    void respawn() { respawnFishPopulation(); }
    void spawnOctopusAtCenter();
    void spawnSeahorseAtCenter();

    // Summon the new creatures now (debug keys / on-demand).
    void spawnCrabNow();
    void spawnJellyfishNow();
    void spawnSharkNow();

private:
    // --- population management ---
    void activateFish(Fish& f, bool activeNow);
    void applyFishPopulation();
    void respawnFishPopulation();
    void spreadInitialFishLayout();
    bool fishSpawnClear(int fishIndex, float x, float y, float minGapX, float minGapY) const;
    void resetBubble(Bubble& b, bool spreadOut);
    void applyBubblePopulation(bool spreadNew);
    float randomFishDepthBrightness();
    void refreshFishDepth(Fish& f);
    void refreshFishRenderColor(Fish& f);

    // --- per-frame updates ---
    void updateFlakes(float dt);
    void updateBubbles(float dt);
    void updateFish(float dt);
    void updateOctopus(unsigned long now, float dt);
    void updateSeahorse(unsigned long now, float dt);
    void updateCrab(unsigned long now, float dt);
    void updateJellyfish(unsigned long now, float dt);
    void updateShark(unsigned long now, float dt);
    void keepVisitorsSeparated();

    // --- fish steering helpers ---
    int closestFlakeForFish(const Fish& f, float maxDist) const;
    void steerFishAwayFromOctopus(Fish& f, float cx, float cy, float dt) const;
    void steerFishAwayFromSeahorse(Fish& f, float cx, float cy, float dt) const;
    void steerFishAwayFromShark(Fish& f, float cx, float cy, float dt) const;
    void keepFishOutsideOctopus(Fish& f) const;
    void keepFishOutsideSeahorse(Fish& f) const;

    // --- visitor spawning ---
    void scheduleOctopusSpawn(unsigned long now);
    void spawnOctopus(unsigned long now);
    void scheduleSeahorseSpawn(unsigned long now);
    void spawnSeahorse(unsigned long now);
    unsigned long octopusSpawnIntervalMs() const;
    unsigned long seahorseSpawnIntervalMs() const;

    // --- drawing ---
    void drawSeaweed(Framebuffer& fb) const;
    void drawSeaweedBranches(Framebuffer& fb, int bladeIndex, float bladeHeight,
                             float sway, float bx, int y0) const;
    void drawFlakes(Framebuffer& fb) const;
    void drawBubbles(Framebuffer& fb) const;
    void drawFish(Framebuffer& fb) const;
    void drawOctopus(Framebuffer& fb) const;
    void drawSeahorse(Framebuffer& fb) const;
    void drawCrab(Framebuffer& fb) const;
    void drawJellyfish(Framebuffer& fb) const;
    void drawShark(Framebuffer& fb) const;

    // --- glyph metrics (computed in init from Font 2) ---
    void initFishMirrors();
    void initFishGlyphMetrics();
    const char* fishGlyphDrawing(const Fish& f) const;
    const std::int16_t* fishGlyphOffsets(const Fish& f) const;
    std::uint8_t fishGlyphLength(const Fish& f) const;
    int fishVisualWidth(const Fish& f) const;

    int activeFishLimit() const;
    int activeBubbleLimit() const;

    Rng& rng_;

    // Mirrored glyphs + cached Font 2 metrics per species/direction.
    char         fishMirroredLeft_[kGlyphCount][kFishGlyphBuf];
    std::uint8_t fishGlyphLenRight_[kGlyphCount];
    std::uint8_t fishGlyphLenLeft_[kGlyphCount];
    std::int16_t fishGlyphWidthRight_[kGlyphCount];
    std::int16_t fishGlyphWidthLeft_[kGlyphCount];
    std::int16_t fishGlyphOffsetRight_[kGlyphCount][kFishGlyphBuf];
    std::int16_t fishGlyphOffsetLeft_[kGlyphCount][kFishGlyphBuf];

    Fish      fishPool_[kMaxFishPool];
    Flake     flakes_[kMaxFlakes];
    Bubble    bubbles_[kMaxBubbles];
    Octopus   octopus_;
    Seahorse  seahorse_;
    Crab      crab_;
    Jellyfish jellyfish_;
    Shark     shark_;

    int   fishTargetCount_;
    int   bubbleTargetCount_;
    int   octopusFrequency_;
    int   seahorseFrequency_;
    float seaweedSwaySpeed_;
    float seaweedLength_;
    float seaweedLengthRandomness_;

    // Seaweed roots, cached once (upstream's static locals in drawSeaweed).
    static constexpr int kSeaweedRoots = 12;
    float seaweedBaseX_[kSeaweedRoots];
    float seaweedAmp_[kSeaweedRoots];
    float seaweedHeightNoise_[kSeaweedRoots];

    // Animation clock, set each update() and read by draw().
    unsigned long nowMs_ = 0;
    float tSec_ = 0.0f;
};

}  // namespace aq
