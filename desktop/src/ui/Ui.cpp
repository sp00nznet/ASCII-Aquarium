#include "ui/Ui.h"

#include <cstdio>

#include "renderer/Color.h"
#include "renderer/Font.h"
#include "renderer/Framebuffer.h"
#include "sim/Aquarium.h"
#include "sim/Clock.h"
#include "sim/Lighting.h"
#include "sim/Props.h"
#include "ui/Hit.h"

namespace aq {
namespace ui {

namespace {

constexpr int SCREEN_W = 320;

// ---- Settings panel layout (upstream constants) ----
constexpr int PANEL_X = 40, PANEL_Y = 24, PANEL_W = 240, PANEL_H = 208;
constexpr int CLOSE_X = PANEL_X + PANEL_W - 34, CLOSE_Y = PANEL_Y + 8, CLOSE_W = 24, CLOSE_H = 24;
constexpr int VALUE_RIGHT_X = PANEL_X + 156;
constexpr int MINUS_X = PANEL_X + 166, PLUS_X = PANEL_X + 202;
constexpr int BTN_W = 30, BTN_H = 24;
constexpr int ACTION_X = PANEL_X + 134, ACTION_W = 98;
constexpr int ROW_START_Y = PANEL_Y + 36, ROW_GAP = 34;
constexpr int TAB_Y = PANEL_Y + PANEL_H - 30, TAB_H = 22, TAB_GAP = 4;
// Five equal tabs across the panel; labels kept short to fit.
constexpr int TAB_W = 42;
constexpr int TANK_TAB_X = PANEL_X + 5;
constexpr int SEAWEED_TAB_X = TANK_TAB_X + TAB_W + TAB_GAP;
constexpr int CLOCK_TAB_X = SEAWEED_TAB_X + TAB_W + TAB_GAP;
constexpr int BG_TAB_X = CLOCK_TAB_X + TAB_W + TAB_GAP;
constexpr int SCENE_TAB_X = BG_TAB_X + TAB_W + TAB_GAP;

constexpr int CLOCK_ROW_1_Y = PANEL_Y + 36, CLOCK_ROW_2_Y = PANEL_Y + 60;
// Scene tab uses 5 rows with a tighter gap to fit above the tab bar.
constexpr int SCENE_ROW_Y = PANEL_Y + 34, SCENE_ROW_GAP = 30;
constexpr int CLOCK_STYLE_BUTTON_X = PANEL_X + 92, CLOCK_STYLE_BUTTON_W = 66;

// ---- Clock style pop-over ----
constexpr int CS_X = 46, CS_Y = 54, CS_W = 228, CS_H = 176;
constexpr int CS_CLOSE_X = CS_X + CS_W - 32, CS_CLOSE_Y = CS_Y + 8, CS_CLOSE_W = 24, CS_CLOSE_H = 22;
constexpr int CS_ROW_1_Y = CS_Y + 40, CS_ROW_2_Y = CS_Y + 74, CS_ROW_3_Y = CS_Y + 104, CS_ROW_4_Y = CS_Y + 134;
constexpr int CS_LABEL_X = CS_X + 14, CS_LEFT_X = CS_X + 104, CS_RIGHT_X = CS_X + 164;
constexpr int CS_CHOICE_W = 54;
constexpr int CS_SWATCH_X = CS_LEFT_X, CS_SWATCH_W = 28;
constexpr int CS_COLOR_BTN_X = CS_RIGHT_X, CS_COLOR_BTN_W = 54;

// ---- Clock color pop-over ----
constexpr int CC_X = 20, CC_Y = 38, CC_W = 280, CC_H = 164;
constexpr int CC_CLOSE_X = CC_X + CC_W - 32, CC_CLOSE_Y = CC_Y + 8, CC_CLOSE_W = 24, CC_CLOSE_H = 22;
constexpr int CC_GRID_X = CC_X + 14, CC_GRID_Y = CC_Y + 46;
constexpr int CC_SWATCH_W = 26, CC_SWATCH_H = 22, CC_GAP_X = 7, CC_GAP_Y = 10, CC_COLS = 8;

// ---- HUD corner buttons ----
constexpr int CORNER_W = 28, CORNER_H = 20, CORNER_Y = 3;
constexpr int DEBUG_X = 7;
constexpr int SETTINGS_CORNER_X = SCREEN_W - 35;
constexpr int RESPAWN_Y = CORNER_Y + CORNER_H + 4;
constexpr int SEAHORSE_Y = RESPAWN_Y + CORNER_H + 4;
constexpr int OCTOPUS_Y = SEAHORSE_Y + CORNER_H + 4;

const char* kVersionLabel = "v1.67-desktop";

void drawButton(Framebuffer& fb, int x, int y, int w, int h, const char* label,
                std::uint16_t fg, std::uint16_t bg) {
    fb.fillRoundRect(x, y, w, h, 4, bg);
    fb.drawRoundRect(x, y, w, h, 4, fg);
    fb.setTextColor(fg, bg);
    fb.setTextDatum(MC_DATUM);
    fb.drawString(label, x + w / 2, y + h / 2);
}

void drawSettingRow(Framebuffer& fb, int rowY, const char* label, const char* value) {
    const int centerY = rowY + BTN_H / 2;
    fb.setTextDatum(ML_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString(label, PANEL_X + 12, centerY);
    fb.setTextDatum(MR_DATUM);
    fb.setTextColor(TFT_GREENYELLOW, TFT_NAVY);
    fb.drawString(value, VALUE_RIGHT_X, centerY);
    drawButton(fb, MINUS_X, rowY, BTN_W, BTN_H, "-", TFT_WHITE, TFT_DARKGREEN);
    drawButton(fb, PLUS_X, rowY, BTN_W, BTN_H, "+", TFT_WHITE, TFT_DARKGREEN);
}

void drawToggleRow(Framebuffer& fb, int rowY, const char* label, const char* leftLabel,
                   const char* rightLabel, bool leftActive, bool rightActive) {
    const int centerY = rowY + BTN_H / 2;
    fb.setTextDatum(ML_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString(label, PANEL_X + 12, centerY);
    drawButton(fb, MINUS_X, rowY, BTN_W, BTN_H, leftLabel,
               leftActive ? TFT_NAVY : TFT_WHITE, leftActive ? TFT_CYAN : TFT_DARKGREEN);
    drawButton(fb, PLUS_X, rowY, BTN_W, BTN_H, rightLabel,
               rightActive ? TFT_NAVY : TFT_WHITE, rightActive ? TFT_CYAN : TFT_DARKGREEN);
}

void drawActionRow(Framebuffer& fb, int rowY, const char* label, const char* actionLabel) {
    const int centerY = rowY + BTN_H / 2;
    fb.setTextDatum(ML_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString(label, PANEL_X + 12, centerY);
    drawButton(fb, ACTION_X, rowY, ACTION_W, BTN_H, actionLabel, TFT_CYAN, TFT_DARKGREEN);
}

void drawChoiceRow(Framebuffer& fb, int rowY, const char* label, const char* leftLabel,
                   const char* rightLabel, bool leftActive, bool rightActive) {
    const int centerY = rowY + BTN_H / 2;
    fb.setTextDatum(ML_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString(label, CS_LABEL_X, centerY);
    drawButton(fb, CS_LEFT_X, rowY, CS_CHOICE_W, BTN_H, leftLabel,
               leftActive ? TFT_NAVY : TFT_WHITE, leftActive ? TFT_CYAN : TFT_DARKGREEN);
    drawButton(fb, CS_RIGHT_X, rowY, CS_CHOICE_W, BTN_H, rightLabel,
               rightActive ? TFT_NAVY : TFT_WHITE, rightActive ? TFT_CYAN : TFT_DARKGREEN);
}

const char* backgroundModeName(BackgroundMode mode) {
    switch (mode) {
        case BackgroundMode::Black:          return "Black";
        case BackgroundMode::BlueGradient:   return "Blue Fade";
        case BackgroundMode::PurpleGradient: return "Purple Fade";
        case BackgroundMode::Flowers:        return "Flowers";
        default:                             return "Background";
    }
}

}  // namespace

Ui::Ui(Aquarium& aquarium, Background& background, Clock& clock, BackgroundMode& bgMode,
       Lighting& lighting, Props& props, bool& burnin, bool& autocycle)
    : aquarium_(aquarium), background_(background), clock_(clock), bgMode_(bgMode),
      lighting_(lighting), props_(props), burnin_(burnin), autocycle_(autocycle) {}

// ------------------------------ drawing ------------------------------------

void Ui::drawHud(Framebuffer& fb, float fps) {
    if (!hudVisible_) return;
    constexpr int kTextYStart = 32, kTextLineDY = 16, kTextX = 8;

    // Top-left toggle (labeled "D" upstream) and the corner action buttons.
    drawButton(fb, DEBUG_X, CORNER_Y, CORNER_W, CORNER_H, "D", TFT_WHITE, TFT_DARKGREEN);
    drawButton(fb, SETTINGS_CORNER_X, CORNER_Y, CORNER_W, CORNER_H, "S", TFT_WHITE, TFT_DARKGREEN);
    drawButton(fb, SETTINGS_CORNER_X, RESPAWN_Y, CORNER_W, CORNER_H, "R", TFT_WHITE, TFT_DARKGREEN);
    drawButton(fb, SETTINGS_CORNER_X, SEAHORSE_Y, CORNER_W, CORNER_H, "H", TFT_WHITE, TFT_DARKGREEN);
    drawButton(fb, SETTINGS_CORNER_X, OCTOPUS_Y, CORNER_W, CORNER_H, "O", TFT_WHITE, TFT_DARKGREEN);

    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_BLACK);
    char title[48];
    std::snprintf(title, sizeof(title), "ASCII Aquarium %s", kVersionLabel);
    fb.drawString(title, kTextX, kTextYStart);
    char line[64];
    std::snprintf(line, sizeof(line), "Fish:%d  FPS:%2.1f", aquarium_.fishTarget(), fps);
    fb.drawString(line, kTextX, kTextYStart + kTextLineDY);
}

void Ui::drawClockSettings(Framebuffer& fb) {
    // Row 1: "Clock" + Style button + OFF/ON.
    const int centerY = CLOCK_ROW_1_Y + BTN_H / 2;
    fb.setTextDatum(ML_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString("Clock", PANEL_X + 12, centerY);
    drawButton(fb, CLOCK_STYLE_BUTTON_X, CLOCK_ROW_1_Y, CLOCK_STYLE_BUTTON_W, BTN_H, "Style",
               clockStylePanelOpen_ ? TFT_NAVY : TFT_CYAN, clockStylePanelOpen_ ? TFT_CYAN : TFT_DARKGREEN);
    const bool on = clock_.visible();
    drawButton(fb, MINUS_X, CLOCK_ROW_1_Y, BTN_W, BTN_H, "OFF",
               !on ? TFT_NAVY : TFT_WHITE, !on ? TFT_CYAN : TFT_DARKGREEN);
    drawButton(fb, PLUS_X, CLOCK_ROW_1_Y, BTN_W, BTN_H, "ON",
               on ? TFT_NAVY : TFT_WHITE, on ? TFT_CYAN : TFT_DARKGREEN);

    // Row 2: Format 12h / 24h.
    drawToggleRow(fb, CLOCK_ROW_2_Y, "Format", "12h", "24h",
                  !clock_.use24Hour(), clock_.use24Hour());
}

void Ui::drawSettingsPanel(Framebuffer& fb) {
    if (!settingsOpen_) return;
    fb.fillRoundRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, 8, TFT_NAVY);
    fb.drawRoundRect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, 8, TFT_CYAN);
    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString("Settings", PANEL_X + 10, PANEL_Y + 10);

    char buf[24];
    if (activeTab_ == SettingsTab::Tank) {
        std::snprintf(buf, sizeof(buf), "%d", aquarium_.fishTarget());
        drawSettingRow(fb, ROW_START_Y, "Fish Population", buf);
        std::snprintf(buf, sizeof(buf), "%d", aquarium_.bubbleTarget());
        drawSettingRow(fb, ROW_START_Y + ROW_GAP, "Bubble Amount", buf);
        std::snprintf(buf, sizeof(buf), "%d/hr", aquarium_.octopusFrequency());
        drawSettingRow(fb, ROW_START_Y + ROW_GAP * 2, "Octopus Freq", buf);
        std::snprintf(buf, sizeof(buf), "%d/hr", aquarium_.seahorseFrequency());
        drawSettingRow(fb, ROW_START_Y + ROW_GAP * 3, "Seahorse Freq", buf);
    } else if (activeTab_ == SettingsTab::Seaweed) {
        std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(aquarium_.seaweedSway()));
        drawSettingRow(fb, ROW_START_Y, "Sway", buf);
        std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(aquarium_.seaweedLength()));
        drawSettingRow(fb, ROW_START_Y + ROW_GAP, "Length", buf);
        std::snprintf(buf, sizeof(buf), "%.2f", static_cast<double>(aquarium_.seaweedRandomness()));
        drawSettingRow(fb, ROW_START_Y + ROW_GAP * 2, "Length Rand", buf);
    } else if (activeTab_ == SettingsTab::Clock) {
        drawClockSettings(fb);
    } else if (activeTab_ == SettingsTab::Background) {
        drawSettingRow(fb, ROW_START_Y, "Style", backgroundModeName(bgMode_));
        if (bgMode_ == BackgroundMode::Flowers) {
            drawActionRow(fb, ROW_START_Y + ROW_GAP, "Flowers", "Randomize");
        }
    } else {
        drawSceneSettings(fb);
    }

    drawButton(fb, CLOSE_X, CLOSE_Y, CLOSE_W, CLOSE_H, "X", TFT_WHITE, TFT_RED);

    // Tabs (five equal cells; short labels).
    struct TabDef { int x; const char* label; SettingsTab tab; };
    const TabDef tabs[] = {
        {TANK_TAB_X, "Tank", SettingsTab::Tank},
        {SEAWEED_TAB_X, "Weeds", SettingsTab::Seaweed},
        {CLOCK_TAB_X, "Clock", SettingsTab::Clock},
        {BG_TAB_X, "Back", SettingsTab::Background},
        {SCENE_TAB_X, "Scene", SettingsTab::Scene},
    };
    for (const auto& t : tabs) {
        const bool active = (activeTab_ == t.tab);
        drawButton(fb, t.x, TAB_Y, TAB_W, TAB_H, t.label,
                   active ? TFT_NAVY : TFT_CYAN, active ? TFT_CYAN : TFT_DARKGREEN);
    }
}

void Ui::drawSceneSettings(Framebuffer& fb) {
    drawToggleRow(fb, SCENE_ROW_Y, "Day/Night", "Off", "On",
                  !lighting_.dayNight(), lighting_.dayNight());
    drawToggleRow(fb, SCENE_ROW_Y + SCENE_ROW_GAP, "Light Rays", "Off", "On",
                  !lighting_.caustics(), lighting_.caustics());
    drawToggleRow(fb, SCENE_ROW_Y + SCENE_ROW_GAP * 2, "Props", "Off", "On",
                  !props_.enabled(), props_.enabled());
    drawToggleRow(fb, SCENE_ROW_Y + SCENE_ROW_GAP * 3, "Anti Burn-in", "Off", "On",
                  !burnin_, burnin_);
    drawToggleRow(fb, SCENE_ROW_Y + SCENE_ROW_GAP * 4, "Auto Cycle", "Off", "On",
                  !autocycle_, autocycle_);
}

void Ui::drawClockStylePanel(Framebuffer& fb) {
    if (!clockStylePanelOpen_) return;
    fb.fillRoundRect(CS_X, CS_Y, CS_W, CS_H, 8, TFT_NAVY);
    fb.drawRoundRect(CS_X, CS_Y, CS_W, CS_H, 8, TFT_CYAN);
    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString("Clock Style", CS_X + 12, CS_Y + 12);

    const bool small = clock_.style() == ClockStyle::SmallText;
    drawChoiceRow(fb, CS_ROW_1_Y, "Style", "Small", "ASCII", small, !small);

    if (small) {
        const bool top = clock_.smallPosition() == ClockSmallPosition::Top;
        drawChoiceRow(fb, CS_ROW_2_Y, "Position", "Top", "Bottom", top, !top);
    } else {
        const int centerY = CS_ROW_2_Y + BTN_H / 2;
        fb.setTextDatum(ML_DATUM);
        fb.setTextColor(TFT_WHITE, TFT_NAVY);
        fb.drawString("Font", CS_LABEL_X, centerY);
        fb.setTextDatum(MR_DATUM);
        fb.setTextColor(TFT_GREENYELLOW, TFT_NAVY);
        fb.drawString("Standard", CS_X + CS_W - 44, centerY);
    }

    const bool flip = clock_.flipHorizontal();
    drawChoiceRow(fb, CS_ROW_3_Y, "Flip Clock", "Off", "On", !flip, flip);

    // Colour row: swatch + Pick button.
    const int centerY = CS_ROW_4_Y + BTN_H / 2;
    fb.setTextDatum(ML_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString("Colour", CS_LABEL_X, centerY);
    const std::uint16_t color = clock_.activeTextColor();
    fb.fillRoundRect(CS_SWATCH_X, CS_ROW_4_Y + 3, CS_SWATCH_W, BTN_H - 6, 3, color);
    fb.drawRoundRect(CS_SWATCH_X, CS_ROW_4_Y + 3, CS_SWATCH_W, BTN_H - 6, 3,
                     color == TFT_WHITE ? TFT_DARKGREY : TFT_WHITE);
    drawButton(fb, CS_COLOR_BTN_X, CS_ROW_4_Y, CS_COLOR_BTN_W, BTN_H, "Pick",
               clockColorPanelOpen_ ? TFT_NAVY : TFT_CYAN, clockColorPanelOpen_ ? TFT_CYAN : TFT_DARKGREEN);

    drawButton(fb, CS_CLOSE_X, CS_CLOSE_Y, CS_CLOSE_W, CS_CLOSE_H, "X", TFT_WHITE, TFT_RED);
}

void Ui::drawClockColorPanel(Framebuffer& fb) {
    if (!clockColorPanelOpen_) return;
    fb.fillRoundRect(CC_X, CC_Y, CC_W, CC_H, 8, TFT_NAVY);
    fb.drawRoundRect(CC_X, CC_Y, CC_W, CC_H, 8, TFT_CYAN);
    fb.setTextDatum(TL_DATUM);
    fb.setTextColor(TFT_WHITE, TFT_NAVY);
    fb.drawString("Clock Colour", CC_X + 12, CC_Y + 12);

    const std::uint16_t selected = clock_.activeTextColor();
    for (int i = 0; i < Clock::paletteCount(); ++i) {
        const int col = i % CC_COLS, row = i / CC_COLS;
        const int x = CC_GRID_X + col * (CC_SWATCH_W + CC_GAP_X);
        const int y = CC_GRID_Y + row * (CC_SWATCH_H + CC_GAP_Y);
        const std::uint16_t c = Clock::paletteColor(i);
        const bool isSel = (c == selected);
        if (isSel) fb.drawRoundRect(x - 3, y - 3, CC_SWATCH_W + 6, CC_SWATCH_H + 6, 5, TFT_WHITE);
        fb.fillRoundRect(x, y, CC_SWATCH_W, CC_SWATCH_H, 4, c);
        fb.drawRoundRect(x, y, CC_SWATCH_W, CC_SWATCH_H, 4, isSel ? TFT_YELLOW : TFT_CYAN);
        if (c == TFT_WHITE) fb.drawRoundRect(x + 1, y + 1, CC_SWATCH_W - 2, CC_SWATCH_H - 2, 3, TFT_DARKGREY);
    }
    drawButton(fb, CC_CLOSE_X, CC_CLOSE_Y, CC_CLOSE_W, CC_CLOSE_H, "X", TFT_WHITE, TFT_RED);
}

void Ui::draw(Framebuffer& fb, float fps) {
    fb.setTextFont(2);
    fb.setTextSize(1);
    drawHud(fb, fps);
    drawSettingsPanel(fb);
    drawClockStylePanel(fb);
    drawClockColorPanel(fb);
}

// ------------------------------ tap routing --------------------------------

bool Ui::handleHudButtons(int x, int y) {
    // Top-right: settings toggle (42px hotspot, matching the sketch).
    if (inside(x, y, SCREEN_W - 42, 0, 42, 26)) {
        settingsOpen_ = !settingsOpen_;
        if (!settingsOpen_) {
            clockStylePanelOpen_ = false;
            clockColorPanelOpen_ = false;
        }
        return true;
    }
    if (inside(x, y, SETTINGS_CORNER_X, RESPAWN_Y, CORNER_W, CORNER_H)) {
        aquarium_.respawn();
        return true;
    }
    if (inside(x, y, SETTINGS_CORNER_X, SEAHORSE_Y, CORNER_W, CORNER_H)) {
        aquarium_.spawnSeahorseAtCenter();
        return true;
    }
    if (inside(x, y, SETTINGS_CORNER_X, OCTOPUS_Y, CORNER_W, CORNER_H)) {
        aquarium_.spawnOctopusAtCenter();
        return true;
    }
    return false;
}

bool Ui::handleClockColorPanelTap(int x, int y) {
    if (inside(x, y, CC_CLOSE_X, CC_CLOSE_Y, CC_CLOSE_W, CC_CLOSE_H)) {
        clockColorPanelOpen_ = false;
        return true;
    }
    for (int i = 0; i < Clock::paletteCount(); ++i) {
        const int col = i % CC_COLS, row = i / CC_COLS;
        const int sx = CC_GRID_X + col * (CC_SWATCH_W + CC_GAP_X);
        const int sy = CC_GRID_Y + row * (CC_SWATCH_H + CC_GAP_Y);
        if (inside(x, y, sx - 3, sy - 3, CC_SWATCH_W + 6, CC_SWATCH_H + 6)) {
            clock_.setActiveTextColor(Clock::paletteColor(i));
            clockColorPanelOpen_ = false;
            return true;
        }
    }
    return true;  // modal: consume taps anywhere while open
}

bool Ui::handleClockStylePanelTap(int x, int y) {
    if (inside(x, y, CS_CLOSE_X, CS_CLOSE_Y, CS_CLOSE_W, CS_CLOSE_H)) {
        clockStylePanelOpen_ = false;
        clockColorPanelOpen_ = false;
        return true;
    }
    if (inside(x, y, CS_LEFT_X, CS_ROW_1_Y, CS_CHOICE_W, BTN_H)) {
        clock_.setStyle(ClockStyle::SmallText);
        clockColorPanelOpen_ = false;
        return true;
    }
    if (inside(x, y, CS_RIGHT_X, CS_ROW_1_Y, CS_CHOICE_W, BTN_H)) {
        clock_.setStyle(ClockStyle::Ascii);
        clockColorPanelOpen_ = false;
        return true;
    }
    if (inside(x, y, CS_SWATCH_X, CS_ROW_4_Y, CS_SWATCH_W, BTN_H) ||
        inside(x, y, CS_COLOR_BTN_X, CS_ROW_4_Y, CS_COLOR_BTN_W, BTN_H)) {
        clockColorPanelOpen_ = true;
        return true;
    }
    if (inside(x, y, CS_LEFT_X, CS_ROW_3_Y, CS_CHOICE_W, BTN_H)) {
        clock_.setFlipHorizontal(false);
        return true;
    }
    if (inside(x, y, CS_RIGHT_X, CS_ROW_3_Y, CS_CHOICE_W, BTN_H)) {
        clock_.setFlipHorizontal(true);
        return true;
    }
    if (clock_.style() == ClockStyle::SmallText) {
        if (inside(x, y, CS_LEFT_X, CS_ROW_2_Y, CS_CHOICE_W, BTN_H)) {
            clock_.setSmallPosition(ClockSmallPosition::Top);
            return true;
        }
        if (inside(x, y, CS_RIGHT_X, CS_ROW_2_Y, CS_CHOICE_W, BTN_H)) {
            clock_.setSmallPosition(ClockSmallPosition::Bottom);
            return true;
        }
    }
    return true;  // modal
}

bool Ui::handleSettingsTap(int x, int y) {
    if (inside(x, y, CLOSE_X, CLOSE_Y, CLOSE_W, CLOSE_H)) {
        settingsOpen_ = false;
        clockStylePanelOpen_ = false;
        clockColorPanelOpen_ = false;
        return true;
    }
    // Tabs.
    if (inside(x, y, TANK_TAB_X, TAB_Y, TAB_W, TAB_H)) {
        activeTab_ = SettingsTab::Tank; clockStylePanelOpen_ = clockColorPanelOpen_ = false; return true;
    }
    if (inside(x, y, SEAWEED_TAB_X, TAB_Y, TAB_W, TAB_H)) {
        activeTab_ = SettingsTab::Seaweed; clockStylePanelOpen_ = clockColorPanelOpen_ = false; return true;
    }
    if (inside(x, y, CLOCK_TAB_X, TAB_Y, TAB_W, TAB_H)) {
        activeTab_ = SettingsTab::Clock; return true;
    }
    if (inside(x, y, BG_TAB_X, TAB_Y, TAB_W, TAB_H)) {
        activeTab_ = SettingsTab::Background; clockStylePanelOpen_ = clockColorPanelOpen_ = false; return true;
    }
    if (inside(x, y, SCENE_TAB_X, TAB_Y, TAB_W, TAB_H)) {
        activeTab_ = SettingsTab::Scene; clockStylePanelOpen_ = clockColorPanelOpen_ = false; return true;
    }

    if (activeTab_ == SettingsTab::Tank) {
        if (inside(x, y, MINUS_X, ROW_START_Y, BTN_W, BTN_H)) { aquarium_.nudgeFish(-1); return true; }
        if (inside(x, y, PLUS_X, ROW_START_Y, BTN_W, BTN_H))  { aquarium_.nudgeFish(1);  return true; }
        if (inside(x, y, MINUS_X, ROW_START_Y + ROW_GAP, BTN_W, BTN_H)) { aquarium_.nudgeBubbles(-1); return true; }
        if (inside(x, y, PLUS_X, ROW_START_Y + ROW_GAP, BTN_W, BTN_H))  { aquarium_.nudgeBubbles(1);  return true; }
        if (inside(x, y, MINUS_X, ROW_START_Y + ROW_GAP * 2, BTN_W, BTN_H)) { aquarium_.cycleOctopusFrequency(-1); return true; }
        if (inside(x, y, PLUS_X, ROW_START_Y + ROW_GAP * 2, BTN_W, BTN_H))  { aquarium_.cycleOctopusFrequency(1);  return true; }
        if (inside(x, y, MINUS_X, ROW_START_Y + ROW_GAP * 3, BTN_W, BTN_H)) { aquarium_.cycleSeahorseFrequency(-1); return true; }
        if (inside(x, y, PLUS_X, ROW_START_Y + ROW_GAP * 3, BTN_W, BTN_H))  { aquarium_.cycleSeahorseFrequency(1);  return true; }
    } else if (activeTab_ == SettingsTab::Seaweed) {
        if (inside(x, y, MINUS_X, ROW_START_Y, BTN_W, BTN_H)) { aquarium_.nudgeSeaweedSway(-0.05f); return true; }
        if (inside(x, y, PLUS_X, ROW_START_Y, BTN_W, BTN_H))  { aquarium_.nudgeSeaweedSway(0.05f);  return true; }
        if (inside(x, y, MINUS_X, ROW_START_Y + ROW_GAP, BTN_W, BTN_H)) { aquarium_.nudgeSeaweedLength(-0.05f); return true; }
        if (inside(x, y, PLUS_X, ROW_START_Y + ROW_GAP, BTN_W, BTN_H))  { aquarium_.nudgeSeaweedLength(0.05f);  return true; }
        if (inside(x, y, MINUS_X, ROW_START_Y + ROW_GAP * 2, BTN_W, BTN_H)) { aquarium_.nudgeSeaweedRandomness(-0.05f); return true; }
        if (inside(x, y, PLUS_X, ROW_START_Y + ROW_GAP * 2, BTN_W, BTN_H))  { aquarium_.nudgeSeaweedRandomness(0.05f);  return true; }
    } else if (activeTab_ == SettingsTab::Clock) {
        if (inside(x, y, CLOCK_STYLE_BUTTON_X, CLOCK_ROW_1_Y, CLOCK_STYLE_BUTTON_W, BTN_H)) {
            clockStylePanelOpen_ = true; clockColorPanelOpen_ = false; return true;
        }
        if (inside(x, y, MINUS_X, CLOCK_ROW_1_Y, BTN_W, BTN_H)) { clock_.setVisible(false); return true; }
        if (inside(x, y, PLUS_X, CLOCK_ROW_1_Y, BTN_W, BTN_H))  { clock_.setVisible(true);  return true; }
        if (inside(x, y, MINUS_X, CLOCK_ROW_2_Y, BTN_W, BTN_H)) { clock_.setUse24Hour(false); return true; }
        if (inside(x, y, PLUS_X, CLOCK_ROW_2_Y, BTN_W, BTN_H))  { clock_.setUse24Hour(true);  return true; }
    } else if (activeTab_ == SettingsTab::Background) {
        if (inside(x, y, MINUS_X, ROW_START_Y, BTN_W, BTN_H)) {
            int n = (static_cast<int>(bgMode_) - 1 + static_cast<int>(BackgroundMode::Count)) %
                    static_cast<int>(BackgroundMode::Count);
            bgMode_ = static_cast<BackgroundMode>(n);
            return true;
        }
        if (inside(x, y, PLUS_X, ROW_START_Y, BTN_W, BTN_H)) {
            int n = (static_cast<int>(bgMode_) + 1) % static_cast<int>(BackgroundMode::Count);
            bgMode_ = static_cast<BackgroundMode>(n);
            return true;
        }
        if (bgMode_ == BackgroundMode::Flowers &&
            inside(x, y, ACTION_X, ROW_START_Y + ROW_GAP, ACTION_W, BTN_H)) {
            background_.randomizeFlowers();
            return true;
        }
    } else {  // Scene — Off/On toggle rows
        const int r0 = SCENE_ROW_Y, r1 = SCENE_ROW_Y + SCENE_ROW_GAP,
                  r2 = SCENE_ROW_Y + SCENE_ROW_GAP * 2, r3 = SCENE_ROW_Y + SCENE_ROW_GAP * 3,
                  r4 = SCENE_ROW_Y + SCENE_ROW_GAP * 4;
        if (inside(x, y, MINUS_X, r0, BTN_W, BTN_H)) { lighting_.setDayNight(false); return true; }
        if (inside(x, y, PLUS_X, r0, BTN_W, BTN_H))  { lighting_.setDayNight(true);  return true; }
        if (inside(x, y, MINUS_X, r1, BTN_W, BTN_H)) { lighting_.setCaustics(false); return true; }
        if (inside(x, y, PLUS_X, r1, BTN_W, BTN_H))  { lighting_.setCaustics(true);  return true; }
        if (inside(x, y, MINUS_X, r2, BTN_W, BTN_H)) { props_.setEnabled(false); return true; }
        if (inside(x, y, PLUS_X, r2, BTN_W, BTN_H))  { props_.setEnabled(true);  return true; }
        if (inside(x, y, MINUS_X, r3, BTN_W, BTN_H)) { burnin_ = false; return true; }
        if (inside(x, y, PLUS_X, r3, BTN_W, BTN_H))  { burnin_ = true;  return true; }
        if (inside(x, y, MINUS_X, r4, BTN_W, BTN_H)) { autocycle_ = false; return true; }
        if (inside(x, y, PLUS_X, r4, BTN_W, BTN_H))  { autocycle_ = true;  return true; }
    }
    return true;  // tap landed inside the open Settings panel: consume it
}

bool Ui::handleTap(int x, int y) {
    // Top-left toggle works whether or not the HUD is currently shown.
    if (inside(x, y, 0, 0, 42, 26)) {
        hudVisible_ = !hudVisible_;
        if (!hudVisible_) {
            settingsOpen_ = false;
            clockStylePanelOpen_ = false;
            clockColorPanelOpen_ = false;
        }
        return true;
    }
    if (hudVisible_ && handleHudButtons(x, y)) return true;
    if (clockColorPanelOpen_) return handleClockColorPanelTap(x, y);
    if (clockStylePanelOpen_) return handleClockStylePanelTap(x, y);
    if (settingsOpen_) return handleSettingsTap(x, y);
    return false;  // not consumed: caller feeds the fish
}

}  // namespace ui
}  // namespace aq
