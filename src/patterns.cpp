#include "patterns.hpp"
namespace zc {
const std::vector<Pattern> &patterns(PlaybackMode mode) {
    static const std::vector<Pattern> pads = {
        {"steady", "Steady", "Continuous sustained chord", 16, {}},
        {"swell", "Slow swell", "Gentle rise and fall over four bars", 64, {}},
        {"breathe", "Breathe", "Soft volume movement every two beats", 8, {}},
        {"pulse", "Pulse", "Beat-synced volume pulse", 4, {}}};
    static const std::vector<Pattern> chords = {
        {"manual", "Manual hit", "One simultaneous chord per press", 16, {}},
        {"quarter",
         "Every beat",
         "Four chord hits per 4/4 bar",
         16,
         {{0, 15, 1.5f, 1}, {4, 15, 1.5f, .8f}, {8, 15, 1.5f, .95f}, {12, 15, 1.5f, .8f}}},
        {"offbeat",
         "Offbeat",
         "Short chords between the beats",
         16,
         {{2, 15, 1, .85f}, {6, 15, 1, 1}, {10, 15, 1, .85f}, {14, 15, 1, 1}}},
        {"syncopated",
         "Syncopated",
         "Uneven chord hits with space",
         16,
         {{0, 15, 1.5f, 1},
          {3, 15, 1, .7f},
          {6, 15, 1.5f, .9f},
          {10, 15, 1, .75f},
          {14, 15, 1, .9f}}},
        {"waltz",
         "Bass/chord 3/4",
         "Low bass, upper chord, upper chord in 3/4",
         12,
         {{0, 1, 3.6f, 1, true}, {4, 14, 2.8f, .8f}, {8, 14, 2.8f, .85f}}},
        {"sustained", "Sustained", "Hold the chord; Hold keeps it after release", 16, {}},
        {"eighth", "Every eighth", "Eight clear chord hits per 4/4 bar", 16,
         {{0,15,1.5f,1}, {2,15,1.5f,.65f}, {4,15,1.5f,.85f}, {6,15,1.5f,.65f},
          {8,15,1.5f,1}, {10,15,1.5f,.65f}, {12,15,1.5f,.85f}, {14,15,1.5f,.65f}}},
        {"bossa", "Bossa accompaniment", "Low root/fifth beneath syncopated upper chords", 16,
         {{0,1,6.8f,1,true}, {3,14,1.5f,.7f}, {6,14,1.6f,.85f},
          {8,4,6.8f,.9f,true}, {10,14,1.5f,.7f}, {14,14,1.6f,.85f}}}};
    static const std::vector<Pattern> arps = {
        {"up",
         "Up",
         "Ascending eighth notes",
         16,
         {{0, 1, 1.9f, 1},
          {2, 2, 1.9f, .85f},
          {4, 4, 1.9f, .9f},
          {6, 8, 1.9f, .85f},
          {8, 1, 1.9f, 1},
          {10, 2, 1.9f, .85f},
          {12, 4, 1.9f, .9f},
          {14, 8, 1.9f, .85f}}},
        {"down",
         "Down",
         "Descending eighth notes",
         16,
         {{0, 8, 1.9f, 1},
          {2, 4, 1.9f, .85f},
          {4, 2, 1.9f, .9f},
          {6, 1, 1.9f, .85f},
          {8, 8, 1.9f, 1},
          {10, 4, 1.9f, .85f},
          {12, 2, 1.9f, .9f},
          {14, 1, 1.9f, .85f}}},
        {"up_down",
         "Up / down",
         "Rise and fall; repeat endpoints at each turnaround",
         16,
         {{0, 1, 1.9f, 1},
          {2, 2, 1.9f, .85f},
          {4, 4, 1.9f, .9f},
          {6, 8, 1.9f, .85f},
          {8, 8, 1.9f, 1},
          {10, 4, 1.9f, .85f},
          {12, 2, 1.9f, .9f},
          {14, 1, 1.9f, .85f}}},
        {"alternating",
         "Root and upper notes",
         "Root alternates with upper notes",
         16,
         {{0, 1, 1.9f, 1},
          {2, 4, 1.9f, .8f},
          {4, 1, 1.9f, .9f},
          {6, 2, 1.9f, .8f},
          {8, 1, 1.9f, 1},
          {10, 8, 1.9f, .8f},
          {12, 1, 1.9f, .9f},
          {14, 4, 1.9f, .8f}}},
        {"pop",
         "Broken eighths",
         "Even broken chords with accents",
         16,
         {{0, 1, 1.9f, 1},
          {2, 4, 1.9f, .7f},
          {4, 2, 1.9f, .9f},
          {6, 8, 1.9f, .7f},
          {8, 1, 1.9f, 1},
          {10, 4, 1.9f, .7f},
          {12, 8, 1.9f, .95f},
          {14, 2, 1.9f, .7f}}},
        {"rnb",
         "Syncopated notes",
         "Syncopated notes and little rests",
         16,
         {{0, 1, 1.4f, 1},
          {3, 2, .8f, .65f},
          {6, 4, 1.3f, .85f},
          {9, 8, .8f, .7f},
          {11, 2, .8f, .65f},
          {14, 4, 1.9f, .9f}}},
        {"rock",
         "Root/fifth and chord",
         "Legacy mixed note and chord pattern",
         16,
         {{0, 1, 1.4f, 1},
          {2, 4, 1.1f, .8f},
          {4, 14, 1.3f, .95f},
          {6, 4, 1.1f, .8f},
          {8, 1, 1.4f, 1},
          {10, 4, 1.1f, .8f},
          {12, 14, 1.3f, .95f},
          {14, 4, 1.1f, .8f}}},
        {"waltz",
         "Broken 3/4",
         "Legacy broken chord in 3/4",
         12,
         {{0, 1, 2, 1}, {4, 6, 1.5f, .8f}, {8, 12, 1.5f, .85f}}},
        {"bossa",
         "Bossa mixed",
         "Legacy alternating notes and upper chords",
         16,
         {{0, 1, 2, 1},
          {3, 6, 1, .7f},
          {6, 12, 1, .8f},
          {8, 4, 2, .9f},
          {10, 6, 1, .7f},
          {14, 12, 1, .85f}}}};
    static const std::vector<Pattern> silent{{"none", "None", "Foreground notes only", 16, {}}};
    if (mode == PlaybackMode::FrontOnly)
        return silent;
    return mode == PlaybackMode::Pad ? pads : (mode == PlaybackMode::Chord ? chords : arps);
}
int patternIndex(const Settings &s) {
    if (s.mode == PlaybackMode::FrontOnly)
        return 0;
    return s.mode == PlaybackMode::Pad
               ? s.padMotion
               : (s.mode == PlaybackMode::Chord ? s.chordPattern : s.pattern);
}
void setPatternIndex(Settings &s, int n) {
    if (s.mode == PlaybackMode::FrontOnly)
        return;
    if (s.mode == PlaybackMode::Pad)
        s.padMotion = n;
    else if (s.mode == PlaybackMode::Chord)
        s.chordPattern = n;
    else
        s.pattern = n;
}
} // namespace zc
