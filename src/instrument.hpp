#pragma once
#include "music.hpp"
#include "presets.hpp"
#include <cstdint>
#include <vector>
namespace zc {
enum class Part { Chord, Harp };
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
    std::array<int, 7> playingFrontNotes() const;
    void key(char key, bool down, uint64_t now);
    void tick(uint64_t now);
    void panic();
    void configure();
    void clearKeys();
    void auditionFront(const Settings &settings, int key);
    void audition(const Chord &chord, const Settings &settings, bool loop = false);

  private:
    Sink &sink_;
    bool pressed_[128]{};
    unsigned selectedMask_ = 1;
    bool selectedSharp_ = false;
    bool chordPending_ = false, chordStarting_ = false;
    uint64_t chordDue_ = 0;
    int baseColumn_ = -1, playingPattern_ = 0;
    uint32_t nextId_ = 0;
    uint64_t lastTick_ = 0, now_ = 0;
    double phase_ = 0;
    int64_t nextTick_ = 0, patternStart_ = 0;
    float lastGain_ = -1;
    uint64_t lastGainAt_ = 0;
    Settings previewSettings_;
    Chord previewChord_;
    uint64_t previewUntil_ = 0;
    bool previewLoop_ = false;
    std::vector<uint32_t> chordVoices_;
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
    void hit(unsigned mask, int duration, float accent = 1.f);
    void pluck(int pitch, int index, uint64_t now);
    unsigned mask(int column) const;
    bool anyChordKey() const;
};
} // namespace zc
