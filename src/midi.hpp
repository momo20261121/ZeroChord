#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
namespace zc {
// A channel message from a MIDI keyboard. Note on with velocity 0 arrives as NoteOff.
struct MidiEvent {
    enum Type { NoteOn, NoteOff, Control } type = NoteOn;
    int channel = 0, data1 = 0, data2 = 0; // Note and velocity, or controller and value.
};
// Turns a raw MIDI byte stream into note and controller events. Handles running status and
// skips SysEx, other system messages and real-time bytes such as clock and active sensing.
class MidiParser {
  public:
    void feed(const uint8_t *bytes, size_t count, const std::function<void(const MidiEvent &)> &out);
    void reset() { status_ = 0, count_ = 0, needed_ = 0; }

  private:
    uint8_t status_ = 0, data_[2]{};
    int count_ = 0, needed_ = 0;
};
// What MIDI plays (Settings::midiTarget) and how velocity sets its level (midiVelocity).
enum MidiTarget { MidiFront, MidiChord, MidiArp, MidiPad, MidiBass, MidiDrums };
constexpr int midiTargetCount = 6;
const char *midiTargetName(int target);
enum MidiVelocity { VelocitySoft, VelocityNormal, VelocityHard, VelocityFixed };
constexpr int midiVelocityCount = 4;
const char *midiVelocityName(int curve);
float midiGain(int velocity, int curve); // 0-1 level for a 1-127 velocity.
} // namespace zc
