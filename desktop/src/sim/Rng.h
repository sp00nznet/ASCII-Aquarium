// Arduino-compatible pseudo-random generator.
//
// The upstream sketch leans on Arduino's global random()/randomSeed() and a
// frand() helper built on them. Exact bit-for-bit reproduction of the ESP32's
// sequence isn't meaningful on desktop (it's seeded from esp_random()), so this
// only reproduces the *semantics* — the ranges and distribution — which is all
// the visual simulation depends on.

#pragma once

#include <cstdint>
#include <random>

namespace aq {

class Rng {
public:
    // Seedable for reproducible runs; defaults to a nondeterministic seed.
    explicit Rng(std::uint32_t seed) : engine_(seed) {}
    Rng() : engine_(std::random_device{}()) {}

    void seed(std::uint32_t s) { engine_.seed(s); }

    // Arduino random(howbig): uniform in [0, howbig). Returns 0 if howbig <= 0.
    long random(long howbig) {
        if (howbig <= 0) return 0;
        return static_cast<long>(engine_() % static_cast<std::uint32_t>(howbig));
    }

    // Arduino random(howsmall, howbig): uniform in [howsmall, howbig).
    long random(long howsmall, long howbig) {
        if (howsmall >= howbig) return howsmall;
        return howsmall + random(howbig - howsmall);
    }

    // Upstream frand(a, b): uniform float in [a, b], matching the original's
    // a + (b - a) * random(0, 10000) / 9999 construction.
    float frand(float a, float b) {
        return a + (b - a) * static_cast<float>(random(0, 10000)) / 9999.0f;
    }

private:
    std::mt19937 engine_;
};

}  // namespace aq
