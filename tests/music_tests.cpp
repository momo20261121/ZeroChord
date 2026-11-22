#include "instrument.hpp"
#include "patterns.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::cerr << __LINE__ << ": " #x "\n";                                                 \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
using namespace zc;
struct Capture : Sink {
    std::vector<Command> events;
    void send(const Command &c) override { events.push_back(c); }
};
int main() {
    for (int tonic = 0; tonic < 12; tonic++)
        for (int scale = 0; scale < 2; scale++) {
            CHECK(rootFor(tonic, scale, 6) == 48 + tonic);
            const int expected[2][7] = {{2, 4, 5, 7, 9, 11, 0}, {2, 3, 5, 7, 8, 10, 0}};
            for (int col = 0; col < 7; col++)
                CHECK(rootFor(tonic, scale, col) == 48 + (tonic + expected[scale][col]) % 12);
            for (int col = 0; col < 7; col++)
                for (unsigned mask = 1; mask < 8; mask++) {
                    Chord c{rootFor(tonic, scale, col), -1, col, qualityFor(mask)};
                    auto harp = harpNotes(c);
                    for (int i = 1; i < 7; i++)
                        CHECK(harp[i] > harp[i - 1]);
                }
        }
    {
        Settings s;
        CHECK(s.chordVolume == 80 && s.harpVolume == 100);
        for (int tonic = 0; tonic < 12; tonic++) {
            s.tonic = tonic;
            for (int col = 0; col < 7; col++) CHECK(s.chordRoot(col) == rootFor(tonic, 0, col));
            s.scale = 1;
            for (int col = 0; col < 7; col++) CHECK(s.chordRoot(col) == rootFor(tonic, 0, col));
        }
        s.chordOctave = -1; s.degreeChordRoot[6] = 0;
        for (int pitch : chordNotes({s.chordRoot(6), 7, 6, Quality::Major})) CHECK(pitch >= 0 && pitch <= 127);
        Capture notes; Instrument player(notes); player.settings = s; player.settings.combinationMs = 0;
        player.settings.degreeChordRoot[0] = 6; player.settings.degreeChordOctave[0] = 4;
        player.key('2', true, 100); CHECK(player.chord.root == 66);
        player.panic(); player.key('1', true, 200); player.key('2', true, 201);
        CHECK(player.chord.root == 67);
        auto front = player.settings; front.frontMode = 1;
        auto before = frontNotes(player.chord, front);
        front.degreeChordOctave[0] = 2; front.degreeChordRoot[0] = 1;
        CHECK(frontNotes({front.chordRoot(0), -1, 0, Quality::Major}, front) == before);
    }
    const Quality qualities[] = {Quality::Major,    Quality::Minor,  Quality::Diminished,
                                 Quality::Seventh,  Quality::Major7, Quality::Minor7,
                                 Quality::Augmented};
    for (unsigned mask = 1; mask < 8; mask++)
        CHECK(qualityFor(mask) == qualities[mask - 1]);
    Capture sink;
    Instrument i(sink);
    i.settings.combinationMs = 0; // Existing performance checks exercise immediate mode.
    i.key('8', true, 1);
    CHECK(chordName(i.chord) == "C");
    auto count = sink.events.size();
    i.key('8', true, 2);
    CHECK(sink.events.size() == count);
    i.key('j', true, 3);
    CHECK(chordName(i.chord) == "Cmaj7");
    i.key('u', true, 4);
    CHECK(chordName(i.chord) == "Caug");
    i.key('j', false, 5);
    i.key('u', false, 6);
    CHECK(chordName(i.chord) == "Caug");
    i.key('8', false, 7);
    CHECK(i.active);
    i.key('1', true, 8);
    i.key('q', true, 9);
    CHECK(chordName(i.chord) == "Ebm");
    i.key('1', false, 10);
    CHECK(chordName(i.chord) == "Ebm");
    i.panic();
    i.key('8', true, 20);
    i.key('5', true, 21);
    CHECK(chordName(i.chord) == "C/G");
    CHECK(chordNotes(i.chord)[0] < chordNotes(i.chord)[1]);
    i.panic();
    i.key('8', true, 30);
    i.key('8', false, 31);
    sink.events.clear();
    i.key('l', true, 40);
    i.tick(40);
    CHECK(sink.events.size() == 1);
    i.tick(74);
    CHECK(sink.events.size() == 1);
    i.tick(75);
    CHECK(sink.events.size() == 2);
    i.tick(250);
    std::vector<int> up;
    for (auto c : sink.events)
        if (c.type == CommandType::On)
            up.push_back(c.pitch);
    CHECK(up.size() == 7);
    CHECK(std::is_sorted(up.begin(), up.end()));
    sink.events.clear();
    i.key('k', true, 300);
    i.tick(300);
    i.tick(510);
    std::vector<int> down;
    for (auto c : sink.events)
        if (c.type == CommandType::On)
            down.push_back(c.pitch);
    CHECK(down.size() == 7);
    for (int n = 0; n < 7; n++)
        CHECK(down[n] == up[6 - n]);
    i.key('l', false, 520);
    i.key('l', true, 530);
    i.panic();
    sink.events.clear();
    i.tick(1000);
    CHECK(sink.events.empty());
    i.settings.hold = false;
    i.key('8', true, 1100);
    i.key('8', false, 1101);
    CHECK(!i.active);
    i.settings.hold = true;
    i.key('8', true, 1200);
    i.key('o', true, 1201);
    CHECK(i.settings.mode == PlaybackMode::Chord);
    i.key('o', false, 1201);
    i.key('o', true, 1201);
    CHECK(i.settings.mode == PlaybackMode::Arp);
    sink.events.clear();
    i.tick(1201);
    CHECK(sink.events.size() == 1);
    i.tick(1202);
    CHECK(sink.events.size() == 1);
    i.key(' ', true, 1300);
    CHECK(!i.active && i.settings.mode == PlaybackMode::Arp);
    i.key('1', true, 1400);
    i.panic();
    i.key('8', true, 1401);
    CHECK(chordName(i.chord) == "C");
    i.key('l', false, 1402);
    i.key('l', true, 1403);
    i.key('0', true, 1404);
    CHECK(i.settings.preset == 0);
    sink.events.clear();
    i.tick(1700);
    CHECK(sink.events.empty());

    // Chord strikes are simultaneous and finite; manual mode never repeats.
    Capture modes;
    Instrument player(modes);
    player.settings.combinationMs = 0;
    player.settings.bpm = 100;
    player.key('8', true, 0);
    player.key('8', false, 1);
    CHECK(modes.events.size() == 4);
    for (const auto &c : modes.events)
        CHECK(c.durationMs == 0 && c.part == Part::Chord);
    modes.events.clear();
    player.key('o', true, 10);
    player.key('o', false, 11);
    CHECK(player.settings.mode == PlaybackMode::Chord);
    CHECK(modes.events.front().type == CommandType::StopChord);
    CHECK(modes.events[1].type == CommandType::Configure &&
          modes.events[1].settings.mode == PlaybackMode::Chord); // Renderer switches timbre too.
    CHECK(std::count_if(modes.events.begin(), modes.events.end(), [](const auto &c) {
              return c.type == CommandType::On && c.durationMs == 400;
          }) == 4);
    modes.events.clear();
    player.tick(2000);
    CHECK(modes.events.empty());
    player.key('8', true, 2100);
    player.key('8', false, 2101);
    player.key('l', true, 2102);
    player.tick(2400);
    CHECK(std::count_if(modes.events.begin(), modes.events.end(), [](const auto &c) {
              return c.type == CommandType::On && c.part == Part::Chord;
          }) == 4);
    CHECK(std::count_if(modes.events.begin(), modes.events.end(), [](const auto &c) {
              return c.type == CommandType::On && c.part == Part::Harp;
          }) == 7);
    player.key('o', true, 2500);
    player.key('o', false, 2501);
    modes.events.clear();
    for (int at : {2500, 2799, 2800, 3100, 3400, 3700})
        player.tick(at);
    CHECK(modes.events.size() == 5);
    auto notes = chordNotes(player.chord);
    for (int n = 0; n < 5; n++)
        CHECK(modes.events[n].pitch == notes[n % 4] && modes.events[n].durationMs == 180);
    // Changing harmony between steps keeps the beat, including after a tempo edit.
    modes.events.clear();
    player.key('2', true, 3750);
    player.key('2', false, 3751);
    player.tick(3999);
    CHECK(modes.events.empty());
    player.tick(4000);
    CHECK(modes.events.size() == 1);
    player.settings.bpm = 200;
    modes.events.clear();
    player.tick(4149);
    CHECK(modes.events.empty());
    player.tick(4150);
    CHECK(modes.events.size() == 1);
    // Long UI stalls skip old beats rather than overflow the audio command queue.
    modes.events.clear();
    player.tick(1000000);
    CHECK(modes.events.size() <= 4);
    // Every rhythmic pattern produces bounded, articulated backing events.
    for (auto mode : {PlaybackMode::Chord, PlaybackMode::Arp})
        for (int pattern = 0; pattern < int(patterns(mode).size()); pattern++) {
            player.panic();
            player.settings.mode = mode;
            setPatternIndex(player.settings, pattern);
            player.configure();
            modes.events.clear();
            player.key('8', true, 1000001);
            player.key('8', false, 1000002);
            for (int ms = 0; ms < 2400; ms += 10)
                player.tick(1000001 + ms);
            CHECK(std::count_if(modes.events.begin(), modes.events.end(),
                                [](const auto &c) { return c.type == CommandType::On; }) > 0);
            for (const auto &c : modes.events)
                if (c.type == CommandType::On)
                    CHECK(c.durationMs > 0 && c.gain > 0 && c.gain <= 1);
        }
    player.panic();
    player.settings.mode = PlaybackMode::Pad;
    player.settings.padMotion = 3;
    player.configure();
    modes.events.clear();
    player.key('8', true, 2000000);
    player.key('8', false, 2000001);
    for (int ms = 0; ms < 1200; ms += 10)
        player.tick(2000000 + ms);
    CHECK(std::count_if(modes.events.begin(), modes.events.end(), [](const auto &c) {
              return c.type == CommandType::ChordGain && c.gain < .8f;
          }) > 10);
    // Front only selects/latches harmony without scheduling any backing notes.
    player.panic();
    player.settings.mode = PlaybackMode::Arp;
    player.configure();
    player.key('8', true, 2500000);
    player.key('8', false, 2500001);
    player.key('o', true, 2500002);
    player.key('o', false, 2500003);
    CHECK(player.settings.mode == PlaybackMode::FrontOnly && player.active);
    modes.events.clear();
    player.key('5', true, 2500010);
    player.key('5', false, 2500011);
    CHECK(player.active && chordName(player.chord) == "G");
    for (int ms = 0; ms < 2000; ms += 10)
        player.tick(2500010 + ms);
    CHECK(std::none_of(modes.events.begin(), modes.events.end(),
                       [](const Command &c) { return c.type == CommandType::On; }));
    player.key('z', true, 2502100);
    player.key('z', false, 2502101);
    player.key('l', true, 2502200);
    player.key('l', false, 2502201);
    for (int ms = 0; ms < 500; ms += 10)
        player.tick(2502200 + ms);
    CHECK(std::count_if(modes.events.begin(), modes.events.end(), [](const Command &c) {
              return c.type == CommandType::On && c.part == Part::Harp;
          }) == 8);
    CHECK(std::none_of(modes.events.begin(), modes.events.end(), [](const Command &c) {
        return c.type == CommandType::On && c.part == Part::Chord;
    }));
    player.key('o', true, 2503000);
    player.key('o', false, 2503001);
    CHECK(player.settings.mode == PlaybackMode::Pad);
    // Style changes preserve the app key, learned mappings, and held fingering.
    player.panic();
    player.library.entries[1].sound.tonic = 2;
    player.library.entries[1].sound.combinations[0] = int(Quality::Sus4);
    player.settings = player.library.entries[0].sound;
    player.settings.tonic = 4;
    player.settings.combinations[0] = int(Quality::No3);
    player.key('8', true, 3000000);
    player.key('u', true, 3000001);
    player.key('0', true, 3000002);
    CHECK(chordName(player.chord) == "Eno3" && player.settings.preset == 2);
    // Recognition waits from the first button, never extending the deadline.
    // Every order of each combination produces one completed backing attack.
    for (auto mode : {PlaybackMode::Pad, PlaybackMode::Chord, PlaybackMode::Arp})
        for (unsigned mask : combinationMasks) {
            std::string keys;
            const char rows[]{'8', 'u', 'j'};
            for (int n = 0; n < 3; n++)
                if (mask & (1u << n))
                    keys += rows[n];
            std::sort(keys.begin(), keys.end());
            do {
                Capture captured;
                Instrument grouped(captured);
                grouped.settings.mode = mode;
                for (int n = 0; n < int(keys.size()); n++)
                    grouped.key(keys[n], true, 100 + n * 14);
                grouped.tick(129);
                CHECK(captured.events.empty());
                grouped.tick(130);
                auto notes = chordNotes(grouped.chord);
                CHECK(grouped.chord.quality == qualityFor(mask));
                int n = 0;
                for (const auto &event : captured.events)
                    if (event.type == CommandType::On) {
                        CHECK(event.part == Part::Chord && event.pitch == notes[n++]);
                    }
                CHECK(n == (mode == PlaybackMode::Arp ? 1 : 4));
                // Releases retain the selected combination rather than retriggering partials.
                captured.events.clear();
                for (char key : keys)
                    grouped.key(key, false, 131);
                grouped.tick(132);
                CHECK(captured.events.empty());
            } while (std::next_permutation(keys.begin(), keys.end()));
        }
    Capture groupedEvents;
    Instrument grouped(groupedEvents);
    grouped.settings.mode = PlaybackMode::Chord;
    grouped.key('8', true, 100);
    grouped.key('u', true, 115);
    grouped.key('z', true, 116);
    CHECK(groupedEvents.events.size() == 1 && groupedEvents.events[0].part == Part::Harp);
    groupedEvents.events.clear();
    grouped.key('j', true, 128);
    grouped.tick(130);
    CHECK(groupedEvents.events.size() == 4 && chordName(grouped.chord) == "Caug");
    grouped.panic();
    groupedEvents.events.clear();
    grouped.key('8', true, 200);
    grouped.key('u', true, 231); // Outside the window: first attack commits before this event.
    CHECK(groupedEvents.events.size() == 4);
    grouped.tick(261);
    CHECK(groupedEvents.events.size() == 8);
    grouped.panic();
    grouped.key('8', true, 300);
    grouped.panic();
    groupedEvents.events.clear();
    grouped.tick(1000);
    CHECK(groupedEvents.events.empty());
    grouped.settings.hold = 0;
    grouped.key('8', true, 1100);
    grouped.key('8', false, 1105);
    CHECK(!grouped.active);
    groupedEvents.events.clear();
    grouped.tick(1200);
    CHECK(groupedEvents.events.empty());
    // Later harmony changes do not restart the arpeggio's beat.
    grouped.panic();
    grouped.settings.hold = 1;
    grouped.settings.mode = PlaybackMode::Arp;
    grouped.key('8', true, 2000);
    grouped.key('8', false, 2001);
    grouped.tick(2030);
    grouped.tick(2300);
    groupedEvents.events.clear();
    grouped.key('2', true, 2305);
    grouped.tick(2320);
    grouped.tick(2335);
    CHECK(groupedEvents.events.size() == 1 &&
          groupedEvents.events[0].pitch == chordNotes(grouped.chord)[1]);
    std::cout << "PASS: harmony, combinations, harp direction, hold, simultaneous chords, "
                 "patterns, phase and bounded scheduling\n";
}
