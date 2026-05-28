// ASCII Aquarium desktop port — main entry.
//
// Opens an SDL2 window backed by a 320x240 RGB565 Framebuffer (matching the
// CYD's native format), runs a vsync'd main loop, and supports kiosk-style
// CLI flags. The aquarium simulation, fonts, and UI panels arrive in later
// tasks; for now the loop renders a placeholder pattern exercising the
// freshly-landed renderer primitives so it's visually obvious the loop and
// the drawing pipeline both work.

#include <SDL.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "renderer/Color.h"
#include "renderer/Framebuffer.h"

namespace {

constexpr int kBackingWidth = 320;
constexpr int kBackingHeight = 240;
constexpr const char* kWindowTitle = "ASCII Aquarium";

struct Options {
    int scale = 2;
    bool fullscreen = false;
    bool hide_cursor = false;
};

void print_usage(const char* argv0) {
    std::printf(
        "Usage: %s [options]\n"
        "\n"
        "  --scale N         integer scale factor for windowed mode (1..16, default 2)\n"
        "  --fullscreen      borderless fullscreen, content scaled to fit\n"
        "  --hide-cursor     hide the mouse cursor (kiosk mode)\n"
        "  -h, --help        show this help and exit\n"
        "\n"
        "Runtime keys:\n"
        "  Escape            quit\n"
        "  F11               toggle fullscreen\n",
        argv0);
}

bool parse_args(int argc, char* argv[], Options& opts) {
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (std::strcmp(a, "--fullscreen") == 0) {
            opts.fullscreen = true;
        } else if (std::strcmp(a, "--hide-cursor") == 0) {
            opts.hide_cursor = true;
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

// Temporary "is it alive" splash. Exercises the renderer primitives landed in
// task #3 (fillScreen / fillRoundRect / drawRoundRect / drawLine) AND the
// bitmap text landed in task #4 (setTextFont / setTextDatum / setTextColor /
// drawString / textWidth), so a successful build immediately shows whether the
// renderer wiring AND both fonts render correctly end to end. Replaced once the
// aquarium simulation lands.
void render_splash(aq::Framebuffer& fb, unsigned frame) {
    fb.fillScreen(TFT_NAVY);

    // Soft drifting band so it's obvious the loop is running.
    const int band_y = static_cast<int>((frame / 2u) % static_cast<unsigned>(kBackingHeight));
    fb.fillRect(0, band_y, kBackingWidth, 6, aq::rgb565(40, 64, 96));

    // Centered "panel" preview using the round-rect primitives.
    const int pw = 220;
    const int ph = 110;
    const int px = (kBackingWidth - pw) / 2;
    const int py = (kBackingHeight - ph) / 2;
    fb.fillRoundRect(px,     py,     pw,     ph,     8, TFT_BLACK);
    fb.drawRoundRect(px,     py,     pw,     ph,     8, TFT_CYAN);
    fb.drawRoundRect(px + 2, py + 2, pw - 4, ph - 4, 7, TFT_DARKCYAN);

    const int cx = kBackingWidth / 2;

    // Title in Font 2 (16px proportional), centered via the MC datum.
    fb.setTextFont(2);
    fb.setTextDatum(MC_DATUM);
    fb.setTextColor(TFT_GOLD);
    const char* title = "ASCII Aquarium";
    fb.drawString(title, cx, py + 34);

    // Cyan underline sized by textWidth — proves measurement matches rendering.
    const int tw = fb.textWidth(title);
    fb.drawLine(cx - tw / 2, py + 48, cx + tw / 2, py + 48, TFT_CYAN);

    // Subtitle in Font 1 (GLCD 6x8), centered.
    fb.setTextFont(1);
    fb.setTextColor(TFT_GREENYELLOW);
    fb.drawString("desktop port", cx, py + 66);

    // Animated frame counter, also Font 1, to confirm the loop advances.
    char buf[32];
    std::snprintf(buf, sizeof(buf), "frame %u", frame);
    fb.setTextColor(TFT_DARKCYAN);
    fb.drawString(buf, cx, py + 88);
}

bool is_fullscreen(SDL_Window* window) {
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    Options opts;
    if (!parse_args(argc, argv, opts)) return 1;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
        return 1;
    }

    const Uint32 window_flags = opts.fullscreen
        ? SDL_WINDOW_FULLSCREEN_DESKTOP
        : 0u;
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

    bool running = true;
    unsigned frame = 0;
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
                            is_fullscreen(window) ? 0u : SDL_WINDOW_FULLSCREEN_DESKTOP);
                        SDL_RenderSetIntegerScale(renderer,
                            is_fullscreen(window) ? SDL_FALSE : SDL_TRUE);
                    }
                    break;
                default:
                    break;
            }
        }

        render_splash(fb, frame);

        SDL_UpdateTexture(texture, nullptr, fb.data(),
            static_cast<int>(fb.width() * sizeof(std::uint16_t)));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        ++frame;
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
