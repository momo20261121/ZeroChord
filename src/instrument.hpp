#pragma once
#include "midi.hpp"
#include "music.hpp"
#include "presets.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace zc {
struct PatternStep;
class Looper;
enum class Part { Chord, Harp, Pad, Arp, Drums, Bass };
// Tempo carries the BPM in `pitch` (for the echo time); Route sets `pitch` 1 when the
// headphone jack is in use, 0 for the speaker.
enum class CommandType { On, Off, Panic, Configure, StopChord, ChordGain, Tempo, Route };
struct Command {
    CommandType type = CommandType::Panic;
    uint32_t id = 0;
    int pitch = 0, durationMs = 0;
    int delayUs = 0; // On: start this much later (strums and humanized timing).
    bool live = false; // On: a MIDI note; it ends on its own Off, not with the chord.
    // 0 live playing, 1 the loop: Panic, StopChord, ChordGain and Configure affect one slot.
    int slot = 0;
    bool click = false; // On (Drums): a count-in click at a fixed level, whatever the mix.
    Part part = Part::Harp;
    float gain = 1.f;
    Settings settings;
    Command(CommandType t = CommandType::Panic, uint32_t handle = 0, int note = 0, int duration = 0,
            Part p = Part::Harp)
        : type(t), id(handle), pitch(note), durationMs(duration), part(p) {}
};
struct Sink {
    virtual ~Sink() = default;
    virtual void send(const Command &) = 0;
};
class Instrument {
  public:
    explicit Instrument(Sink &sink);
    Settings settings;
    PresetLibrary library;
    Chord chord;
    bool active = false;
    int harpFlash = -1;
    uint64_t harpFlashUntil = 0;
    std::array<int, 2> frontOctaveShift{}; // Live playing register, not saved key mappings.
    int semitoneShift() const { return pressed_['1'] ? 1 : 0; }
    // The chord Z-M follow: the live chord, or the loop's while no live chord sounds.
    const Chord &frontChord() const;
    bool frontChordActive() const;
    std::array<int, 7> playingFrontNotes() const;
    bool frontHeld(int key) const { return frontVoices_[key] != 0; } // Z-M sounding while held.
    void key(char key, bool down, uint64_t now);
    // All front (hold Space): every letter and number key plays the front; the backing keeps
    // its chord. Not saved; Q/W shift the octave for this session.
    bool allFront = false;
    int allFrontShift = 0;
    void setAllFront(bool on);
    std::array<int, allFrontKeyCount> playingAllFrontNotes() const {
        return allFrontNotes(settings, allFrontShift,
                             settings.allFrontMode == 0 ? semitoneShift() : 0);
    }
    bool allFrontHeld(int index) const { return allFrontVoices_[size_t(index)] != 0; }
    void tick(uint64_t now);
    void panic();
    void configure();
    void setBackingMode(PlaybackMode mode);
    // Live layer and mix changes (layers as Settings::enabledLayers); keeps the chord.
    void setBacking(unsigned layers, const std::array<int, 5> &mix);
    void clearKeys();
    void auditionFront(const Settings &settings, int key);
    void audition(const Chord &chord, const Settings &settings, bool loop = false);
    void stopAudition(); // End a running preview, if any, keeping live playing intact.
    bool auditioning() const { return previewUntil_ || frontAudition_; }
    // Headphone jack: -1 unknown (Auto plays full range), 0 speaker, 1 headphones.
    int jack = -1;
    void setJack(int state);
    // Notes a backing part plays for the current chord: [0] root or slash bass, [1]-[3] upper
    // voices (Chord and Pad voicings), or the chord tones an octave up for Arp. -1 is silent.
    std::array<int, 4> partNotes(Part part);
    // MIDI keyboard: notes play Settings::midiTarget's sound, envelope and level at their own
    // pitch. Muting that layer stops only its pattern.
    std::string midiDevice; // Connected keyboard's name; empty when none.
    int midiLastNote = -1, midiLastVelocity = 0;
    void midi(const MidiEvent &event, uint64_t now);
    void midiReleaseAll(); // Release every MIDI note, as when the keyboard is unplugged.
    bool midiLit() const { return !midiNotes_.empty() || now_ < midiFlashUntil_; }
    static constexpr size_t maxMidiNotes = 16; // The oldest note makes way beyond this.

  private:
    Sink &sink_;
    bool pressed_[128]{};
    unsigned selectedMask_ = 1;
    bool selectedSharp_ = false;
    bool chordPending_ = false, chordStarting_ = false;
    uint64_t chordDue_ = 0;
    int baseColumn_ = -1;
    std::array<int, 4> playingPatterns_{}; // Chord, Arp, Drums, Bass.
    uint32_t nextId_ = 0;
    uint64_t lastTick_ = 0, now_ = 0;
    double phase_ = 0;
    int64_t nextTick_ = 0;
    std::array<int64_t, 4> patternStarts_{};
    float lastGain_ = -1;
    uint64_t lastGainAt_ = 0;
    Settings previewSettings_;
    Chord previewChord_;
    uint64_t previewUntil_ = 0;
    bool previewLoop_ = false, frontAudition_ = false;
    std::vector<uint32_t> chordVoices_;
    std::array<uint32_t, 7> frontVoices_{}; // Held Z-M notes, released on key up.
    std::array<uint32_t, allFrontKeyCount> allFrontVoices_{};
    void allFrontKey(char key, bool down);
    void releaseHeldNotes(); // Ends held front and All front notes.
    struct Pending {
        uint64_t at;
        int pitch, index;
    };
    std::vector<Pending> pending_;
    void select(int column, uint64_t now);
    void commitChord(uint64_t now);
    void soundChord();
    void stopChord();
    void resetTransport();
    void sendSettings(const Settings &s); // Configure, with the sounds' levels measured.
    void runTick(uint64_t now);
    void hit(unsigned mask, int duration, float accent = 1.f, Part part = Part::Chord, bool bass = false,
             int transpose = 0, int tick = -1);
    struct Voiced {
        Chord chord;
        int voicing = -1;
        std::array<int, 4> notes{-1, -1, -1, -1};
    };
    std::array<Voiced, 2> voiced_{}; // Chord, Pad: voice leading follows the last voicing.
    uint32_t random_ = 0x2545f491u;
    float random(); // 0-1, deterministic per session.
    int sentBpm_ = 0;
    bool strummed() const; // Chord sound is a plucked string: strum its hits.
    double swingOffset(int64_t tick) const; // Late shift of a 16th step, in ticks.
    void pluck(int pitch, int index, uint64_t now, bool held = false);
    void bassHit(const PatternStep &step, int duration);
    unsigned mask(int column) const;
    bool anyChordKey() const;
    struct MidiNote {
        int pitch;
        uint32_t id;
        bool down; // Key still held; released notes ring on while the pedal is down.
    };
    std::vector<MidiNote> midiNotes_; // Sounding MIDI notes, oldest first.
    bool sustain_ = false;
    uint64_t midiFlashUntil_ = 0;
    void endMidiNote(size_t index);

  public:
    // Loop (Enter): records what is played and plays it back through a second Instrument.
    Looper *looper = nullptr; // None on the loop's own engine.
    void stopEverything();    // Space: live sound and loop playback.
    // The loop's engine is driven through these rather than keys.
    void playChord(const Chord &c, uint64_t now); // A start from silence restarts the transport.
    void releaseChord();
    void restartTransport(uint64_t now);
    void reseed(uint32_t seed) { random_ = seed ? seed : 0x2545f491u; }
    void adopt(const Settings &s); // New settings, keeping the chord (layers start or stop).
    unsigned selectedMask() const { return selectedMask_; }
    bool selectedSharp() const { return selectedSharp_; }
    int bassColumn() const { return bassColumn_; }

  private:
    int bassColumn_ = -1; // Degree of a slash bass, or -1.
    // What the loop has heard: chord changes are reported once their combination settles,
    // timed from the first key of the combination.
    bool heardActive_ = false;
    Chord heardChord_;
    unsigned heardMask_ = 0;
    int64_t chordAt_ = -1;
    void reportChord(uint64_t now);
    void reportNote(uint32_t id, int pitch, Part part, int durationMs, float gain, int source,
                    int index, int shift);
    void reportOff(uint32_t id);
    void reportStyle();
    void keyPress(char key, bool down, uint64_t now);
};
} // namespace zc
