#include "sim/Lighting.h"

#include <cmath>

#include "renderer/Framebuffer.h"

namespace aq {

namespace {

constexpr int kScreenW = 320;
constexpr float kTwoPi = 6.28318f;

// Time-of-day -> per-channel light multipliers. Deep night is dim and blue,
// dawn/dusk warm, midday neutral. Linear interpolation between keyframes.
struct LightKey { float h, r, g, b; };
const LightKey kKeys[] = {
    {0.0f,  0.30f, 0.42f, 0.60f},   // deep night (blue)
    {5.0f,  0.30f, 0.42f, 0.60f},
    {6.5f,  0.55f, 0.46f, 0.44f},   // pre-dawn, warming
    {7.5f,  0.95f, 0.78f, 0.62f},   // sunrise glow
    {9.0f,  1.00f, 1.00f, 1.00f},   // full day
    {17.0f, 1.00f, 1.00f, 1.00f},
    {18.5f, 1.00f, 0.72f, 0.50f},   // sunset glow
    {20.0f, 0.55f, 0.46f, 0.46f},   // dusk
    {21.5f, 0.30f, 0.42f, 0.60f},   // night
    {24.0f, 0.30f, 0.42f, 0.60f},
};
constexpr int kKeyCount = static_cast<int>(sizeof(kKeys) / sizeof(kKeys[0]));

void lightAt(float h, float& r, float& g, float& b) {
    if (h < 0.0f) h = 0.0f;
    if (h > 24.0f) h = 24.0f;
    for (int i = 1; i < kKeyCount; ++i) {
        if (h <= kKeys[i].h) {
            const LightKey& a = kKeys[i - 1];
            const LightKey& c = kKeys[i];
            float span = c.h - a.h;
            float t = (span > 0.0f) ? (h - a.h) / span : 0.0f;
            r = a.r + (c.r - a.r) * t;
            g = a.g + (c.g - a.g) * t;
            b = a.b + (c.b - a.b) * t;
            return;
        }
    }
    r = kKeys[kKeyCount - 1].r;
    g = kKeys[kKeyCount - 1].g;
    b = kKeys[kKeyCount - 1].b;
}

// Add a small delta to a packed RGB565 pixel's channels (clamped).
inline std::uint16_t brightenPixel(std::uint16_t c, int dr, int dg, int db) {
    int r = ((c >> 11) & 0x1F) + dr;
    int g = ((c >> 5) & 0x3F) + dg;
    int b = (c & 0x1F) + db;
    if (r > 31) r = 31;
    if (g > 63) g = 63;
    if (b > 31) b = 31;
    return static_cast<std::uint16_t>((r << 11) | (g << 5) | b);
}

}  // namespace

void Lighting::update(float dt, float hourOfDay) {
    tSec_ += dt;
    if (dayNight_) {
        lightAt(hourOfDay, mr_, mg_, mb_);
    } else {
        mr_ = mg_ = mb_ = 1.0f;
    }
}

void Lighting::drawCaustics(Framebuffer& fb) const {
    if (!caustics_) return;
    constexpr int kRays = 6;
    constexpr int kDepth = 150;       // how far down the shafts reach
    constexpr float kSlope = 0.42f;   // diagonal lean
    for (int i = 0; i < kRays; ++i) {
        // Each shaft drifts horizontally at its own slow rate/phase.
        float phase = i * 1.7f;
        float drift = std::sin(tSec_ * 0.13f + phase) * 26.0f;
        float baseX = (i + 0.5f) * (kScreenW / static_cast<float>(kRays)) + drift;
        // Per-shaft brightness flicker.
        float flick = 0.7f + 0.3f * std::sin(tSec_ * 0.6f + phase * 2.0f);
        for (int d = 0; d < kDepth; ++d) {
            float fade = (1.0f - d / static_cast<float>(kDepth));
            float amt = fade * fade * flick;            // brightest near the surface
            int x = static_cast<int>(baseX + d * kSlope +
                                     std::sin(d * 0.05f + tSec_ * 0.4f + phase) * 1.5f);
            int dr = static_cast<int>(amt * 4.0f);
            int dg = static_cast<int>(amt * 5.0f);
            int db = static_cast<int>(amt * 7.0f);      // bluish-white light
            if (dr == 0 && dg == 0 && db == 0) continue;
            for (int w = -1; w <= 1; ++w) {
                int px = x + w;
                if (px < 0 || px >= kScreenW || d < 0 || d >= fb.height()) continue;
                int wr = (w == 0) ? dr : dr / 2;
                int wg = (w == 0) ? dg : dg / 2;
                int wb = (w == 0) ? db : db / 2;
                fb.drawPixel(px, d, brightenPixel(fb.readPixel(px, d), wr, wg, wb));
            }
        }
    }
}

void Lighting::applyAmbient(Framebuffer& fb) const {
    if (mr_ >= 0.999f && mg_ >= 0.999f && mb_ >= 0.999f) return;  // noon: no-op
    std::uint16_t* px = fb.data();
    const int n = fb.width() * fb.height();
    for (int i = 0; i < n; ++i) {
        const std::uint16_t c = px[i];
        int r = static_cast<int>(((c >> 11) & 0x1F) * mr_);
        int g = static_cast<int>(((c >> 5) & 0x3F) * mg_);
        int b = static_cast<int>((c & 0x1F) * mb_);
        px[i] = static_cast<std::uint16_t>((r << 11) | (g << 5) | b);
    }
}

}  // namespace aq
