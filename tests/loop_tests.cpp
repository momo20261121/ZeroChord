#include "looper.hpp"
#include "patterns.hpp"
#include <string>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
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
namespace {
LoopEvent chordAt(double tick, int column) {
    LoopEvent e;
    e.kind = LoopEvent::ChordOn;
    e.tick = tick;
    e.chord.chord = Chord{48 + column, -1, column, Quality::Major};
    return e;
}
LoopEvent offAt(double tick) {
    LoopEvent e;
    e.kind = LoopEvent::ChordOff;
    e.tick = tick;
    return e;
}
std::vector<std::pair<double, int>> changes(const std::vector<LoopEvent> &events) {
    std::vector<std::pair<double, int>> out; // Tick and column; -1 for silence.
    for (const auto &e : events)
        if (e.kind == LoopEvent::ChordOn) out.push_back({e.tick, e.chord.chord.column});
        else if (e.kind == LoopEvent::ChordOff) out.push_back({e.tick, -1});
    return out;
}
void tap(Instrument &i, char key, uint64_t at) {
    i.key(key, true, at);
    i.key(key, false, at + 1);
}
// Ticks the instrument (and so the loop) every few milliseconds, as the app does.
void run(Instrument &i, uint64_t &now, uint64_t until) {
    for (; now < until; now += 3) i.tick(now);
    i.tick(now = until);
}
} // namespace
int main() {
    using Changes = std::vector<std::pair<double, int>>;
    // Clean-up merges short chords on the raw times before snapping: a key pressed at beat
    // 0.2 and its partner at 0.3 become one chord from 0.2, which snaps to beat 1. Snapping
    // first would have kept a half-beat chord the player never meant.
    CHECK((changes(cleanLoop({chordAt(.8, 2), chordAt(1.2, 5), chordAt(8, 3)}, 16, 2, 2)) ==
           Changes{{0, 5}, {8, 3}}));
    // Each change snaps on its own to the grid from beat 1, so lateness never adds up.
    std::vector<LoopEvent> late;
    for (int beat = 0; beat < 128; ++beat) late.push_back(chordAt(beat * 4 + .9, beat % 7));
    const auto snapped = changes(cleanLoop(late, 512, 2, 2));
    CHECK(snapped.size() == 128);
    for (int beat = 0; beat < 128; ++beat) CHECK(snapped[size_t(beat)].first == beat * 4);
    // A short stab followed by silence is a stab, not a slip: it stays.
    CHECK((changes(cleanLoop({chordAt(0, 1), offAt(1), chordAt(8, 2)}, 16, 2, 0)) ==
           Changes{{0, 1}, {1, -1}, {8, 2}}));
    // A short gap between two chords closes; the same chord on both sides simply rings on.
    CHECK((changes(cleanLoop({chordAt(0, 1), offAt(7.5), chordAt(8, 2)}, 16, 2, 0)) ==
           Changes{{0, 1}, {8, 2}}));
    CHECK((changes(cleanLoop({chordAt(0, 1), offAt(7.5), chordAt(8, 1)}, 16, 2, 0)) ==
           Changes{{0, 1}}));
    // Two changes snapping together keep the later; one snapping onto the end is dropped.
    CHECK((changes(cleanLoop({chordAt(0, 1), chordAt(3.1, 2), chordAt(3.3, 3)}, 16, 0, 4)) ==
           Changes{{0, 1}, {4, 3}}));
    CHECK((changes(cleanLoop({chordAt(0, 1), chordAt(15.5, 2)}, 16, 0, 4)) == Changes{{0, 1}}));
    // A chord struck just before the end belongs to the next pass.
    CHECK((changes(cleanLoop({chordAt(0, 1), chordAt(8, 2), chordAt(15.5, 1)}, 16, 2, 0)) ==
           Changes{{0, 1}, {8, 2}}));
    // Off leaves the take exactly as played.
    CHECK((changes(cleanLoop({chordAt(.8, 2), chordAt(1.2, 5)}, 16, 0, 0)) ==
           Changes{{.8, 2}, {1.2, 5}}));

    {
        // Record: Enter arms, the first chord starts the take on beat 1, Enter ends it on the
        // nearest bar. At 120 BPM a 16th is 125 ms and a bar 2 s.
        Capture out;
        Instrument live(out);
        Looper loop(live, out);
        live.looper = &loop;
        live.settings.bpm = 120;
        live.settings.hold = 0;
        live.settings.combinationMs = 30;
        live.settings.loopShort = 2;
        live.settings.loopSnap = 2;
        live.settings.loopCount = 0; // Count-in is checked below.
        live.configure();
        uint64_t now = 0;
        loop.tap(0, 1);
        CHECK(loop.state() == LoopState::Armed);
        run(live, now, 1000);
        CHECK(loop.state() == LoopState::Armed); // Nothing played yet.
        // t, then g 100 ms later: past the combination window, so two chords are heard...
        live.key('t', true, now);
        run(live, now, 1100);
        live.key('g', true, now);
        run(live, now, 1900);
        live.key('t', false, now);
        live.key('g', false, now);
        CHECK(loop.state() == LoopState::Recording);
        const auto &raw = loop.take().events;
        CHECK(raw.size() == 3 && raw[0].kind == LoopEvent::ChordOn && raw[0].tick == 0 &&
              raw[1].kind == LoopEvent::ChordOn && std::abs(raw[1].tick - .8) < .05 &&
              raw[2].kind == LoopEvent::ChordOff);
        const auto seventh = raw[1].chord.chord;
        // ...then IV a bar later, held while a front note plays over it.
        run(live, now, 3000);
        live.key('5', true, now);
        run(live, now, 3200);
        tap(live, 'z', now);
        run(live, now, 4900);
        live.key('5', false, now);
        loop.tap(4950, 5000); // 31.6 16ths: the nearest bar is the second.
        CHECK(loop.state() == LoopState::Holding && loop.recordedBars() == 2);
        // ...and the cleaned loop plays the seventh from beat 1: the short chord joined it.
        const auto clean = changes(cleanLoop(raw, 32, 2, 2));
        // The release at 7.2 snaps to 8; the last one, at 31.2, lands on the loop's end.
        CHECK((clean == Changes{{0, 4}, {8, -1}, {16, 3}}));

        // Play: the loop's commands go to slot 1 and its chord shows while no live chord does.
        out.events.clear();
        now = 6000;
        loop.tap(now, now);
        CHECK(loop.state() == LoopState::Playing);
        run(live, now, 6010);
        CHECK(loop.chordActive() && loop.chord() == seventh && !live.active);
        CHECK(live.frontChordActive() && live.frontChord() == seventh);
        CHECK(live.playingFrontNotes() == frontNotes(seventh, live.settings));
        bool loopSlot = false, liveSlot = false;
        for (const auto &c : out.events)
            if (c.type == CommandType::On) (c.slot == 1 ? loopSlot : liveSlot) = true;
        CHECK(loopSlot && !liveSlot);
        // The recorded front note plays at its own time, a fifth of a bar into IV.
        out.events.clear();
        run(live, now, 6000 + 2190);
        CHECK(loop.chord().column == 3);
        bool note = false;
        for (const auto &c : out.events) note = note || (c.part == Part::Harp && c.slot == 1);
        CHECK(note);
        // A live chord takes the screen and front keys; the loop keeps playing underneath.
        live.key('2', true, now);
        run(live, now, now + 40);
        CHECK(live.active && live.frontChord() == live.chord && loop.chordActive());
        live.key('2', false, now);
        // The tempo sets the loop's speed: at 240 BPM two seconds are two bars, one full pass.
        run(live, now, 8500);
        const double before = loop.position();
        live.settings.bpm = 240;
        run(live, now, 10500);
        CHECK(std::abs(loop.position() - before) < .5);
        live.settings.bpm = 120;
        // Space stops the loop and keeps it.
        tap(live, ' ', now);
        CHECK(loop.state() == LoopState::Holding && !loop.chordActive());
        bool stopped = false;
        for (const auto &c : out.events) stopped = stopped || (c.type == CommandType::Panic && c.slot == 1);
        CHECK(stopped);

        // Recorded keeps the take's style; Follow current uses today's style and key.
        const int takePattern = live.settings.drumPattern;
        live.settings.drumPattern = (takePattern + 1) % drumPatternCount;
        live.settings.tonic = 2;
        loop.tap(now, now);
        run(live, now, now + 10);
        CHECK(loop.engine().settings.drumPattern == takePattern && loop.chord() == seventh);
        live.settings.loopStyle = 1;
        loop.refresh();
        run(live, now, now + 60);
        CHECK(loop.engine().settings.drumPattern == live.settings.drumPattern);
        CHECK(loop.chord().root == seventh.root + 2 && loop.chord().quality == seventh.quality);
        live.settings.loopStyle = 0;
        loop.refresh();
        // Loop mix overrides one layer for the loop only and restores to the take.
        const int drums = 5; // Front, then backingLayerOrder: Chord, Arp, Pad, Bass, Drums.
        CHECK(loopLayerName(drums) == std::string("Drums"));
        const bool drumsOn = loop.layerOn(drums);
        loop.setLayer(drums, !drumsOn, 40);
        run(live, now, now + 60);
        CHECK(loop.layerEdited(drums) && loop.engine().settings.enabled(PlaybackMode::Drums) != drumsOn &&
              loop.engine().settings.mix(PlaybackMode::Drums) == 40 &&
              live.settings.enabled(PlaybackMode::Drums) == drumsOn);
        loop.restoreLayer(drums);
        run(live, now, now + 60);
        CHECK(!loop.layerEdited(drums) && loop.engine().settings.enabled(PlaybackMode::Drums) == drumsOn);

        // The take survives a save and load exactly.
        std::stringstream file;
        writeLoop(loop.take(), file);
        LoopTake copy;
        CHECK(readLoop(copy, file));
        CHECK(copy.bars == 2 && copy.seed == loop.take().seed && copy.events.size() == raw.size() &&
              copy.styles.size() == loop.take().styles.size() &&
              copy.styles[0].drumPattern == takePattern);
        for (size_t n = 0; n < raw.size(); ++n)
            CHECK(copy.events[n].kind == raw[n].kind && copy.events[n].tick == raw[n].tick &&
                  copy.events[n].chord.chord == raw[n].chord.chord && copy.events[n].pitch == raw[n].pitch);
        std::stringstream broken("zerochord-loop 1\nbars 0\n");
        CHECK(!readLoop(copy, broken));

        // Count-in: a bar of clicks (slot 1, fixed level, beat 1 accented), then beat 1.
        live.settings.loopCount = 1;
        tap(live, ' ', now); // From Holding.
        out.events.clear();
        loop.tap(now, now);
        CHECK(loop.state() == LoopState::Counting && loop.position() < 0 && !loop.chordActive());
        const uint64_t counted = now;
        run(live, now, counted + 1990);
        CHECK(loop.state() == LoopState::Counting);
        std::vector<float> clicks;
        for (const auto &c : out.events)
            if (c.type == CommandType::On) {
                CHECK(c.click && c.slot == 1 && c.part == Part::Drums);
                clicks.push_back(c.gain);
            }
        CHECK(clicks.size() == 4 && clicks[0] > clicks[1] && clicks[1] == clicks[3]);
        run(live, now, counted + 2020);
        CHECK(loop.state() == LoopState::Playing && loop.chordActive() && loop.position() < 1);
        tap(live, ' ', now);
        loop.tap(now, now); // Enter during the count-in cancels it.
        loop.tap(now + 100, now + 100);
        CHECK(loop.state() == LoopState::Holding);
        live.settings.loopCount = 0;

        loop.clear();
        CHECK(loop.state() == LoopState::Empty && loop.take().events.empty());
    }
    {
        // Enter a little late (within the 1/8 window) ends on the bar just finished; later, or
        // early, makes a whole bar. 120 BPM: a 16th is 125 ms, so 1/8 is 250 ms.
        for (auto [stopAt, bars] : {std::pair{4200, 2}, std::pair{4300, 3}, std::pair{3800, 2},
                                    std::pair{2100, 1}}) {
            Capture out;
            Instrument live(out);
            Looper loop(live, out);
            live.looper = &loop;
            live.settings.bpm = 120;
            live.settings.combinationMs = 0;
            live.configure();
            uint64_t now = 0;
            loop.tap(0, 0);
            live.key('2', true, 0);
            run(live, now, uint64_t(stopAt));
            loop.tap(uint64_t(stopAt), uint64_t(stopAt) + 80);
            CHECK(loop.state() == LoopState::Holding && loop.recordedBars() == bars);
        }
    }
    {
        // The length limit acts as Enter; a held Enter drops a take in progress.
        Capture out;
        Instrument live(out);
        Looper loop(live, out);
        live.looper = &loop;
        live.settings.bpm = 120;
        live.settings.loopBars = 1;
        live.configure();
        uint64_t now = 0;
        loop.tap(0, 0);
        tap(live, 'z', 100); // Chord-mode Z-M are silent with no chord: nothing to record.
        CHECK(loop.state() == LoopState::Armed);
        live.key('2', true, 120);
        live.key('2', false, 300); // Hold keeps it ringing...
        run(live, now, 1000);
        CHECK(live.active);
        run(live, now, 2200);
        CHECK(loop.state() == LoopState::Holding && loop.recordedBars() == 1);
        CHECK(!live.active); // ...until the take ends, which stops it.
        loop.clear();
        loop.tap(now, now);
        live.key('3', false, now);
        live.key('3', true, now + 5);
        run(live, now, now + 100);
        CHECK(loop.state() == LoopState::Recording);
        loop.discard();
        CHECK(loop.state() == LoopState::Empty);
    }
    {
        // 3/4: a style is in 3/4 when any layer plays a 3/4 pattern, even a muted one.
        auto index = [](PlaybackMode mode, const char *id) {
            const auto &list = patterns(mode);
            for (size_t n = 0; n < list.size(); ++n)
                if (std::string(list[n].id) == id) return int(n);
            std::exit(1);
        };
        Settings pop;
        pop.chordPattern = index(PlaybackMode::Chord, "pop_strum");
        pop.pattern = index(PlaybackMode::Arp, "sixteenths");
        pop.bassPattern = index(PlaybackMode::Bass, "root");
        pop.drumPattern = index(PlaybackMode::Drums, "pop");
        CHECK(beatsPerBar(pop) == 4);
        for (auto [mode, id] : {std::pair{PlaybackMode::Chord, "waltz_chords"},
                                std::pair{PlaybackMode::Arp, "waltz"},
                                std::pair{PlaybackMode::Bass, "waltz"},
                                std::pair{PlaybackMode::Drums, "waltz"}}) {
            Settings s = pop;
            s.mode = mode;
            setPatternIndex(s, index(mode, id));
            s.setEnabledLayers(0); // Muted layers still set the meter.
            CHECK(beatsPerBar(s) == 3);
        }
        // A Waltz take: bars of 12 16ths (1.5 s at 120 BPM), ended on the nearest bar, counted
        // in with three clicks, and wrapping after its own length.
        Capture out;
        Instrument live(out);
        Looper loop(live, out);
        live.looper = &loop;
        live.settings.bpm = 120;
        live.settings.combinationMs = 0;
        live.settings.loopCount = 1;
        live.settings.drumPattern = index(PlaybackMode::Drums, "waltz");
        live.configure();
        uint64_t now = 0;
        loop.tap(0, 0);
        live.key('2', true, 0);
        // A held bend and a mod wheel move during the take are recorded with it.
        live.midi({MidiEvent::PitchBend, 0, 12288, 0}, 0);
        run(live, now, 700);
        live.midi(MidiEvent{MidiEvent::Control, 0, 1, 64}, now);
        run(live, now, 3100); // 100 ms after bar 3 begins: inside the 1/8 window.
        loop.tap(3100, 3180);
        CHECK(loop.state() == LoopState::Holding && loop.take().beats == 3 && loop.recordedBars() == 2);
        CHECK(loop.take().length() == 24);
        int bends = 0, wheels = 0;
        for (const auto &e : loop.take().events) {
            if (e.kind == LoopEvent::Bend) {
                bends++;
                CHECK(e.tick == 0 && e.pitch == 4096);
            }
            if (e.kind == LoopEvent::Modulation) {
                wheels++;
                CHECK(e.pitch == 64 && e.tick > 4);
            }
        }
        CHECK(bends == 1 && wheels == 1);
        // It saves with its meter; a loop saved before 3/4 existed reads as 4/4.
        std::stringstream file;
        writeLoop(loop.take(), file);
        LoopTake copy;
        CHECK(readLoop(copy, file) && copy.beats == 3 && copy.length() == 24);
        std::string old = file.str();
        old.erase(old.find("beats 3\n"), 8);
        std::stringstream oldFile(old);
        CHECK(readLoop(copy, oldFile) && copy.beats == 4 && copy.length() == 32);
        // Count-in: one 3/4 bar is three clicks, beat 1 accented.
        live.midiResetControllers();
        out.events.clear();
        loop.tap(now, now);
        const uint64_t counted = now;
        run(live, now, counted + 1490);
        std::vector<float> clicks;
        for (const auto &c : out.events)
            if (c.type == CommandType::On && c.click) clicks.push_back(c.gain);
        CHECK(clicks.size() == 3 && clicks[0] > clicks[1] && clicks[1] == clicks[2]);
        CHECK(loop.state() == LoopState::Counting);
        // Playback sends the take's bend to the loop's slot from beat 1...
        out.events.clear();
        run(live, now, counted + 1520);
        CHECK(loop.state() == LoopState::Playing);
        auto sentBend = [&](int value) {
            for (const auto &c : out.events)
                if (c.type == CommandType::Bend && c.slot == 1 && c.pitch == value) return true;
            return false;
        };
        CHECK(sentBend(4096));
        // ...and wraps after two 3/4 bars (3 s), restarting from beat 1 with the bend again.
        run(live, now, counted + 1500 + 2900);
        CHECK(loop.position() > 22);
        out.events.clear();
        run(live, now, counted + 1500 + 3100);
        CHECK(loop.position() < 2 && sentBend(0) && sentBend(4096));
    }
    std::cout << "PASS: loop clean-up, recording, playback, style, mix, limit, saving and 3/4\n";
}
