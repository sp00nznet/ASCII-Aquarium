#include "app/Capture.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <vector>

#include "renderer/Framebuffer.h"

namespace fs = std::filesystem;

namespace aq {

namespace {

fs::path defaultCaptureDir() {
#if defined(_WIN32)
    const char* base = std::getenv("APPDATA");
    fs::path dir = (base && base[0]) ? fs::path(base) : fs::path(".");
    return dir / "ascii-aquarium" / "captures";
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    const char* home = std::getenv("HOME");
    fs::path dir;
    if (xdg && xdg[0]) dir = fs::path(xdg);
    else if (home && home[0]) dir = fs::path(home) / ".local" / "share";
    else dir = fs::path(".");
    return dir / "ascii-aquarium" / "captures";
#endif
}

// "20260528-195412" — safe for filenames, sortable.
std::string timestamp() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d-%02d%02d%02d",
                  local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                  local.tm_hour, local.tm_min, local.tm_sec);
    return buf;
}

// Write the framebuffer (RGB565) as a bottom-up 24-bit BMP.
bool writeBmp(const fs::path& path, const Framebuffer& fb) {
    const int w = fb.width(), h = fb.height();
    if (w <= 0 || h <= 0) return false;
    const int rowBytes = (w * 3 + 3) & ~3;
    const std::uint32_t dataSize = static_cast<std::uint32_t>(rowBytes) * h;
    const std::uint32_t fileSize = 54 + dataSize;

    std::ofstream out(path, std::ios::binary);
    if (!out) return false;

    std::uint8_t hdr[54] = {0};
    hdr[0] = 'B'; hdr[1] = 'M';
    hdr[2] = fileSize & 0xFF; hdr[3] = (fileSize >> 8) & 0xFF;
    hdr[4] = (fileSize >> 16) & 0xFF; hdr[5] = (fileSize >> 24) & 0xFF;
    hdr[10] = 54;
    hdr[14] = 40;
    hdr[18] = w & 0xFF; hdr[19] = (w >> 8) & 0xFF;
    hdr[20] = (w >> 16) & 0xFF; hdr[21] = (w >> 24) & 0xFF;
    hdr[22] = h & 0xFF; hdr[23] = (h >> 8) & 0xFF;
    hdr[24] = (h >> 16) & 0xFF; hdr[25] = (h >> 24) & 0xFF;
    hdr[26] = 1; hdr[28] = 24;
    out.write(reinterpret_cast<const char*>(hdr), 54);

    const std::uint16_t* px = fb.data();
    std::vector<std::uint8_t> row(rowBytes, 0);
    for (int y = h - 1; y >= 0; --y) {  // BMP rows run bottom-up
        for (int x = 0; x < w; ++x) {
            const std::uint16_t c = px[y * w + x];
            const int r5 = (c >> 11) & 0x1F, g6 = (c >> 5) & 0x3F, b5 = c & 0x1F;
            row[x * 3 + 0] = static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2));  // B
            row[x * 3 + 1] = static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4));  // G
            row[x * 3 + 2] = static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2));  // R
        }
        out.write(reinterpret_cast<const char*>(row.data()), rowBytes);
    }
    return static_cast<bool>(out);
}

}  // namespace

Capture::Capture(std::string dir)
    : dir_(dir.empty() ? defaultCaptureDir().string() : std::move(dir)) {}

bool Capture::saveScreenshot(const Framebuffer& fb) {
    std::error_code ec;
    fs::create_directories(dir_, ec);
    fs::path path = fs::path(dir_) / ("aquarium-" + timestamp() + ".bmp");
    // Disambiguate if two shots land in the same second.
    int suffix = 2;
    while (fs::exists(path)) {
        path = fs::path(dir_) / ("aquarium-" + timestamp() + "-" + std::to_string(suffix++) + ".bmp");
    }
    if (writeBmp(path, fb)) {
        lastStatus_ = "Saved " + path.filename().string();
        std::printf("[capture] %s\n", path.string().c_str());
        return true;
    }
    lastStatus_ = "Capture failed";
    std::fprintf(stderr, "[capture] failed to write %s\n", path.string().c_str());
    return false;
}

bool Capture::ffmpegAvailable() const {
    if (ffmpeg_ < 0) {
#if defined(_WIN32)
        ffmpeg_ = (std::system("ffmpeg -version >NUL 2>&1") == 0) ? 1 : 0;
#else
        ffmpeg_ = (std::system("ffmpeg -version >/dev/null 2>&1") == 0) ? 1 : 0;
#endif
    }
    return ffmpeg_ == 1;
}

std::string Capture::encodeSequence() {
    const std::string in = (fs::path(sequenceDir_) / "frame%06d.bmp").string();
    const std::string mp4 = sequenceDir_ + ".mp4";  // sibling of the frames dir
    std::string cmd = "ffmpeg -y -loglevel error -framerate 60 -start_number 0 -i \"" +
                      in + "\" -pix_fmt yuv420p \"" + mp4 + "\"";
    if (std::system(cmd.c_str()) != 0) {
        std::fprintf(stderr, "[capture] ffmpeg mp4 encode failed\n");
        return "Encode failed (ffmpeg)";
    }
    std::printf("[capture] encoded %s\n", mp4.c_str());
    // A gif too, but only for short clips (they balloon fast).
    if (sequenceFrame_ <= 900) {
        const std::string gif = sequenceDir_ + ".gif";
        std::string g = "ffmpeg -y -loglevel error -framerate 30 -start_number 0 -i \"" +
                        in + "\" -vf \"fps=30,scale=320:-1:flags=neighbor\" \"" + gif + "\"";
        std::system(g.c_str());  // best-effort
    }
    return "Encoded " + fs::path(mp4).filename().string();
}

void Capture::toggleSequence() {
    if (sequenceActive_) {
        sequenceActive_ = false;
        std::printf("[capture] sequence stopped: %lu frames in %s\n",
                    sequenceFrame_, sequenceDir_.c_str());
        if (sequenceFrame_ > 0 && ffmpegAvailable()) {
            lastStatus_ = encodeSequence();
        } else {
            lastStatus_ = "Sequence: " + std::to_string(sequenceFrame_) + " frames" +
                          (ffmpegAvailable() ? "" : " (install ffmpeg to encode)");
        }
        return;
    }
    std::error_code ec;
    sequenceDir_ = (fs::path(dir_) / ("sequence-" + timestamp())).string();
    fs::create_directories(sequenceDir_, ec);
    sequenceFrame_ = 0;
    sequenceActive_ = true;
    lastStatus_ = "Recording sequence";
    std::printf("[capture] sequence recording to %s\n", sequenceDir_.c_str());
}

void Capture::recordFrame(const Framebuffer& fb) {
    if (!sequenceActive_) return;
    char name[32];
    std::snprintf(name, sizeof(name), "frame%06lu.bmp", sequenceFrame_);
    writeBmp(fs::path(sequenceDir_) / name, fb);
    ++sequenceFrame_;
}

}  // namespace aq
