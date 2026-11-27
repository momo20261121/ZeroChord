#include "instrument.hpp"
#include "midi.hpp"
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
    size_t count(CommandType type) const {
        return size_t(std::count_if(events.begin(), events.end(),
                                    [&](const Command &c) { return c.type == type; }));
    }
};
std::vector<MidiEvent> parse(MidiParser &parser, std::vector<uint8_t> bytes) {
    std::vector<MidiEvent> out;
    parser.feed(bytes.data(), bytes.size(), [&](const MidiEvent &e) { out.push_back(e); });
    return out;
}
MidiEvent on(int note, int velocity = 100) { return {MidiEvent::NoteOn, 0, note, velocity}; }
MidiEvent off(int note) { return {MidiEvent::NoteOff, 0, note, 0}; }
MidiEvent control(int cc, int value) { return {MidiEvent::Control, 0, cc, value}; }
int main() {
    { // Running status, note on at velocity 0, any channel, and split reads.
        MidiParser parser;
        auto events = parse(parser, {0x93, 60, 100, 64, 90, 60, 0});
        CHECK(events.size() == 3 && events[0].type == MidiEvent::NoteOn && events[0].channel == 3 &&
              events[0].data1 == 60 && events[0].data2 == 100);
        CHECK(events[1].type == MidiEvent::NoteOn && events[1].data1 == 64);
        CHECK(events[2].type == MidiEvent::NoteOff && events[2].data1 == 60);
        CHECK(parse(parser, {0x80, 64}).empty()); // A message split across reads.
        events = parse(parser, {10});
        CHECK(events.size() == 1 && events[0].type == MidiEvent::NoteOff && events[0].data1 == 64);
    }
    { // Real-time bytes inside a message; SysEx and system common data are skipped.
        MidiParser parser;
        auto events = parse(parser, {0x90, 0xf8, 60, 0xfe, 100, 0xf0, 0x7e, 0x10, 0x60, 0xf7,
                                     0x90, 62, 80, 0xf2, 0x10, 0x20, 0x40, 0x41});
        CHECK(events.size() == 2 && events[0].data1 == 60 && events[0].data2 == 100);
        CHECK(events[1].data1 == 62 && events[1].data2 == 80); // Then no running status.
        // Program change has one data byte; controllers come through with their value.
        events = parse(parser, {0xc0, 5, 6, 0xb1, 64, 127, 64, 0});
        CHECK(events.size() == 2 && events[0].type == MidiEvent::Control &&
              events[0].data1 == 64 && events[0].data2 == 127 && events[1].data2 == 0);
        CHECK(parse(parser, {60, 100}).size() == 1); // Running status for the controller.
    }
    { // Velocity curves: Normal is linear, Soft lifts light notes, Hard lowers them.
        CHECK(midiGain(127, VelocityNormal) == 1.f && midiGain(127, VelocityHard) == 1.f);
        CHECK(midiGain(32, VelocitySoft) > midiGain(32, VelocityNormal) &&
              midiGain(32, VelocityNormal) > midiGain(32, VelocityHard));
        CHECK(midiGain(1, VelocityFixed) == 1.f);
    }
    for (int target = 0; target < midiTargetCount; target++) {
        // Notes play the target layer's sound at their own pitch, whatever the key or octave.
        static constexpr Part parts[]{Part::Harp, Part::Chord, Part::Arp,
                                      Part::Pad,  Part::Bass,  Part::Drums};
        Capture sink;
        Instrument p(sink);
        p.settings.midiTarget = target;
        p.settings.tonic = 5;
        p.frontOctaveShift = {1, 1};
        p.midi(on(61, 64), 10);
        CHECK(sink.events.size() == 1);
        const auto note = sink.events[0];
        CHECK(note.type == CommandType::On && note.part == parts[target] && note.pitch == 61 &&
              note.live && note.delayUs == 0);
        CHECK(std::abs(note.gain - midiGain(64, VelocityNormal)) < 1e-6f);
        CHECK(p.midiLastNote == 61 && p.midiLastVelocity == 64 && p.midiLit());
        p.midi(off(61), 20);
        if (target == MidiDrums) { // Hits ring out like the drum layer's; release does nothing.
            CHECK(note.durationMs > 0 && sink.events.size() == 1);
        } else {
            CHECK(note.durationMs == 0); // Held until its release.
            CHECK(sink.events.size() == 2 && sink.events[1].type == CommandType::Off &&
                  sink.events[1].id == note.id);
        }
        p.tick(500);
        CHECK(!p.midiLit());
    }
    { // Muting the target layer stops its pattern only; the keyboard still plays it.
        Capture sink;
        Instrument p(sink);
        p.settings.midiTarget = MidiBass;
        p.settings.bass = 0;
        p.midi(on(40), 0);
        CHECK(sink.events.size() == 1 && sink.events[0].part == Part::Bass);
    }
    { // The sustain pedal holds released notes until it lifts; held keys keep sounding.
        Capture sink;
        Instrument p(sink);
        p.midi(control(64, 127), 0);
        p.midi(on(60), 1);
        p.midi(on(64), 2);
        p.midi(off(60), 3);
        CHECK(sink.count(CommandType::Off) == 0 && p.midiLit());
        p.midi(on(60), 4); // The same key again restarts it.
        CHECK(sink.count(CommandType::Off) == 1 && sink.events.back().type == CommandType::On);
        const uint32_t restarted = sink.events.back().id, held = sink.events[1].id;
        p.midi(off(60), 5);
        p.midi(control(64, 0), 6);
        CHECK(sink.count(CommandType::Off) == 2 && sink.events.back().id == restarted);
        p.midi(off(64), 7); // Still held through the pedal: its own release ends it.
        CHECK(sink.count(CommandType::Off) == 3 && sink.events.back().id == held);
        p.midi(off(64), 8); // A repeated release does nothing.
        CHECK(sink.count(CommandType::Off) == 3);
    }
    { // A chord change or stop leaves MIDI notes alone; Space and All notes off end them.
        Capture sink;
        Instrument p(sink);
        p.settings.midiTarget = MidiChord;
        p.settings.combinationMs = 0;
        p.configure();
        p.midi(on(60), 0);
        p.key('8', true, 10);
        p.key('8', false, 11);
        p.key('5', true, 20);
        p.tick(30);
        const uint32_t id = std::find_if(sink.events.begin(), sink.events.end(), [](const Command &c) {
                                return c.live;
                            })->id;
        CHECK(std::none_of(sink.events.begin(), sink.events.end(), [&](const Command &c) {
            return c.type == CommandType::Off && c.id == id;
        }));
        sink.events.clear();
        p.midi(on(62), 40);
        p.midi(control(64, 127), 41);
        p.midi(off(62), 42);
        p.key(' ', true, 50);
        CHECK(sink.events.back().type == CommandType::Panic);
        sink.events.clear();
        p.midi(off(60), 60); // Already silenced: nothing left to release.
        p.midi(control(64, 0), 61);
        CHECK(sink.events.empty());
        p.midi(on(65), 70);
        p.midi(on(67), 71);
        p.midi(control(123, 0), 72);
        CHECK(sink.count(CommandType::Off) == 2);
        p.midi(on(69), 80);
        p.midiReleaseAll(); // Unplugged.
        CHECK(sink.count(CommandType::Off) == 3);
        p.tick(400);
        CHECK(!p.midiLit());
    }
    { // Beyond the note limit, a pedal-held note makes way first, else the oldest.
        Capture sink;
        Instrument p(sink);
        p.midi(control(64, 127), 0);
        p.midi(on(30), 1);
        p.midi(off(30), 2); // Pedal-held.
        for (int n = 1; n < int(Instrument::maxMidiNotes); n++) p.midi(on(40 + n), 3);
        CHECK(sink.count(CommandType::Off) == 0);
        const uint32_t pedalNote = sink.events[0].id, oldestHeld = sink.events[1].id;
        p.midi(on(90), 4);
        CHECK(sink.count(CommandType::Off) == 1 && sink.events[sink.events.size() - 2].id == pedalNote);
        p.midi(on(91), 5);
        CHECK(sink.count(CommandType::Off) == 2 && sink.events[sink.events.size() - 2].id == oldestHeld);
    }
    std::cout << "MIDI input tests passed\n";
}
