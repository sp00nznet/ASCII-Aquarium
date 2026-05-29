// Rectangle hit-testing for the on-screen UI.
//
// The upstream sketch routes a tap through a chain of inside(x,y,rx,ry,rw,rh)
// checks against button/panel rectangles. This header provides the same test
// (plus a Rect convenience overload) for the desktop UI layer.

#pragma once

namespace aq {
namespace ui {

struct Rect {
    int x, y, w, h;
};

// True if (px, py) lies within the rectangle [rx, rx+rw) x [ry, ry+rh).
// Identical semantics to the sketch's inside().
inline bool inside(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px < rx + rw && py >= ry && py < ry + rh);
}

inline bool inside(int px, int py, const Rect& r) {
    return inside(px, py, r.x, r.y, r.w, r.h);
}

}  // namespace ui
}  // namespace aq
