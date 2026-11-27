#include "instrument.hpp"
#include "patterns.hpp"
#include <algorithm>
#include <cmath>
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
    // 1 modifies new selections/plucks without retuning the current backing.
    for (int frontMode : {0, 1})
        for (auto mode : {PlaybackMode::Pad, PlaybackMode::Chord, PlaybackMode::Arp,
                          PlaybackMode::FrontOnly}) {
            Capture captured;
            Instrument p(captured);
            p.settings.combinationMs = 0;
            p.settings.frontMode = frontMode;
            p.settings.mode = mode;
            p.settings.degreeChordRoot[6] = 11; // B -> C must cross the octave.
            p.settings.frontSource[0] = 4;
            p.settings.frontOffset[0] = 0;
            p.settings.frontOctave[0] = 3;
            p.settings.scaleOctave[0] = 3;
            p.frontOctaveShift[frontMode] = -1;
            p.key('8', true, 10);
            p.key('5', true, 11); // Slash bass transposes with the chord.
            p.key('8', false, 12);
            p.key('5', false, 13);
            auto before = p.playingFrontNotes();
            auto harmony = p.chord;
            auto count = captured.events.size();
            p.key('1', true, 20);
            CHECK(p.semitoneShift() == 1);
            CHECK(p.chord == harmony && captured.events.size() == count);
            auto raised = p.playingFrontNotes();
            for (int n = 0; n < 7; n++) CHECK(raised[n] == before[n] + 1);
            p.key('1', true, 21); // Key repeat cannot transpose twice.
            CHECK(captured.events.size() == count && p.playingFrontNotes() == raised);
            captured.events.clear();
            p.key('z', true, 22);
            CHECK(p.frontHeld(0) && !p.frontHeld(1)); // The screen lights a key while it sounds.
            p.key('z', false, 23);
            CHECK(!p.frontHeld(0));
            // A front key sounds while held and its release starts the envelope's release.
            CHECK(captured.events.size() == 2 && captured.events[0].pitch == raised[0] &&
                  captured.events[0].durationMs == 0 && captured.events[1].type == CommandType::Off &&
                  captured.events[1].id == captured.events[0].id);
            p.key('1', false, 24);
            CHECK(p.semitoneShift() == 0 && p.chord == harmony);
            CHECK(p.playingFrontNotes() == before);
            // A fresh sharp selection stays sharp after 1 is released.
            p.key('1', true, 30);
            p.key('2', true, 31);
            CHECK(p.chord.root == p.settings.chordRoot(0) + 1);
            raised = p.playingFrontNotes();
            p.key('1', false, 32);
            CHECK(p.chord.root == p.settings.chordRoot(0) + 1);
            auto released = p.playingFrontNotes();
            for (int n = 0; n < 7; n++)
                CHECK(released[n] == raised[n] - 1);
            captured.events.clear();
            p.key('1', true, 33);
            CHECK(p.playingFrontNotes() == raised && captured.events.empty());
            p.panic();
            CHECK(p.semitoneShift() == 0);
            p.key('8', true, 34);
            CHECK(p.chord.root == p.settings.chordRoot(6));
        }
    {
        Capture captured;
        Instrument p(captured);
        p.settings.frontMode = 1; // Scale notes work before any chord is selected.
        auto before = p.playingFrontNotes();
        p.key('1', true, 1);
        CHECK(captured.events.empty() && !p.active);
        p.key('z', true, 2);
        CHECK(captured.events.back().pitch == before[0] + 1);
        p.key('1', false, 3);
        CHECK(p.playingFrontNotes() == before);
        p.panic();
        p.settings.frontMode = 0;
        p.settings.mode = PlaybackMode::Chord;
        p.key('8', true, 100);
        p.key('1', true, 114); // A pending selection retains the pitch chosen at keydown.
        CHECK(p.chord.root == 48);
        p.tick(130);
        int count = 0;
        for (const auto &c : captured.events)
            if (c.type == CommandType::On && c.part == Part::Chord)
                CHECK(c.pitch == voicedChord(p.chord, VoicingClose, {-1, -1, -1, -1})[count++]);
        CHECK(count == 4);
        p.key('1', false, 140);
        auto notes = p.playingFrontNotes();
        captured.events.clear();
        p.key('l', true, 200);
        p.tick(200);
        p.key('1', true, 201); // Already queued notes retain their original pitch.
        p.tick(410);
        count = 0;
        for (const auto &c : captured.events)
            if (c.type == CommandType::On && c.part == Part::Harp) {
                CHECK(c.pitch == notes[count]);
                ++count;
            }
        CHECK(count == 7);
    }
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
    i.key('o', true, 1201); // Advance input time; O itself is handled by the popup.
    i.setBackingMode(PlaybackMode((int(i.settings.mode) + 1) % 4));
    CHECK(i.settings.mode == PlaybackMode::Chord);
    i.key('o', false, 1201);
    i.key('o', true, 1201); // Advance input time; O itself is handled by the popup.
    i.setBackingMode(PlaybackMode((int(i.settings.mode) + 1) % 4));
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
    CHECK(i.settings.preset == 1);
    sink.events.clear();
    i.tick(1700);
    // Switching presets cancels the queued strum; backing layers may keep playing.
    CHECK(std::none_of(sink.events.begin(), sink.events.end(),
                       [](const Command &c) { return c.part == Part::Harp; }));

    // Chord strikes are simultaneous and finite; manual mode never repeats.
    Capture modes;
    Instrument player(modes);
    player.settings.combinationMs = 0;
    player.settings.bpm = 100;
    player.key('8', true, 0);
    player.key('8', false, 1);
    CHECK(modes.events.size() == 4);
    for (const auto &c : modes.events)
        CHECK(c.durationMs == 0 && c.part == Part::Pad);
    modes.events.clear();
    player.key('o', true, 10); // Advance input time; O itself is handled by the popup.
    player.setBackingMode(PlaybackMode((int(player.settings.mode) + 1) % 4));
    player.key('o', false, 11);
    CHECK(player.settings.mode == PlaybackMode::Chord);
    CHECK(std::count_if(modes.events.begin(), modes.events.end(), [](const Command &c) { return c.type == CommandType::StopChord; }) == 1);
    CHECK(std::any_of(modes.events.begin(), modes.events.end(), [](const Command &c) {
        return c.type == CommandType::Configure && c.settings.mode == PlaybackMode::Chord; })); // Renderer switches timbre too.
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
    player.key('o', true, 2500); // Advance input time; O itself is handled by the popup.
    player.setBackingMode(PlaybackMode((int(player.settings.mode) + 1) % 4));
    player.key('o', false, 2501);
    modes.events.clear();
    for (int at : {2500, 2799, 2800, 3100, 3400, 3700})
        player.tick(at);
    CHECK(modes.events.size() == 5);
    auto notes = chordNotes(player.chord);
    for (int n = 0; n < 5; n++)
        CHECK(modes.events[n].pitch == notes[n % 4] + 12 && modes.events[n].durationMs == 285); // Arp: an octave up.
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
    CHECK(modes.events.size() == 1 && modes.events[0].type == CommandType::Tempo &&
          modes.events[0].pitch == 200); // The echo follows the new tempo.
    modes.events.clear();
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
                    CHECK((c.durationMs > 0 || (mode == PlaybackMode::Chord &&
                                               pattern == sustainedChordPattern)) &&
                          c.gain > 0 && c.gain <= 1);
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
    player.key('o', true, 2500002); // Advance input time; O itself is handled by the popup.
    player.setBackingMode(PlaybackMode((int(player.settings.mode) + 1) % 4));
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
    player.key('o', true, 2503000); // Advance input time; O itself is handled by the popup.
    player.setBackingMode(PlaybackMode((int(player.settings.mode) + 1) % 4));
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
                // Pad and Chord play their voicings; Arp starts an octave above the chord.
                auto notes = mode == PlaybackMode::Arp ? chordNotes(grouped.chord)
                             : voicedChord(grouped.chord,
                                           mode == PlaybackMode::Pad ? VoicingOpen : VoicingClose,
                                           {-1, -1, -1, -1});
                if (mode == PlaybackMode::Arp)
                    for (auto &note : notes) note += 12;
                CHECK(grouped.chord.quality == qualityFor(mask));
                int n = 0;
                for (const auto &event : captured.events)
                    if (event.type == CommandType::On) {
                        CHECK(event.part == (mode == PlaybackMode::Pad ? Part::Pad : mode == PlaybackMode::Arp ? Part::Arp : Part::Chord) && event.pitch == notes[n++]);
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
    grouped.settings.combinationMs = Settings::maxCombinationMs;
    groupedEvents.events.clear();
    grouped.key('8', true, 1000);
    grouped.key('u', true, 1110);
    grouped.tick(1119);
    CHECK(groupedEvents.events.empty());
    grouped.tick(1120);
    CHECK(groupedEvents.events.size() == 4 && grouped.chord.quality == Quality::Diminished);
    grouped.panic();
    grouped.settings.combinationMs = Settings{}.combinationMs;
    grouped.key('8', true, 1200);
    grouped.panic();
    groupedEvents.events.clear();
    grouped.tick(1300);
    CHECK(groupedEvents.events.empty());
    grouped.settings.hold = 0;
    grouped.key('8', true, 1400);
    grouped.key('8', false, 1405);
    CHECK(!grouped.active);
    groupedEvents.events.clear();
    grouped.tick(1500);
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
          groupedEvents.events[0].pitch == chordNotes(grouped.chord)[1] + 12);
    // Sustained makes one simultaneous attack, no timed repeats, and respects Hold.
    for (bool hold : {false, true}) {
        Capture captured;
        Instrument sustained(captured);
        sustained.settings.mode = PlaybackMode::Chord;
        sustained.settings.chordPattern = sustainedChordPattern;
        sustained.settings.hold = hold;
        sustained.settings.combinationMs = 0;
        sustained.key('8', true, 1);
        CHECK(captured.events.size() == 4);
        for (const auto &c : captured.events) CHECK(c.durationMs == 0 && c.part == Part::Chord);
        captured.events.clear();
        sustained.tick(5000);
        CHECK(captured.events.empty());
        sustained.key('8', false, 5001);
        CHECK(sustained.active == hold);
        CHECK(captured.events.empty() == hold);
        if (hold) {
            sustained.key('2', true, 5002);
            CHECK(std::count_if(captured.events.begin(), captured.events.end(), [](const auto &c) {
                return c.type == CommandType::Off;
            }) == 4);
        }
        sustained.panic();
        CHECK(!sustained.active);
    }
    // Every checkbox combination shares harmony/tempo but keeps independent note routes.
    for (unsigned layers = 0; layers < 8; ++layers) {
        Capture captured; Instrument p(captured);
        p.settings.mode = modeForLayers(layers); p.settings.combinationMs = 0;
        p.settings.chordPattern = 0; p.settings.pattern = 0;
        CHECK(backingLayers(p.settings.mode) == layers);
        p.key('8', true, 100); p.key('8', false, 101); p.tick(101);
        int count[4]{};
        for (const auto &event : captured.events)
            if (event.type == CommandType::On) ++count[int(event.part)];
        CHECK(count[int(Part::Chord)] == ((layers & 1) ? 4 : 0));
        CHECK(count[int(Part::Pad)] == ((layers & 2) ? 4 : 0));
        CHECK(count[int(Part::Arp)] == ((layers & 4) ? 1 : 0));
        CHECK(p.active && chordName(p.chord) == "C");
        captured.events.clear();
        p.key('z', true, 102); p.key('z', false, 103);
        CHECK(captured.events.size() == 2 && captured.events[0].part == Part::Harp &&
              captured.events[1].type == CommandType::Off);
        p.setBackingMode(PlaybackMode::FrontOnly);
        captured.events.clear(); p.tick(5000);
        CHECK(captured.events.empty() && p.active);
    }
    // A 0% mix silences an unmuted layer. Mix changes apply live and keep the harmony.
    {
        Capture captured; Instrument p(captured);
        p.settings.mode = PlaybackMode::All; p.settings.combinationMs = 0;
        p.settings.chordPattern = 0;
        p.settings.mix(PlaybackMode::Pad) = 0;
        p.key('8', true, 100); p.tick(100);
        auto ons = [&](Part part) {
            return std::count_if(captured.events.begin(), captured.events.end(), [&](const Command &c) {
                return c.type == CommandType::On && c.part == part;
            });
        };
        CHECK(ons(Part::Pad) == 0 && ons(Part::Chord) == 4);
        const auto harmony = p.chord;
        captured.events.clear();
        auto mix = p.settings.layerMix;
        mix[layerIndex(PlaybackMode::Pad)] = 50;
        p.setBacking(p.settings.enabledLayers(), mix);
        CHECK(ons(Part::Pad) == 4 && p.active && p.chord == harmony);
        captured.events.clear();
        mix[layerIndex(PlaybackMode::Pad)] = 40;
        p.setBacking(p.settings.enabledLayers(), mix); // A level change keeps ringing notes.
        CHECK(captured.events.size() == 1 && captured.events[0].type == CommandType::Configure &&
              captured.events[0].settings.mix(PlaybackMode::Pad) == 40);
    }
    // The Waltz and Bossa bass notes sit below the harmony, rather than replacing its upper voice.
    for (int pattern : {4, 7}) {
        Capture captured; Instrument p(captured);
        p.settings.mode = PlaybackMode::Chord; p.settings.chordPattern = pattern;
        p.settings.combinationMs = 0; p.key('8', true, 100); p.tick(100);
        CHECK(captured.events.back().type == CommandType::On);
        CHECK(captured.events.back().pitch == p.chord.root - 12);
    }
    // Scales: seven-note scales set the chord roots; shorter ones continue up on the front keys.
    {
        Settings s;
        s.chordScale = 4; // Dorian
        CHECK(s.chordRoot(6) % 12 == 0 && s.chordRoot(1) % 12 == 3 && s.chordRoot(4) % 12 == 9);
        s.chordScale = 2; // Harmonic minor: raised seventh.
        CHECK(s.chordRoot(5) % 12 == 11);
        s.chordScale = 10; // Blues cannot set chord roots.
        s.clamp();
        CHECK(s.chordScale == chordScaleCount - 1);
        s.frontMode = 1;
        s.frontFollow = 0;
        s.frontTonic = 0;
        s.scale = 10; // Blues: C Eb F F# G Bb C'
        auto notes = frontNotes({48, -1, 6, Quality::Major}, s);
        CHECK((notes == std::array<int, 7>{60, 63, 65, 66, 67, 70, 72}));
        s.scale = 9; // Minor pentatonic: C Eb F G Bb C' Eb'
        notes = frontNotes({48, -1, 6, Quality::Major}, s);
        CHECK((notes == std::array<int, 7>{60, 63, 65, 67, 70, 72, 75}));
        CHECK(scaleMenu(true).size() == size_t(chordScaleCount) &&
              scaleMenu(false).size() == size_t(scaleCount));
        for (int scale : scaleMenu(true)) CHECK(scaleInfo(scale).size == 7);
    }
    // Drums follow the chord: they play on the beat clock while it sounds and stop on release.
    {
        Capture captured; Instrument p(captured);
        p.settings.combinationMs = 0;
        p.settings.hold = 0;
        p.settings.mode = PlaybackMode::FrontOnly;
        p.settings.mix(PlaybackMode::Drums) = 60;
        p.settings.drumPattern = 7; // Waltz 3/4: kick every 12 steps.
        p.settings.bpm = 120;       // 125 ms per step.
        auto drumHits = [&](int note) {
            return std::count_if(captured.events.begin(), captured.events.end(), [&](const Command &c) {
                return c.type == CommandType::On && c.part == Part::Drums && c.pitch == note;
            });
        };
        p.key('8', true, 0);
        for (int ms = 0; ms <= 3050; ms += 5) p.tick(ms);
        CHECK(drumHits(36) == 3 && drumHits(37) == 4); // Bars at 0, 1.5 and 3 s.
        CHECK(std::none_of(captured.events.begin(), captured.events.end(),
                           [](const Command &c) { return c.part == Part::Chord || c.part == Part::Arp; }));
        p.key('8', false, 3051);
        captured.events.clear();
        for (int ms = 3051; ms <= 5000; ms += 5) p.tick(ms);
        CHECK(drumHits(36) == 0);
        p.settings.drums = 0; // Muted drums stay silent while the chord plays.
        p.key('8', true, 6000);
        for (int ms = 6000; ms <= 8000; ms += 5) p.tick(ms);
        CHECK(drumHits(36) == 0);
        for (const auto &pattern : patterns(PlaybackMode::Drums))
            for (const auto &step : pattern.steps)
                CHECK(step.tick >= 0 && step.tick < pattern.ticks && step.notes >= 35 &&
                      step.notes <= 84); // Up to the bell tree.
        CHECK(int(patterns(PlaybackMode::Drums).size()) == drumPatternCount);
    }
    // Bass follows the chord's lowest note an octave below and stops with the chord.
    {
        Capture captured; Instrument p(captured);
        p.settings.combinationMs = 0;
        p.settings.hold = 0;
        p.settings.mode = PlaybackMode::FrontOnly;
        p.settings.mix(PlaybackMode::Bass) = 60;
        p.settings.bassPattern = 2; // Root and fifth.
        p.settings.bpm = 120;
        auto bassNotes = [&] {
            std::vector<int> notes;
            for (const auto &c : captured.events)
                if (c.type == CommandType::On && c.part == Part::Bass) notes.push_back(c.pitch);
            return notes;
        };
        p.key('8', true, 0); // C: root C3 in the chord, so the bass plays C2 and G2.
        for (int ms = 0; ms <= 1990; ms += 5) p.tick(ms);
        CHECK((bassNotes() == std::vector<int>{36, 43, 36}));
        p.key('8', false, 1991);
        captured.events.clear();
        for (int ms = 1991; ms <= 4000; ms += 5) p.tick(ms);
        CHECK(bassNotes().empty());
        p.key('8', true, 5000); p.key('5', true, 5000); // C/G: the slash bass leads.
        captured.events.clear();
        for (int ms = 5000; ms <= 5100; ms += 5) p.tick(ms);
        CHECK(!bassNotes().empty() && bassNotes()[0] % 12 == 7 && bassNotes()[0] >= 28 &&
              bassNotes()[0] <= 52);
        for (const auto &pattern : patterns(PlaybackMode::Bass))
            for (const auto &step : pattern.steps)
                CHECK(step.tick >= 0 && step.tick < pattern.ticks && step.notes > 0 && step.notes < 16);
        CHECK(int(patterns(PlaybackMode::Bass).size()) == bassPatternCount);
    }
    // Voicings: upper voices stay near middle C and move to the nearest chord notes; power
    // chords stack root, fifth and octave; rich voicings add 9ths without the root.
    {
        const std::array<int, 4> none{-1, -1, -1, -1};
        const Chord c{48, -1, 6, Quality::Major}, f{53, -1, 2, Quality::Major},
            g{55, -1, 3, Quality::Major};
        auto classes = [](const std::array<int, 4> &v) {
            std::vector<int> out;
            for (int n = 1; n < 4; n++) out.push_back(v[n] % 12);
            std::sort(out.begin(), out.end());
            return out;
        };
        auto first = voicedChord(c, VoicingClose, none);
        CHECK(first[0] % 12 == 0 && first[0] < first[1] && first[1] >= 52 && first[3] < 76);
        CHECK((classes(first) == std::vector<int>{0, 4, 7}));
        auto next = voicedChord(f, VoicingClose, first);
        int moved = 0;
        for (int n = 1; n < 4; n++) moved += std::abs(next[n] - first[n]);
        CHECK((classes(next) == std::vector<int>{0, 5, 9}) && moved <= 3); // C stays, E-F, G-A.
        auto back = voicedChord(g, VoicingClose, next);
        CHECK((classes(back) == std::vector<int>{2, 7, 11}) && back[1] >= 52 && back[3] < 76);
        auto power = voicedChord(g, VoicingPower, none);
        CHECK(power[2] - power[1] == 7 && power[3] - power[1] == 12 && power[1] - power[0] == 12);
        auto rich = voicedChord({57, -1, 4, Quality::Minor7}, VoicingRich, none); // Am9, no A.
        CHECK((classes(rich) == std::vector<int>{0, 7, 11}));
        auto open = voicedChord(c, VoicingOpen, none);
        CHECK((classes(open) == std::vector<int>{0, 4, 7}) && open[3] - open[1] > 12);
        auto slash = voicedChord({48, 43, 6, Quality::Major}, VoicingClose, none); // C/G.
        CHECK(slash[0] % 12 == 7 && slash[0] < slash[1]);
        // With a bass line playing, full chords leave the root to it.
        Capture captured; Instrument p(captured);
        p.settings.combinationMs = 0;
        p.settings.mode = PlaybackMode::Chord;
        p.settings.chordPattern = sustainedChordPattern;
        p.settings.bass = 1;
        p.settings.mix(PlaybackMode::Bass) = 80;
        p.key('8', true, 0);
        p.tick(1);
        std::vector<int> chord;
        for (const auto &e : captured.events)
            if (e.type == CommandType::On && e.part == Part::Chord) chord.push_back(e.pitch);
        CHECK(chord.size() == 3 && *std::min_element(chord.begin(), chord.end()) >= 52);
    }
    // Swing: 16ths delay every second 16th by up to a third of a 16th; 8ths delay the "and"
    // by up to two thirds, and its 16ths in proportion. 100% is the triplet; beats stay put.
    struct SwingCase {
        int mode, amount;
    };
    for (auto [mode, amount] : {SwingCase{SwingOff, 100}, {SwingSixteenths, 100},
                                {SwingSixteenths, 50}, {SwingEighths, 100}, {SwingEighths, 0}}) {
        Capture captured; Instrument p(captured);
        p.settings.combinationMs = 0;
        p.settings.mode = PlaybackMode::FrontOnly;
        p.settings.mix(PlaybackMode::Drums) = 60;
        p.settings.drumPattern = 5; // Four on the floor: shaker on every 16th.
        p.settings.bpm = 120;       // 125 ms per 16th.
        p.settings.swingMode = mode;
        p.settings.swingAmount = amount;
        p.key('8', true, 0);
        std::vector<int> shaker;
        for (int ms = 0; ms <= 600; ms++) {
            const auto before = captured.events.size();
            p.tick(ms);
            for (size_t n = before; n < captured.events.size(); n++)
                if (captured.events[n].part == Part::Drums && captured.events[n].pitch == 70)
                    shaker.push_back(ms);
        }
        const double a = amount / 100.;
        const double late[4]{0, mode == SwingSixteenths ? a / 3 : mode == SwingEighths ? a / 3 : 0,
                             mode == SwingEighths ? 2 * a / 3 : 0,
                             mode == SwingSixteenths ? a / 3 : mode == SwingEighths ? a / 3 : 0};
        CHECK(shaker.size() >= 4);
        for (int n = 0; n < 4; n++) CHECK(std::abs(shaker[n] - (n + late[n]) * 125) <= 2);
    }
    { // All front layouts: rows top to bottom 4-0, E-O, D-L, Z-M.
        CHECK(allFrontIndex('4') == 0 && allFrontIndex('e') == 7 && allFrontIndex('d') == 14 &&
              allFrontIndex('z') == 21 && allFrontIndex('m') == 27 && allFrontIndex('q') < 0 &&
              allFrontIndex('1') < 0 && allFrontIndex(' ') < 0);
        Settings s; // Scale mode, C major from octave 3: one octave per row, Z-M lowest.
        auto notes = allFrontNotes(s);
        CHECK(notes[21] == 48 && notes[27] == 59 && notes[14] == 60 && notes[7] == 72 &&
              notes[0] == 84 && notes[6] == 95);
        CHECK(allFrontNotes(s, 1, 1)[21] == 61); // Octave shift and the sharpen key.
        s.frontFollow = 0, s.frontTonic = 2, s.scale = 8; // Own key: D major pentatonic.
        notes = allFrontNotes(s);
        CHECK(notes[21] == 50 && notes[25] == 59 && notes[26] == 62 && notes[27] == 64);
        CHECK(notes[14] == 62); // Each row starts on the tonic, an octave up.
        s = Settings{};
        s.allFrontMode = 1; // Free play from F3: whites on E-O and Z-M, sharps above.
        notes = allFrontNotes(s);
        const std::array<int, 7> lowWhite{53, 55, 57, 59, 60, 62, 64}, lowSharp{54, 56, 58, -1, 61, 63, -1};
        for (int n = 0; n < 7; n++) {
            CHECK(notes[size_t(21 + n)] == lowWhite[size_t(n)]);
            CHECK(notes[size_t(14 + n)] == lowSharp[size_t(n)]);
            CHECK(notes[size_t(7 + n)] == lowWhite[size_t(n)] + 12);
            CHECK(notes[size_t(n)] == (lowSharp[size_t(n)] < 0 ? -1 : lowSharp[size_t(n)] + 12));
        }
        s.allFrontLowest = 48; // From C3 the gaps move to E and B.
        notes = allFrontNotes(s);
        CHECK(notes[14] == 49 && notes[15] == 51 && notes[16] == -1 && notes[17] == 54 &&
              notes[20] == -1 && notes[7] == 60 && notes[13] == 71);
        s.allFrontLowest = 54; // F#3 is not a white key: it starts from F3.
        s.clamp();
        CHECK(s.allFrontLowest == 53);
        s.allFrontMode = 0, s.allFrontOctave = 9;
        s.clamp();
        CHECK(s.allFrontOctave == 5 && allFrontNotes(s)[6] == 119);
    }
    { // Playing All front: every key plays the front; the backing keeps its chord.
        Capture captured;
        Instrument p(captured);
        p.settings.combinationMs = 0;
        p.settings.mode = PlaybackMode::Pad;
        p.configure();
        p.key('8', true, 10); // Hold is on: C stays after release.
        p.key('8', false, 20);
        CHECK(p.active);
        captured.events.clear();
        const auto latched = p.chord;
        p.setAllFront(true);
        CHECK(p.allFront && p.active && captured.events.empty());
        p.key('e', true, 30); // E plays C5 instead of selecting a chord.
        CHECK(p.chord == latched && captured.events.size() == 1 &&
              captured.events[0].part == Part::Harp && captured.events[0].pitch == 72 &&
              captured.events[0].durationMs == 0 && p.allFrontHeld(7));
        p.key('o', true, 31); // O and P are notes or ignored, not shortcuts.
        p.key('1', true, 32);
        p.key('z', true, 33); // 1 raises new notes in Scale mode.
        CHECK(captured.events.size() == 3 && captured.events[1].pitch == 83 &&
              captured.events[2].pitch == 49);
        p.key('1', false, 34);
        p.key('e', false, 35);
        CHECK(captured.events.back().type == CommandType::Off &&
              captured.events.back().id == captured.events[0].id && !p.allFrontHeld(7));
        p.key('w', true, 40); // Q/W shift the octave.
        p.key('w', false, 41);
        CHECK(p.playingAllFrontNotes()[21] == 60);
        p.settings.allFrontOctave = 5; // Top row already B8: one more octave would leave MIDI.
        p.allFrontShift = 0;
        p.key('w', true, 42);
        p.key('w', false, 43);
        CHECK(p.allFrontShift == 0);
        p.key('q', true, 44);
        CHECK(p.allFrontShift == -1);
        p.settings.allFrontOctave = 3, p.allFrontShift = 0;
        // Leaving releases held keys; the chord keeps sounding.
        const size_t offs = std::count_if(captured.events.begin(), captured.events.end(),
                                          [](const Command &c) { return c.type == CommandType::Off; });
        p.setAllFront(false);
        CHECK(!p.allFront && p.active &&
              size_t(std::count_if(captured.events.begin(), captured.events.end(), [](const Command &c) {
                  return c.type == CommandType::Off;
              })) == offs + 2); // O and Z were still held.
        p.settings.allFrontMode = 1; // Free play: 1 does not sharpen, gaps are silent.
        p.setAllFront(true);
        captured.events.clear();
        p.key('1', true, 50);
        p.key('z', true, 51);
        p.key('h', true, 52);
        CHECK(captured.events.size() == 1 && captured.events[0].pitch == 53);
        p.key(' ', true, 60); // Space stops everything, staying in All front.
        CHECK(captured.events.back().type == CommandType::Panic && !p.active && p.allFront &&
              !p.allFrontHeld(21));
    }
    { // With Hold off, chord keys held while switching count as released.
        Capture captured;
        Instrument p(captured);
        p.settings.combinationMs = 0;
        p.settings.hold = 0;
        p.configure();
        p.key('8', true, 10);
        CHECK(p.active);
        p.setAllFront(true);
        CHECK(!p.active && captured.events.back().type == CommandType::StopChord);
        p.key('8', false, 20); // Its release later is a note key's release: nothing sounds.
        CHECK(captured.events.back().type == CommandType::StopChord);
    }
    std::cout << "PASS: harmony, combinations, harp direction, hold, simultaneous chords, "
                 "patterns, phase, bounded scheduling and All front\n";
}
