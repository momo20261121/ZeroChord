#pragma once
#include "music.hpp"
#include <vector>
namespace zc {
struct PatternStep {
    int tick, notes;
    float gate, accent;
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
