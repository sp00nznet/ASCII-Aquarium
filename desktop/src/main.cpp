// ASCII Aquarium desktop port — scaffold entry point.
//
// At this stage main() just validates the SDL2 build/link and exits cleanly.
// The real window + frame loop arrives in the next task.

#include <SDL.h>

#include <cstdio>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);

    std::printf("ascii-aquarium-desktop scaffold OK\n");
    std::printf("  SDL2 compiled against %u.%u.%u, linked %u.%u.%u\n",
                compiled.major, compiled.minor, compiled.patch,
                linked.major, linked.minor, linked.patch);

    SDL_Quit();
    return 0;
}
