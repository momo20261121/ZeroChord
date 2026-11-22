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
         "Waltz",
         "Bass, chord, chord in 3/4",
         12,
         {{0, 1, 2, 1}, {4, 14, 1.5f, .8f}, {8, 14, 1.5f, .85f}}},
        {"sustained", "Sustained", "Hold the chord; Hold keeps it after release", 16, {}}};
    static const std::vector<Pattern> arps = {
        {"up",
         "Up",
         "Ascending eighth notes",
         16,
         {{0, 1, 1.2f, 1},
          {2, 2, 1.2f, .85f},
          {4, 4, 1.2f, .9f},
          {6, 8, 1.2f, .85f},
          {8, 1, 1.2f, 1},
          {10, 2, 1.2f, .85f},
          {12, 4, 1.2f, .9f},
          {14, 8, 1.2f, .85f}}},
        {"down",
         "Down",
         "Descending eighth notes",
         16,
         {{0, 8, 1.2f, 1},
          {2, 4, 1.2f, .85f},
          {4, 2, 1.2f, .9f},
          {6, 1, 1.2f, .85f},
          {8, 8, 1.2f, 1},
          {10, 4, 1.2f, .85f},
          {12, 2, 1.2f, .9f},
          {14, 1, 1.2f, .85f}}},
        {"up_down",
         "Up / down",
         "Rise and fall through the chord",
         16,
         {{0, 1, 1.2f, 1},
          {2, 2, 1.2f, .85f},
          {4, 4, 1.2f, .9f},
          {6, 8, 1.2f, .85f},
          {8, 4, 1.2f, 1},
          {10, 2, 1.2f, .85f},
          {12, 1, 1.2f, .9f},
          {14, 2, 1.2f, .85f}}},
        {"alternating",
         "Alternating bass",
         "Root alternates with upper notes",
         16,
         {{0, 1, 1.2f, 1},
          {2, 4, 1.2f, .8f},
          {4, 1, 1.2f, .9f},
          {6, 2, 1.2f, .8f},
          {8, 1, 1.2f, 1},
          {10, 8, 1.2f, .8f},
          {12, 1, 1.2f, .9f},
          {14, 4, 1.2f, .8f}}},
        {"pop",
         "Pop",
         "Even broken chords with accents",
         16,
         {{0, 1, 1.3f, 1},
          {2, 4, 1.1f, .7f},
          {4, 2, 1.3f, .9f},
          {6, 8, 1.1f, .7f},
          {8, 1, 1.3f, 1},
          {10, 4, 1.1f, .7f},
          {12, 8, 1.3f, .95f},
          {14, 2, 1.1f, .7f}}},
        {"rnb",
         "R&B",
         "Syncopated notes and little rests",
         16,
         {{0, 1, 1.4f, 1},
          {3, 2, .8f, .65f},
          {6, 4, 1.3f, .85f},
          {9, 8, .8f, .7f},
          {11, 2, .8f, .65f},
          {14, 4, 1.2f, .9f}}},
        {"rock",
         "Rock",
         "Root and fifth with upper chord hits",
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
         "Waltz",
         "Bass and upper chord in 3/4",
         12,
         {{0, 1, 2, 1}, {4, 6, 1.5f, .8f}, {8, 12, 1.5f, .85f}}},
        {"bossa",
         "Bossa",
         "Alternating low notes, offbeat chords",
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
