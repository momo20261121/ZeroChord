#include "midi.hpp"
#include <algorithm>
#include <cmath>
namespace zc {
void MidiParser::feed(const uint8_t *bytes, size_t count,
                      const std::function<void(const MidiEvent &)> &out) {
    for (size_t n = 0; n < count; ++n) {
        const uint8_t byte = bytes[n];
        if (byte >= 0xf8) continue; // Real-time bytes may arrive anywhere, even mid-message.
        if (byte & 0x80) {
            count_ = 0;
            if (byte >= 0xf0) { // System messages cancel running status; their data is skipped.
                status_ = 0;
                needed_ = byte == 0xf1 || byte == 0xf3 ? 1 : byte == 0xf2 ? 2 : 0;
                if (byte == 0xf0) needed_ = -1; // SysEx: skip until any status byte.
                continue;
            }
            status_ = byte;
            needed_ = (byte & 0xf0) == 0xc0 || (byte & 0xf0) == 0xd0 ? 1 : 2;
            continue;
        }
        if (!status_) { // Data of a skipped system message, or of no message at all.
            if (needed_ > 0) --needed_;
            continue;
        }
        data_[count_++] = byte;
        if (count_ < needed_) continue;
        count_ = 0; // Running status: further data repeats the same message type.
        const int type = status_ & 0xf0, channel = status_ & 0x0f;
        if (type == 0x90 && data_[1] > 0) out({MidiEvent::NoteOn, channel, data_[0], data_[1]});
        else if (type == 0x80 || type == 0x90) out({MidiEvent::NoteOff, channel, data_[0], 0});
        else if (type == 0xb0) out({MidiEvent::Control, channel, data_[0], data_[1]});
        else if (type == 0xe0) out({MidiEvent::PitchBend, channel, data_[0] | data_[1] << 7, 0});
    }
}
const char *midiTargetName(int target) {
    static const char *names[]{"Front", "Chord", "Arp", "Pad", "Bass", "Drums"};
    return names[std::clamp(target, 0, midiTargetCount - 1)];
}
const char *midiVelocityName(int curve) {
    static const char *names[]{"Soft", "Normal", "Hard", "Fixed"};
    return names[std::clamp(curve, 0, midiVelocityCount - 1)];
}
float midiGain(int velocity, int curve) {
    if (curve == VelocityFixed) return 1.f;
    const float v = std::clamp(velocity, 1, 127) / 127.f;
    // Soft lifts light playing; Hard needs a firmer touch for the same level.
    const float exponent = curve == VelocitySoft ? .5f : curve == VelocityHard ? 1.6f : 1.f;
    return std::pow(v, exponent);
}
} // namespace zc
