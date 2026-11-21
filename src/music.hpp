#pragma once
#include <array>
#include <string>

namespace zc {
enum class Quality {
    Major,
    Minor,
    Seventh,
    Major7,
    Minor7,
    Diminished,
    Augmented,
    No3,
    Sus2,
    Sus4
};
constexpr int qualityCount = 10;
constexpr std::array<unsigned, 4> combinationMasks{3, 5, 6, 7};
int combinationIndex(unsigned mask);
const char *qualityName(Quality quality);
const char *qualityDescription(Quality quality);
struct Chord {
    int root = 48, bass = -1, column = 6;
    Quality quality = Quality::Major;
    bool operator==(const Chord &b) const {
        return root == b.root && bass == b.bass && quality == b.quality;
    }
};
int columnFor(char key);
int rowFor(char key);
int rootFor(int tonic, int scale, int column);
Quality qualityFor(unsigned mask);
std::array<int, 4> chordNotes(const Chord &chord);
std::array<int, 7> harpNotes(const Chord &chord);
std::string noteName(int pitch);
std::string chordName(const Chord &chord);
const char *degreeName(int column);
struct Preset {
    const char *name;
    int chordProgram, harpProgram;
};
extern const std::array<Preset, 4> presets;
enum class PlaybackMode { Pad, Chord, Arp };
const char *modeName(PlaybackMode mode);
struct PerformanceSettings {
    PlaybackMode mode = PlaybackMode::Pad;
    int chordProgram = -1, harpProgram = -1;
    int chordPattern = 0, padMotion = 0;
    int chordVolume = 65, harpVolume = 85, echo = 15;
    int strumMs = 210, bpm = 100, pattern = 0;
    bool operator==(const PerformanceSettings &b) const;
    bool operator!=(const PerformanceSettings &b) const { return !(*this == b); }
};
struct Settings : PerformanceSettings {
    int tonic = 0, scale = 0, preset = 0;
    int hold = 1, presetLevel = 100;
    int masterVolume = 100, theme = 0;
    std::array<int, 4> combinations{int(Quality::Diminished), int(Quality::Major7),
                                    int(Quality::Minor7), int(Quality::Augmented)};
    std::array<std::array<int, 4>, 7> degreeCombinations = [] {
        std::array<std::array<int, 4>, 7> values{};
        for (auto &row : values)
            row.fill(-1);
        return values;
    }();
    Quality quality(unsigned mask, int column) const;
    void clamp();
};
} // namespace zc
