// ASCII Aquarium desktop port — main entry.
//
// Opens an SDL2 window backed by a 320x240 RGB565 Framebuffer (matching the
// CYD's native format), runs a vsync'd main loop, and supports kiosk-style
// CLI flags. The loop drives the aquarium simulation; clicking/tapping drops a
// food flake. Backgrounds, HUD, and settings panels arrive in later tasks.

#include <SDL.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "renderer/Color.h"
#include "renderer/Framebuffer.h"
#include "sim/Aquarium.h"
#include "sim/Background.h"
#include "sim/Clock.h"
#include "sim/Rng.h"
#include "ui/Ui.h"
#include "app/Capture.h"
#include "app/Config.h"

namespace {

constexpr int kBackingWidth = 320;
constexpr int kBackingHeight = 240;
constexpr const char* kWindowTitle = "ASCII Aquarium";

struct Options {
    int scale = 2;
    bool fullscreen = false;
    bool hide_cursor = false;
    std::string capture_dir;
};

void print_usage(const char* argv0) {
    std::printf(
        "Usage: %s [options]\n"
        "\n"
        "  --scale N         integer scale factor for windowed mode (1..16, default 2)\n"
        "  --fullscreen      borderless fullscreen, content scaled to fit\n"
        "  --hide-cursor     hide the mouse cursor (kiosk mode)\n"
        "  --capture-dir D   directory for screenshots/sequences (default: app data dir)\n"
        "  -h, --help        show this help and exit\n"
        "\n"
        "Runtime keys:\n"
        "  Escape            quit\n"
        "  F11               toggle fullscreen\n"
        "  F2                save a screenshot\n"
        "  F3                start/stop sequence recording\n",
        argv0);
}

bool parse_args(int argc, char* argv[], Options& opts) {
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (std::strcmp(a, "--fullscreen") == 0) {
            opts.fullscreen = true;
        } else if (std::strcmp(a, "--hide-cursor") == 0) {
            opts.hide_cursor = true;
        } else if (std::strcmp(a, "--capture-dir") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "--capture-dir requires a value\n");
                return false;
            }
            opts.capture_dir = argv[i];
        } else if (std::strcmp(a, "--scale") == 0) {
            if (++i >= argc) {
                std::fprintf(stderr, "--scale requires a value\n");
                return false;
            }
            const int s = std::atoi(argv[i]);
            if (s < 1 || s > 16) {
                std::fprintf(stderr, "--scale must be between 1 and 16\n");
                return false;
            }
            opts.scale = s;
        } else if (std::strcmp(a, "-h") == 0 || std::strcmp(a, "--help") == 0) {
            print_usage(argv[0]);
            std::exit(0);
        } else {
            std::fprintf(stderr, "Unknown option: %s\n", a);
            print_usage(argv[0]);
            return false;
        }
    }
    return true;
}

bool is_fullscreen(SDL_Window* window) {
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    Options opts;
    if (!parse_args(argc, argv, opts)) return 1;

    // Become DPI-aware before SDL inits its video subsystem. Without this, a
    // Windows display set above 100% scaling virtualizes the process: the OS
    // reports window/render sizes in physical pixels but delivers mouse events
    // in scaled (virtual) coordinates, so clicks land at 1/scale of where they
    // should. Declaring per-monitor-v2 awareness makes the OS report everything
    // in real pixels, so SDL_RenderWindowToLogical maps clicks correctly.
    // (Harmless / ignored on non-Windows platforms.)
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
        return 1;
    }

    const Uint32 window_flags = opts.fullscreen
        ? static_cast<Uint32>(SDL_WINDOW_FULLSCREEN_DESKTOP)
        : Uint32{0};
    const int win_w = opts.fullscreen ? kBackingWidth : kBackingWidth * opts.scale;
    const int win_h = opts.fullscreen ? kBackingHeight : kBackingHeight * opts.scale;

    SDL_Window* window = SDL_CreateWindow(
        kWindowTitle,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        window_flags);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, kBackingWidth, kBackingHeight);
    SDL_RenderSetIntegerScale(renderer, opts.fullscreen ? SDL_FALSE : SDL_TRUE);

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        kBackingWidth, kBackingHeight);
    if (!texture) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (opts.hide_cursor) SDL_ShowCursor(SDL_DISABLE);

    aq::Framebuffer fb(kBackingWidth, kBackingHeight);

    aq::Rng rng(static_cast<std::uint32_t>(SDL_GetPerformanceCounter()));
    aq::Aquarium aquarium(rng);
    aq::Background background(rng);
    aq::BackgroundMode bg_mode = aq::BackgroundMode::BlueGradient;
    aq::Clock clock;

    // Load saved settings (if any) and apply them before populating the tank,
    // so the fish count etc. are correct on the first frame.
    const std::string config_path = aq::config::filePath();
    aq::config::Settings settings = aq::config::snapshot(aquarium, clock, bg_mode);
    aq::config::load(config_path, settings);
    aq::config::apply(settings, aquarium, clock, bg_mode);
    aquarium.init();

    aq::ui::Ui ui(aquarium, background, clock, bg_mode);
    aq::Capture capture(opts.capture_dir);

    // Capture state: a pending single-shot flag and a transient on-screen toast.
    bool screenshot_pending = false;
    Uint32 toast_until_ticks = 0;
    auto show_toast = [&](Uint32 now) { toast_until_ticks = now + 1800; };

    // Debounced settings autosave: save 1.2s after the last change (matching
    // the device), plus a final save on exit.
    aq::config::Settings saved_settings = aq::config::snapshot(aquarium, clock, bg_mode);
    aq::config::Settings prev_settings = saved_settings;
    Uint32 last_change_ticks = 0;

    // Animation clock: monotonic ms since the loop started, so the simulation's
    // time base is independent of how long setup took.
    const Uint32 start_ticks = SDL_GetTicks();
    Uint32 last_ticks = start_ticks;
    float fps = 0.0f;

    // Tap pipeline: both mouse clicks and touchscreen taps funnel through here,
    // debounced like the device's TOUCH_DEBOUNCE_MS so a single touch (which SDL
    // also reports as a synthetic mouse click) only registers once.
    Uint32 last_tap_ticks = 0;
    constexpr Uint32 kTapDebounceMs = 160;
    auto dispatch_tap = [&](float lx, float ly) {
        const Uint32 t = SDL_GetTicks();
        if (t - last_tap_ticks < kTapDebounceMs) return;
        last_tap_ticks = t;
        const int x = static_cast<int>(lx);
        const int y = static_cast<int>(ly);
        // The UI gets first crack at the tap (HUD/Settings hit-tests). Only a
        // tap that lands in open water falls through to feeding the fish.
        if (ui.handleTap(x, y)) return;
        aquarium.spawnFlake(static_cast<float>(x), static_cast<float>(y));
    };
    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if (ev.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    } else if (ev.key.keysym.sym == SDLK_F11) {
                        SDL_SetWindowFullscreen(window,
                            is_fullscreen(window) ? Uint32{0}
                                                  : static_cast<Uint32>(SDL_WINDOW_FULLSCREEN_DESKTOP));
                        SDL_RenderSetIntegerScale(renderer,
                            is_fullscreen(window) ? SDL_FALSE : SDL_TRUE);
                    }
                    // Temporary debug keys until the Settings panel (task #8)
                    // exposes these controls through the on-screen UI.
                    else if (ev.key.keysym.sym == SDLK_b) {
                        bg_mode = static_cast<aq::BackgroundMode>(
                            (static_cast<int>(bg_mode) + 1) %
                            static_cast<int>(aq::BackgroundMode::Count));
                    } else if (ev.key.keysym.sym == SDLK_c) {
                        clock.toggleVisible();
                    } else if (ev.key.keysym.sym == SDLK_v) {
                        clock.cycleStyle();
                    } else if (ev.key.keysym.sym == SDLK_h) {
                        clock.setUse24Hour(!clock.use24Hour());
                    } else if (ev.key.keysym.sym == SDLK_m) {
                        clock.setFlipHorizontal(!clock.flipHorizontal());
                    } else if (ev.key.keysym.sym == SDLK_F2) {
                        screenshot_pending = true;  // serviced after the frame draws
                    } else if (ev.key.keysym.sym == SDLK_F3) {
                        capture.toggleSequence();
                        show_toast(SDL_GetTicks());
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (ev.button.button == SDL_BUTTON_LEFT) {
                        // With a renderer logical size set, SDL already reports
                        // mouse event coords in the logical (320x240) space, so
                        // they're used directly — no further scaling.
                        dispatch_tap(static_cast<float>(ev.button.x),
                                     static_cast<float>(ev.button.y));
                    }
                    break;
                case SDL_FINGERDOWN:
                    // Touch coords are normalized [0,1]; scale to the canvas.
                    dispatch_tap(ev.tfinger.x * kBackingWidth,
                                 ev.tfinger.y * kBackingHeight);
                    break;
                default:
                    break;
            }
        }

        const Uint32 now_ticks = SDL_GetTicks();
        float dt = static_cast<float>(now_ticks - last_ticks) * 0.001f;
        last_ticks = now_ticks;
        // Clamp dt so a stall (window drag, breakpoint) can't teleport entities.
        if (dt > 0.1f) dt = 0.1f;
        if (dt > 0.0f) fps = fps * 0.9f + (1.0f / dt) * 0.1f;  // smoothed

        // Debounced settings autosave: reset the timer whenever a setting
        // changes, and write once it has been stable for 1.2s.
        {
            aq::config::Settings cur = aq::config::snapshot(aquarium, clock, bg_mode);
            if (cur != prev_settings) {
                last_change_ticks = now_ticks;
                prev_settings = cur;
            }
            if (cur != saved_settings && now_ticks - last_change_ticks >= 1200) {
                if (aq::config::save(config_path, cur)) saved_settings = cur;
            }
        }

        aquarium.update(dt, now_ticks - start_ticks);
        clock.update();

        // Draw order mirrors the sketch: background, the big ASCII clock layer
        // (behind the fish), the live scene, the small clock overlay, then the
        // HUD/Settings chrome on top.
        background.draw(fb, bg_mode);
        clock.drawBackgroundLayer(fb);
        aquarium.draw(fb);
        clock.drawOverlay(fb);
        ui.draw(fb, fps);

        // Capture the composited frame (before the capture indicator is drawn,
        // so screenshots/recordings don't include the REC/toast overlay).
        capture.recordFrame(fb);
        if (screenshot_pending) {
            screenshot_pending = false;
            capture.saveScreenshot(fb);
            show_toast(now_ticks);
        }

        // Capture indicator + transient toast (display only).
        if (capture.sequenceActive()) {
            char rec[24];
            std::snprintf(rec, sizeof(rec), "REC %lu", capture.sequenceFrameCount());
            fb.setTextFont(2);
            fb.setTextDatum(TR_DATUM);
            fb.setTextColor(TFT_RED);
            fb.drawString(rec, kBackingWidth - 6, kBackingHeight - 18);
        }
        if (now_ticks < toast_until_ticks && !capture.lastStatus().empty()) {
            fb.setTextFont(2);
            fb.setTextDatum(BC_DATUM);
            fb.setTextColor(TFT_WHITE, TFT_NAVY);
            fb.drawString(capture.lastStatus().c_str(), kBackingWidth / 2, kBackingHeight - 4);
        }

        SDL_UpdateTexture(texture, nullptr, fb.data(),
            static_cast<int>(fb.width() * sizeof(std::uint16_t)));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Flush any unsaved settings on exit.
    {
        aq::config::Settings cur = aq::config::snapshot(aquarium, clock, bg_mode);
        if (cur != saved_settings) aq::config::save(config_path, cur);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
