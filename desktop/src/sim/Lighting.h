// Scene lighting — a day/night ambient wash driven by the OS clock, plus
// animated caustic "god-rays" from the water surface.
//
// Net-new desktop polish (not in the upstream sketch). Both are post-processes
// over the rendered aquarium: caustics brighten the water, then the ambient
// tint dims/colors the whole scene by time of day. The HUD/clock are drawn
// afterward so they stay readable.

#pragma once

#include <cstdint>

namespace aq {

class Framebuffer;

class Lighting {
public:
    // Advance animation by `dt` seconds and recompute the ambient tint for the
    // given local time of day (fractional hour, [0, 24)).
    void update(float dt, float hourOfDay);

    // Brighten the water with drifting diagonal light shafts. Call after the
    // scene entities, before applyAmbient().
    void drawCaustics(Framebuffer& fb) const;

    // Tint/dim the whole framebuffer for the current time of day. Call last,
    // before the HUD/clock overlay.
    void applyAmbient(Framebuffer& fb) const;

    void setDayNight(bool on) { dayNight_ = on; }
    bool dayNight() const { return dayNight_; }
    void setCaustics(bool on) { caustics_ = on; }
    bool caustics() const { return caustics_; }

private:
    bool  dayNight_ = true;
    bool  caustics_ = true;
    float tSec_ = 0.0f;
    // Ambient per-channel multipliers for the current time of day (1,1,1 = noon).
    float mr_ = 1.0f, mg_ = 1.0f, mb_ = 1.0f;
};

}  // namespace aq
