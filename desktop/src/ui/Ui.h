// On-screen HUD + Settings UI, ported from the upstream sketch.
//
// Owns the panel state (HUD visibility, which settings tab, the clock
// style/color pop-overs) and both halves of the interaction: draw() renders
// the chrome over the finished scene, and handleTap() routes a tap through the
// same inside()-rect chain the sketch uses, mutating the Aquarium / Background /
// Clock through their public setters.
//
// Scope note: the WiFi and Capture HUD buttons + panels and the device's
// internet-time / manual-clock-set rows are intentionally omitted here; they
// belong to the deferred WiFi (task 10) and Capture (task 12) work.

#pragma once

#include "sim/Background.h"

namespace aq {

class Aquarium;
class Clock;
class Framebuffer;
class Lighting;
class Props;

namespace ui {

enum class SettingsTab : int { Tank = 0, Seaweed = 1, Clock = 2, Background = 3, Scene = 4 };

class Ui {
public:
    Ui(Aquarium& aquarium, Background& background, Clock& clock, BackgroundMode& bgMode,
       Lighting& lighting, Props& props, bool& burnin, bool& autocycle);

    // Render HUD + Settings panel + clock pop-overs over the finished scene.
    void draw(Framebuffer& fb, float fps);

    // Route a tap. Returns true if the UI consumed it (so the caller should not
    // treat the tap as a feed-the-fish action).
    bool handleTap(int x, int y);

    bool hudVisible() const { return hudVisible_; }

private:
    void drawHud(Framebuffer& fb, float fps);
    void drawSettingsPanel(Framebuffer& fb);
    void drawClockSettings(Framebuffer& fb);
    void drawSceneSettings(Framebuffer& fb);
    void drawClockStylePanel(Framebuffer& fb);
    void drawClockColorPanel(Framebuffer& fb);

    bool handleHudButtons(int x, int y);
    bool handleSettingsTap(int x, int y);
    bool handleClockStylePanelTap(int x, int y);
    bool handleClockColorPanelTap(int x, int y);

    Aquarium&       aquarium_;
    Background&     background_;
    Clock&          clock_;
    BackgroundMode& bgMode_;
    Lighting&       lighting_;
    Props&          props_;
    bool&           burnin_;
    bool&           autocycle_;

    bool hudVisible_ = false;
    bool settingsOpen_ = false;
    SettingsTab activeTab_ = SettingsTab::Tank;
    bool clockStylePanelOpen_ = false;
    bool clockColorPanelOpen_ = false;
};

}  // namespace ui
}  // namespace aq
