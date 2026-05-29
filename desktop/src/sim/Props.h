// Static tank decorations — a sandcastle, a treasure chest that burbles
// bubbles, and a couple of coral/rock clusters resting on the sand. Drawn
// behind the live scene (between the background and the seaweed/fish).
//
// Net-new desktop polish. The art is fixed-width and rendered in Font 1 (the
// 6x8 GLCD font) so the ASCII lines stay aligned, the same trick the seahorse
// uses.

#pragma once

#include <cstdint>

namespace aq {

class Framebuffer;

class Props {
public:
    Props();

    void update(float dt);
    void draw(Framebuffer& fb) const;

    void setEnabled(bool on) { enabled_ = on; }
    bool enabled() const { return enabled_; }

private:
    static constexpr int kChestBubbles = 4;

    bool  enabled_ = true;
    float tSec_ = 0.0f;
    float bubbleY_[kChestBubbles];   // rising offset per chest bubble
};

}  // namespace aq
