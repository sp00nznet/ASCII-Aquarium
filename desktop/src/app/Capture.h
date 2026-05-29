// Screenshot + sequence capture.
//
// Replaces the device's SD-card BMP capture (/AQCAP, /AQSEQ) with writes to a
// folder on disk. A single screenshot saves one timestamped 24-bit BMP; the
// sequence recorder dumps one numbered BMP per frame into a timestamped
// subfolder until toggled off. BMP keeps it dependency-free and matches the
// device's format (PNG could be added later with an encoder).

#pragma once

#include <string>

namespace aq {

class Framebuffer;

class Capture {
public:
    // `dir` overrides the default captures location when non-empty.
    explicit Capture(std::string dir = "");

    // Save one screenshot now. Returns true on success.
    bool saveScreenshot(const Framebuffer& fb);

    // Start/stop frame-sequence recording.
    void toggleSequence();
    bool sequenceActive() const { return sequenceActive_; }

    // If recording, write the current frame. No-op otherwise.
    void recordFrame(const Framebuffer& fb);

    // Frames written in the current/last sequence (for an on-screen indicator).
    unsigned long sequenceFrameCount() const { return sequenceFrame_; }

    // Short human-readable result of the last action (toast text).
    const std::string& lastStatus() const { return lastStatus_; }

    // The resolved captures directory.
    const std::string& directory() const { return dir_; }

private:
    // Best-effort encode of the just-finished sequence into mp4 (+ gif for
    // short clips) via ffmpeg, if it's on PATH. Returns a status string.
    std::string encodeSequence();
    bool ffmpegAvailable() const;

    std::string dir_;
    bool sequenceActive_ = false;
    std::string sequenceDir_;
    unsigned long sequenceFrame_ = 0;
    std::string lastStatus_;
    mutable int ffmpeg_ = -1;  // -1 unknown, 0 absent, 1 present
};

}  // namespace aq
