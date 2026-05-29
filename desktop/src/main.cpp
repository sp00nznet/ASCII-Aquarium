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

#include "renderer/Color.h"
#include "renderer/Framebuffer.h"
#include "sim/Aquarium.h"
#include "sim/Rng.h"

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
    aquarium.init();

    // Animation clock: monotonic ms since the loop started, so the simulation's
    // time base is independent of how long setup took.
    const Uint32 start_ticks = SDL_GetTicks();
    Uint32 last_ticks = start_ticks;

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
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (ev.button.button == SDL_BUTTON_LEFT) {
                        // Map window pixel coords to the 320x240 logical canvas.
                        float lx = 0.0f, ly = 0.0f;
                        SDL_RenderWindowToLogical(renderer, ev.button.x, ev.button.y, &lx, &ly);
                        aquarium.spawnFlake(lx, ly);
                    }
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

        aquarium.update(dt, now_ticks - start_ticks);
        aquarium.draw(fb);

        SDL_UpdateTexture(texture, nullptr, fb.data(),
            static_cast<int>(fb.width() * sizeof(std::uint16_t)));
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
