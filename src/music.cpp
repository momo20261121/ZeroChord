#include "music.hpp"
#include <algorithm>
#include <tuple>

namespace zc {
const std::array<Preset, 4> presets = {
    {{"Harp", 48, 46}, {"Dream", 89, 10}, {"Keys", 4, 24}, {"Organ", 16, 11}}};
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
        n[0] = bass;
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
const char *modeName(PlaybackMode mode) {
    static const char *names[] = {"Pad", "Chord", "Arp"};
    return names[int(mode)];
}
bool PerformanceSettings::operator==(const PerformanceSettings &b) const {
    return std::tie(mode, chordProgram, harpProgram, chordPattern, padMotion, chordVolume,
                    harpVolume, echo, strumMs, bpm, pattern) ==
           std::tie(b.mode, b.chordProgram, b.harpProgram, b.chordPattern, b.padMotion,
                    b.chordVolume, b.harpVolume, b.echo, b.strumMs, b.bpm, b.pattern);
}
void Settings::clamp() {
    tonic = std::clamp(tonic, 0, 11);
    scale = std::clamp(scale, 0, 1);
    preset = std::clamp(preset, 0, 127);
    mode = PlaybackMode(std::clamp(int(mode), 0, 2));
    chordProgram = std::clamp(chordProgram, -1, 127);
    harpProgram = std::clamp(harpProgram, -1, 127);
    chordPattern = std::clamp(chordPattern, 0, 4);
    padMotion = std::clamp(padMotion, 0, 3);
    hold = !!hold;
    presetLevel = std::clamp(presetLevel, 0, 100);
    masterVolume = std::clamp(masterVolume, 0, 100);
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
} // namespace zc
