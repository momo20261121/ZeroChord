#pragma once
#include "music.hpp"
#include <vector>
namespace zc {
constexpr int sustainedChordPattern = 5; // Append to preserve saved numeric pattern IDs.
struct PatternStep {
    int tick, notes;
    float gate, accent;
    bool bass = false; // Place this event an octave below the chord voicing.
    int transpose = 0; // Semitones added to these notes (Arp and Bass octave leaps).
};
struct Pattern {
    const char *id, *name, *description;
    int ticks;
    std::vector<PatternStep> steps;
};
const std::vector<Pattern> &patterns(PlaybackMode mode);
int patternIndex(const Settings &s);
void setPatternIndex(Settings &s, int index);
} // namespace zc
