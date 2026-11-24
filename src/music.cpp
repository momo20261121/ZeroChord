#include "music.hpp"
#include <algorithm>
#include <tuple>

namespace zc {
const std::array<Preset, 4> presets = {
    {{"Harp", 48, 46}, {"Dream", 88, 10, 89}, {"Keys", 4, 24}, {"Organ", 16, 11}}};
int columnFor(char key) {
    for (const std::string row : {"2345678", "qwertyu", "asdfghj"}) {
        auto n = row.find(key);
        if (n != std::string::npos)
            return int(n);
    }
    return -1;
}
int rowFor(char key) {
    if (std::string("2345678").find(key) != std::string::npos)
        return 0;
    if (std::string("qwertyu").find(key) != std::string::npos)
        return 1;
    if (std::string("asdfghj").find(key) != std::string::npos)
        return 2;
    return -1;
}
int rootFor(int tonic, int scale, int column) {
    static constexpr int intervals[2][7] = {{0, 2, 4, 5, 7, 9, 11}, {0, 2, 3, 5, 7, 8, 10}};
    return 48 + (tonic + intervals[scale != 0][(column + 1) % 7]) % 12;
}
Quality qualityFor(unsigned mask) {
    switch (mask) {
    case 2:
        return Quality::Minor;
    case 4:
        return Quality::Seventh;
    case 5:
        return Quality::Major7;
    case 6:
        return Quality::Minor7;
    case 3:
        return Quality::Diminished;
    case 7:
        return Quality::Augmented;
    default:
        return Quality::Major;
    }
}
int combinationIndex(unsigned mask) {
    for (int n = 0; n < 4; n++)
        if (combinationMasks[n] == mask)
            return n;
    return -1;
}
const char *qualityName(Quality q) {
    static const char *names[] = {"maj", "min", "7",   "maj7", "m7",
                                  "dim", "aug", "no3", "sus2", "sus4"};
    return names[int(q)];
}
const char *qualityDescription(Quality q) {
    static const char *names[] = {"Major",
                                  "Minor",
                                  "Dominant seventh",
                                  "Major seventh",
                                  "Minor seventh",
                                  "Diminished",
                                  "Augmented",
                                  "Root + fifth (no third)",
                                  "Second replaces third",
                                  "Fourth replaces third"};
    return names[int(q)];
}
Quality Settings::quality(unsigned mask, int column) const {
    int index = combinationIndex(mask);
    if (index < 0)
        return qualityFor(mask);
    int value = degreeCombinations[column][index];
    return Quality(value < 0 ? combinations[index] : value);
}
std::array<int, 4> chordNotes(const Chord &c) {
    // The interval choices follow minichord's documented combination qualities.
    static constexpr int intervals[qualityCount][4] = {
        {0, 4, 7, 12}, {0, 3, 7, 12}, {0, 4, 7, 10},  {0, 4, 7, 11}, {0, 3, 7, 10},
        {0, 3, 6, 12}, {0, 4, 8, 12}, {0, 7, 12, 19}, {0, 2, 7, 12}, {0, 5, 7, 12}};
    std::array<int, 4> n{};
    for (int i = 0; i < 4; i++)
        n[i] = c.root + intervals[int(c.quality)][i];
    if (c.bass >= 0) {
        int bass = c.bass;
        while (bass >= c.root)
            bass -= 12;
        n[0] = std::max(0, bass);
    }
    return n;
}
std::array<int, 7> harpNotes(const Chord &c) {
    auto notes = chordNotes(Chord{c.root, -1, c.column, c.quality});
    const bool seventh = c.quality == Quality::Seventh || c.quality == Quality::Major7 ||
                         c.quality == Quality::Minor7;
    std::array<int, 7> out{};
    const int count = c.quality == Quality::No3 ? 2 : (seventh ? 4 : 3);
    for (int i = 0; i < 7; i++)
        out[i] = notes[i % count] + 12 * (1 + i / count);
    return out;
}
std::string noteName(int pitch) {
    static const char *names[] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
    return names[(pitch % 12 + 12) % 12];
}
std::string chordName(const Chord &c) {
    static const char *suffix[] = {"", "m", "7", "maj7", "m7", "dim", "aug", "no3", "sus2", "sus4"};
    return noteName(c.root) + suffix[int(c.quality)] + (c.bass < 0 ? "" : "/" + noteName(c.bass));
}
const char *degreeName(int column) {
    static const char *names[] = {"II", "III", "IV", "V", "VI", "VII", "I"};
    return names[column];
}
unsigned backingLayers(PlaybackMode mode) {
    static constexpr unsigned masks[]{2, 1, 4, 0, 3, 5, 6, 7};
    return masks[std::clamp(int(mode), 0, 7)];
}
PlaybackMode modeForLayers(unsigned layers) {
    for (int n = 0; n < 8; ++n)
        if (backingLayers(PlaybackMode(n)) == (layers & 7u)) return PlaybackMode(n);
    return PlaybackMode::FrontOnly;
}
bool layerEnabled(PlaybackMode selection, PlaybackMode layer) {
    return (backingLayers(selection) & backingLayers(layer)) != 0;
}
const char *modeName(PlaybackMode mode) {
    static const char *names[] = {"Pad", "Chord", "Arp", "Front only", "Chord + Pad",
                                 "Chord + Arp", "Pad + Arp", "Chord + Pad + Arp"};
    return names[std::clamp(int(mode), 0, 7)];
}
bool PerformanceSettings::operator==(const PerformanceSettings &b) const {
    return std::tie(mode, chordProgram, harpProgram, padProgram, chordSound, padSound, frontSound,
                    chordPattern, padMotion, bpm, pattern, envelopes) ==
           std::tie(b.mode, b.chordProgram, b.harpProgram, b.padProgram, b.chordSound, b.padSound,
                    b.frontSound, b.chordPattern, b.padMotion, b.bpm, b.pattern, b.envelopes);
}
int Settings::chordRoot(int column) const {
    int octave = degreeChordOctave[column] == -2 ? chordOctave : degreeChordOctave[column];
    int pitch = degreeChordRoot[column] < 0 ? rootFor(tonic, 0, column) % 12 : degreeChordRoot[column];
    return pitch + 12 * (octave + 1);
}
void Settings::clamp() {
    chordOctave = std::clamp(chordOctave, -1, 7);
    for (auto &root : degreeChordRoot) root = std::clamp(root, -1, 11);
    for (auto &octave : degreeChordOctave) octave = std::clamp(octave, -2, 7);
    for (auto &envelope : envelopes)
        for (int n = 0; n < 4; n++)
            envelope.values[n] = std::clamp(envelope.values[n], -1, n == 2 ? 100 : 5000);
    combinationMs = std::clamp(combinationMs, 0, maxCombinationMs);
    frontMode = std::clamp(frontMode, 0, 1);
    for (auto &action : frontAction)
        action = std::clamp(action, 0, 1);
    scaleStrumMs = std::clamp(scaleStrumMs, 60, 600);
    for (int n = 0; n < 7; n++) {
        frontSource[n] = std::clamp(frontSource[n], -1, 4);
        frontOffset[n] = std::clamp(frontOffset[n], 0, 11);
        frontOctave[n] = std::clamp(frontOctave[n], -2, 9);
        frontDegree[n] = std::clamp(frontDegree[n], 0, 6);
        scaleOctave[n] = std::clamp(scaleOctave[n], -2, 9);
    }
    tonic = std::clamp(tonic, 0, 11);
    scale = std::clamp(scale, 0, 1);
    preset = std::clamp(preset, 0, 127);
    mode = PlaybackMode(std::clamp(int(mode), 0, 7));
    chordProgram = std::clamp(chordProgram, -1, 127);
    harpProgram = std::clamp(harpProgram, -1, 127);
    padProgram = std::clamp(padProgram, -1, 127);
    chordPattern = std::clamp(chordPattern, 0, 7);
    padMotion = std::clamp(padMotion, 0, 3);
    hold = !!hold;
    masterVolume = std::clamp(masterVolume, 0, 100);
    for (auto &gain : eqDb) gain = std::clamp(gain, -12, 12);
    theme = std::clamp(theme, 0, 1);
    chordVolume = std::clamp(chordVolume, 0, 100);
    harpVolume = std::clamp(harpVolume, 0, 100);
    echo = std::clamp(echo, 0, 60);
    strumMs = std::clamp(strumMs, 60, 600);
    bpm = std::clamp(bpm, 40, 240);
    pattern = std::clamp(pattern, 0, 8);
    for (int n = 0; n < 4; n++) {
        if (combinations[n] < 0 || combinations[n] >= qualityCount)
            combinations[n] = int(qualityFor(combinationMasks[n]));
        for (auto &row : degreeCombinations)
            if (row[n] < -1 || row[n] >= qualityCount)
                row[n] = -1;
    }
}
std::array<int, 7> frontNotes(const Chord &chord, const Settings &s) {
    auto result = harpNotes(chord);
    const auto layout = result;
    const auto tones = chordNotes({chord.root, -1, chord.column, chord.quality});
    for (int n = 0; n < 7; n++) {
        int octave = s.frontMode ? s.scaleOctave[n] : s.frontOctave[n];
        if (s.frontMode) {
            static constexpr int scale[2][7]{{0, 2, 4, 5, 7, 9, 11}, {0, 2, 3, 5, 7, 8, 10}};
            result[n] = 60 + s.tonic + scale[s.scale != 0][s.frontDegree[n]];
        } else {
            if (s.frontSource[n] >= 0 && s.frontSource[n] < 4)
                result[n] = tones[s.frontSource[n]] + 12;
            if (s.frontSource[n] == 4)
                result[n] = chord.root + 12 + s.frontOffset[n];
            if (s.frontSource[n] >= 0 && octave == -2)
                octave = layout[n] / 12 - 1;
        }
        if (octave != -2)
            result[n] = (result[n] % 12 + 12) % 12 + 12 * (octave + 1);
        result[n] = std::clamp(result[n], 0, 127);
    }
    return result;
}
} // namespace zc
