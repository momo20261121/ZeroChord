#pragma once
#include "midi.hpp"
#include <functional>
#include <memory>
#include <string>
namespace zc {
// USB MIDI keyboards through ALSA raw MIDI ports (/dev/snd/midiC*D*), read without blocking.
// ZEROCHORD_MIDI_DIR replaces /dev/snd, so a test can feed a FIFO named like a port.
class MidiInput {
  public:
    MidiInput();
    ~MidiInput();
    // Opens ports that appeared since the last scan; call about once a second.
    void scan();
    // Delivers pending events. Returns false when a port went away (keyboard unplugged).
    bool poll(const std::function<void(const MidiEvent &)> &out);
    std::string name() const; // Connected keyboards' name; empty when none.

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace zc
