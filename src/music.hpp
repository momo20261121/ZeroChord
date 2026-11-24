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
    int chordProgram, harpProgram, padProgram = -1;
};
extern const std::array<Preset, 4> presets;
// Preserve the four original persisted values; append combinations.
enum class PlaybackMode { Pad, Chord, Arp, FrontOnly, ChordPad, ChordArp, PadArp, All };
unsigned backingLayers(PlaybackMode mode); // Chord=1, Pad=2, Arp=4.
PlaybackMode modeForLayers(unsigned layers);
bool layerEnabled(PlaybackMode selection, PlaybackMode layer);
const char *modeName(PlaybackMode mode);
struct Envelope {
    // Attack/decay/release in milliseconds, sustain in percent; -1 keeps the sound default.
    std::array<int, 4> values{-1, -1, -1, -1};
    bool operator==(const Envelope &b) const { return values == b.values; }
    bool operator!=(const Envelope &b) const { return !(*this == b); }
};
struct PerformanceSettings {
    std::array<Envelope, 3> envelopes; // Front, Chord (including Arp), Pad.

    PlaybackMode mode = PlaybackMode::Pad;
    int chordProgram = -1, harpProgram = -1, padProgram = -1;
    std::string chordSound, padSound, frontSound;
    int chordPattern = 0, padMotion = 0;
    int bpm = 100, pattern = 0;
    bool operator==(const PerformanceSettings &b) const;
    bool operator!=(const PerformanceSettings &b) const { return !(*this == b); }
};
struct Settings : PerformanceSettings {
    int tonic = 0, scale = 0, preset = 1;
    int hold = 1;
    int chordVolume = 80, harpVolume = 100, echo = 15, strumMs = 210;
    int masterVolume = 100, theme = 0;
    std::array<int, 3> eqDb{}; // App-wide Low / Mid / High gain, in dB.
    int chordOctave = 3;
    std::array<int, 7> degreeChordRoot{-1, -1, -1, -1, -1, -1, -1}; // -1 follows Key; otherwise absolute pitch class.
    std::array<int, 7> degreeChordOctave{-2, -2, -2, -2, -2, -2, -2}; // -2 inherits All.
    int chordRoot(int column) const;
    static constexpr int maxCombinationMs = 120;
    int combinationMs = 30; // Fixed recognition window from the first chord button.
    int frontMode = 0;                    // 0 chord notes, 1 selected key/scale
    std::array<int, 2> frontAction{0, 1}; // 0 strum, 1 octave shift, per front mode
    int scaleStrumMs = 210;
    std::array<int, 7> frontSource{-1, -1, -1, -1,
                                   -1, -1, -1}; // -1 auto, 0–3 chord tone, 4 semitone
    std::array<int, 7> frontOffset{};
    // -2 follows the original layout; other values are actual MIDI note octaves.
    std::array<int, 7> frontOctave{-2, -2, -2, -2, -2, -2, -2};
    std::array<int, 7> frontDegree{0, 1, 2, 3, 4, 5, 6};
    std::array<int, 7> scaleOctave{-2, -2, -2, -2, -2, -2, -2};
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
std::array<int, 7> frontNotes(const Chord &chord, const Settings &settings);
} // namespace zc
