#include "patterns.hpp"
#include <algorithm>
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
          {8,4,6.8f,.9f,true}, {10,14,1.5f,.7f}, {14,14,1.6f,.85f}}},
        // Guitar strums go down on the beat and up on the offbeat eighths.
        {"pop_strum", "Pop strum", "Down, down-up, up-down-up: the classic strum", 16,
         {{0,15,2.6f,1}, {4,15,1.8f,.8f}, {6,15,1.8f,.6f},
          {10,15,1.8f,.65f}, {12,15,1.8f,.85f}, {14,15,1.6f,.6f}}},
        {"waltz_chords", "Chords on 2 and 3", "Upper chord on beats 2 and 3; the bass takes 1", 12,
         {{4,14,2.6f,.75f}, {8,14,2.6f,.7f}}},
        {"rock_eighths", "Rock eighths", "Tight, driving eighths accented on 1 and 3", 16,
         {{0,14,1.3f,1}, {2,14,1.1f,.7f}, {4,14,1.3f,.85f}, {6,14,1.1f,.7f},
          {8,14,1.3f,.95f}, {10,14,1.1f,.7f}, {12,14,1.3f,.85f}, {14,14,1.1f,.75f}}}};
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
         "Broken chord in 3/4: root, then upper notes",
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
          {14, 12, 1, .85f}}},
        {"sixteenths", "Sixteenths up", "Fast rising 16th notes", 16,
         [] {
             std::vector<PatternStep> steps;
             static constexpr int tones[]{1, 2, 4, 8};
             for (int t = 0; t < 16; ++t)
                 steps.push_back({t, tones[t % 4], .9f, t % 4 == 0 ? 1.f : t % 2 ? .6f : .75f});
             return steps;
         }()},
        {"two_octaves", "Two octaves", "Root, third and fifth climbing two octaves", 16,
         {{0, 1, 1.9f, 1},
          {2, 2, 1.9f, .8f},
          {4, 4, 1.9f, .85f},
          {6, 1, 1.9f, .9f, false, 12},
          {8, 2, 1.9f, .8f, false, 12},
          {10, 4, 1.9f, .85f, false, 12},
          {12, 1, 1.9f, .95f, false, 24},
          {14, 4, 1.9f, .75f, false, 12}}}};
    // Drum steps: `notes` is the General MIDI drum note; accent is the hit's velocity.
    // 16 steps are one 4/4 bar; odd steps are the swung 16ths.
    enum : int { Kick = 36, Stick = 37, Snare = 38, Clap = 39, Closed = 42, Pedal = 44,
                 Open = 46, Ride = 51, Shaker = 70 };
    struct Hits {
        int note;
        std::vector<std::pair<int, float>> at; // step, accent
    };
    auto groove = [](std::initializer_list<Hits> parts) {
        std::vector<PatternStep> steps;
        for (const auto &part : parts)
            for (auto [tick, accent] : part.at)
                steps.push_back({tick, part.note,
                                 part.note == Ride ? 4.f : part.note == Open ? 3.f : 2.f, accent});
        return steps;
    };
    // Hi-hats or ride on every 16th (or 8th), shaped from beat to offbeat to 16th.
    auto ride = [](int note, int ticks, int every, float beat, float off, float sixteenth,
                   std::initializer_list<int> skip = {}) {
        Hits hits{note, {}};
        for (int t = 0; t < ticks; t += every)
            if (std::find(skip.begin(), skip.end(), t) == skip.end())
                hits.at.push_back({t, t % 4 == 0 ? beat : t % 2 == 0 ? off : sixteenth});
        return hits;
    };
    static const std::vector<Pattern> drums = {
        {"basic", "Basic beat", "Kick on 1 and 3, snare on 2 and 4, eighth hi-hats", 16,
         groove({{Kick, {{0, 1}, {8, .9f}}}, {Snare, {{4, .9f}, {12, .9f}}},
                 ride(Closed, 16, 2, .8f, .55f, 0)})},
        {"pop", "Pop", "Pushed kick, snare and clap, 16th hi-hats, open hat on 4&", 16,
         groove({{Kick, {{0, 1}, {6, .65f}, {8, .9f}}},
                 {Snare, {{4, .85f}, {12, .9f}}},
                 {Clap, {{4, .5f}, {12, .55f}}},
                 ride(Closed, 16, 1, .7f, .5f, .3f, {14, 15}),
                 {Open, {{14, .5f}}}})},
        {"rock", "Rock", "Heavy kick and snare under a driving ride", 16,
         groove({{Kick, {{0, 1}, {3, .7f}, {8, 1}, {10, .85f}}},
                 {Snare, {{4, 1}, {12, 1}}},
                 ride(Ride, 16, 2, .85f, .65f, 0)})},
        {"rnb", "R&B groove", "Laid-back kick, ghost snares and 16th hi-hats; try swing", 16,
         groove({{Kick, {{0, 1}, {3, .55f}, {10, .85f}, {11, .45f}}},
                 {Snare, {{4, .85f}, {7, .3f}, {9, .25f}, {12, .9f}, {15, .3f}}},
                 ride(Closed, 16, 1, .6f, .45f, .3f, {14, 15}),
                 {Open, {{14, .45f}}}})},
        {"halftime", "Half-time", "Snare on 3 with ride eighths and pedal hi-hat", 16,
         groove({{Kick, {{0, 1}, {3, .5f}, {11, .7f}}},
                 {Snare, {{8, 1}}},
                 ride(Ride, 16, 2, .6f, .45f, 0),
                 {Pedal, {{4, .45f}, {12, .45f}}}})},
        {"four", "Four on the floor", "Kick every beat, claps, open hats between, shaker", 16,
         groove({{Kick, {{0, 1}, {4, 1}, {8, 1}, {12, 1}}},
                 {Clap, {{4, .8f}, {12, .8f}}},
                 {Open, {{2, .55f}, {6, .55f}, {10, .55f}, {14, .55f}}},
                 ride(Shaker, 16, 1, .35f, .3f, .25f)})},
        {"bossa", "Bossa nova", "Cross-stick clave over a soft two-bar groove", 32,
         groove({{Stick, {{0, .7f}, {6, .7f}, {12, .7f}, {20, .7f}, {26, .7f}}},
                 {Kick, {{0, .8f}, {6, .45f}, {8, .75f}, {14, .45f},
                         {16, .8f}, {22, .45f}, {24, .75f}, {30, .45f}}},
                 ride(Closed, 32, 2, .4f, .3f, 0)})},
        {"waltz", "Waltz 3/4", "Kick on 1, cross-stick and pedal hi-hat on 2 and 3", 12,
         groove({{Kick, {{0, .85f}}},
                 {Stick, {{4, .55f}, {8, .5f}}},
                 {Ride, {{0, .5f}, {4, .4f}, {8, .4f}}},
                 {Pedal, {{4, .35f}, {8, .35f}}}})},
        {"brushes", "Brushes", "Jazz ride with brush taps and slaps on 2 and 4", 16,
         groove({{Ride, {{0, .5f}, {4, .55f}, {7, .35f}, {8, .5f}, {12, .55f}, {15, .35f}}},
                 ride(Snare, 16, 2, .25f, .2f, 0),
                 {Clap, {{4, .55f}, {12, .55f}}}, // Brush slap in brush kits.
                 {Pedal, {{4, .4f}, {12, .4f}}},
                 {Kick, {{0, .3f}, {8, .25f}}}})},
        // Dream: soft bells and triangle scattered across two bars, no kick or snare.
        {"chimes", "Chimes", "Bell tree, ride bell, triangle and a soft shaker; no kick or snare", 32,
         {{0, 84, 10, .8f}, {0, 53, 8, .6f}, {6, 81, 6, .6f}, {11, 80, 3, .45f},
          {16, 53, 8, .55f}, {20, 81, 6, .6f}, {26, 84, 8, .65f}, {29, 80, 3, .4f},
          {4, 70, 2, .3f}, {12, 70, 2, .35f}, {20, 70, 2, .3f}, {28, 70, 2, .35f}}}};
    static_assert(drumPatternCount == 10);
    // Bass steps: `notes` picks chord tones (1 root or slash bass, 2 third, 4 fifth, 8 top),
    // played an octave or two below the chord.
    static const std::vector<Pattern> basses = {
        {"root", "Root notes", "Root on 1 and 3", 16, {{0, 1, 7, 1}, {8, 1, 7, .9f}}},
        {"held", "Held root", "One long root note per bar", 16, {{0, 1, 15.5f, .9f}}},
        {"root_fifth", "Root and fifth", "Root on 1, fifth on 3, pickup on 4&", 16,
         {{0, 1, 5.5f, 1}, {8, 4, 5.5f, .85f}, {14, 1, 1.6f, .65f}}},
        {"eighths", "Driving eighths", "Steady root eighth notes", 16,
         {{0, 1, 1.6f, 1},   {2, 1, 1.6f, .7f},  {4, 1, 1.6f, .85f}, {6, 1, 1.6f, .7f},
          {8, 1, 1.6f, .95f}, {10, 1, 1.6f, .7f}, {12, 1, 1.6f, .85f}, {14, 1, 1.6f, .7f}}},
        {"octaves", "Octave pulse", "Root jumping an octave on every eighth", 16,
         {{0, 1, 1.5f, 1},       {2, 1, 1.5f, .75f, false, 12}, {4, 1, 1.5f, .9f},
          {6, 1, 1.5f, .75f, false, 12}, {8, 1, 1.5f, .95f}, {10, 1, 1.5f, .75f, false, 12},
          {12, 1, 1.5f, .9f},     {14, 1, 1.5f, .75f, false, 12}}},
        {"offbeat", "Offbeat", "Root between the beats, as in house", 16,
         {{2, 1, 1.6f, .95f}, {6, 1, 1.6f, .9f}, {10, 1, 1.6f, .95f}, {14, 1, 1.6f, .9f}}},
        {"syncopated", "Syncopated", "Root, fifth and top with space between", 16,
         {{0, 1, 2.6f, 1},
          {3, 1, .9f, .55f},
          {7, 4, 1.4f, .75f},
          {10, 1, 2.4f, .9f},
          {13, 1, .9f, .5f, false, 12},
          {14, 8, 1.6f, .7f}}},
        {"bossa", "Bossa nova", "Root on 1, fifth on 2& and 3, root on 4&", 16,
         {{0, 1, 5.5f, 1}, {6, 4, 1.8f, .75f}, {8, 4, 5.5f, .9f}, {14, 1, 1.8f, .75f}}},
        {"waltz", "Waltz 3/4", "Root on the first bar's 1, fifth on the next", 24,
         {{0, 1, 3.5f, 1}, {12, 4, 3.5f, .9f}}}};
    static_assert(bassPatternCount == 9);
    static const std::vector<Pattern> silent{{"none", "None", "Foreground notes only", 16, {}}};
    if (mode == PlaybackMode::FrontOnly)
        return silent;
    if (mode == PlaybackMode::Drums)
        return drums;
    if (mode == PlaybackMode::Bass)
        return basses;
    return mode == PlaybackMode::Pad ? pads : (mode == PlaybackMode::Chord ? chords : arps);
}
int patternIndex(const Settings &s) {
    if (s.mode == PlaybackMode::FrontOnly)
        return 0;
    if (s.mode == PlaybackMode::Drums)
        return s.drumPattern;
    if (s.mode == PlaybackMode::Bass)
        return s.bassPattern;
    return s.mode == PlaybackMode::Pad
               ? s.padMotion
               : (s.mode == PlaybackMode::Chord ? s.chordPattern : s.pattern);
}
void setPatternIndex(Settings &s, int n) {
    if (s.mode == PlaybackMode::FrontOnly)
        return;
    if (s.mode == PlaybackMode::Drums) {
        s.drumPattern = n;
        return;
    }
    if (s.mode == PlaybackMode::Bass) {
        s.bassPattern = n;
        return;
    }
    if (s.mode == PlaybackMode::Pad)
        s.padMotion = n;
    else if (s.mode == PlaybackMode::Chord)
        s.chordPattern = n;
    else
        s.pattern = n;
}
int beatsPerBar(const Settings &s) {
    // 3/4 patterns repeat every 12 or 24 16ths; every 4/4 one is a multiple of 16.
    auto threeFour = [](PlaybackMode mode, int index) {
        const auto &list = patterns(mode);
        return list[size_t(std::clamp(index, 0, int(list.size()) - 1))].ticks % 16 != 0;
    };
    return threeFour(PlaybackMode::Chord, s.chordPattern) || threeFour(PlaybackMode::Arp, s.pattern) ||
                   threeFour(PlaybackMode::Bass, s.bassPattern) ||
                   threeFour(PlaybackMode::Drums, s.drumPattern)
               ? 3
               : 4;
}
} // namespace zc
