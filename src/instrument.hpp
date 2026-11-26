#pragma once
#include "music.hpp"
#include "presets.hpp"
#include <cstdint>
#include <vector>
namespace zc {
enum class Part { Chord, Harp, Pad, Arp };
enum class CommandType { On, Off, Panic, Configure, StopChord, ChordGain };
struct Command {
    CommandType type = CommandType::Panic;
    uint32_t id = 0;
    int pitch = 0, durationMs = 0;
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
    std::array<int, 7> playingFrontNotes() const;
    bool frontHeld(int key) const { return frontVoices_[key] != 0; } // Z-M sounding while held.
    void key(char key, bool down, uint64_t now);
    void tick(uint64_t now);
    void panic();
    void configure();
    void setBackingMode(PlaybackMode mode);
    void setBacking(PlaybackMode mode, const std::array<int, 3> &mix); // Live, keeps the chord.
    void clearKeys();
    void auditionFront(const Settings &settings, int key);
    void audition(const Chord &chord, const Settings &settings, bool loop = false);
    void stopAudition(); // End a running preview, if any, keeping live playing intact.
    bool auditioning() const { return previewUntil_ || frontAudition_; }

  private:
    Sink &sink_;
    bool pressed_[128]{};
    unsigned selectedMask_ = 1;
    bool selectedSharp_ = false;
    bool chordPending_ = false, chordStarting_ = false;
    uint64_t chordDue_ = 0;
    int baseColumn_ = -1;
    std::array<int, 2> playingPatterns_{};
    uint32_t nextId_ = 0;
    uint64_t lastTick_ = 0, now_ = 0;
    double phase_ = 0;
    int64_t nextTick_ = 0;
    std::array<int64_t, 2> patternStarts_{};
    float lastGain_ = -1;
    uint64_t lastGainAt_ = 0;
    Settings previewSettings_;
    Chord previewChord_;
    uint64_t previewUntil_ = 0;
    bool previewLoop_ = false, frontAudition_ = false;
    std::vector<uint32_t> chordVoices_;
    std::array<uint32_t, 7> frontVoices_{}; // Held Z-M notes, released on key up.
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
    void runTick(uint64_t now);
    void hit(unsigned mask, int duration, float accent = 1.f, Part part = Part::Chord, bool bass = false);
    void pluck(int pitch, int index, uint64_t now, bool held = false);
    unsigned mask(int column) const;
    bool anyChordKey() const;
};
} // namespace zc
