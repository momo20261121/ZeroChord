#pragma once
#include "instrument.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace zc {
// Enter on the playing screen: Empty -> Armed (the first chord or note starts the take) ->
// Recording -> Holding (Enter plays) -> Counting (Settings::loopCount bars of clicks, if any)
// -> Playing -> Holding. A hold deletes, after a question.
enum class LoopState { Empty, Armed, Recording, Holding, Counting, Playing };
// Short chords and Snap steps (Settings::loopShort, loopSnap), in 16ths: Off, 1/16, 1/8, beat.
constexpr std::array<int, 4> loopGridTicks{0, 1, 2, 4};
constexpr std::array<int, 5> loopBarChoices{4, 8, 16, 32, 64};
// Loop mix rows: Front (every melody note), then the backing layers in backingLayerOrder.
constexpr int loopLayerCount = 6;
const char *loopLayerName(int layer);
enum class LoopNoteSource { ChordKey, ScaleKey, AllFront, Midi };
struct LoopChord {
    Chord chord;
    unsigned mask = 1; // Rows held in the chord's column: M, m and 7 as bits 0-2.
    bool sharp = false;
    int bassColumn = -1; // Degree of a slash bass, or -1.
};
struct LoopEvent {
    enum Kind { ChordOn, ChordOff, NoteOn, NoteOff, Style };
    Kind kind = ChordOn;
    double tick = 0; // 16ths from the loop's beat 1.
    // ChordOn: the chord. NoteOn: the chord sounding when the note was played, so Follow
    // current can play the note over the same chord in the current key.
    LoopChord chord;
    uint32_t note = 0; // Pairs a NoteOn with its NoteOff.
    int pitch = 0, durationMs = 0; // As played; held notes end at their NoteOff.
    Part part = Part::Harp;
    float gain = 1.f;
    LoopNoteSource source = LoopNoteSource::ChordKey;
    int index = 0, shift = 0; // Key index, and semitones above that key's note when played.
    int style = 0;            // Style: index into LoopTake::styles.
};
struct LoopTake {
    std::vector<Settings> styles;  // [0] when the take began, then each change during it.
    std::vector<LoopEvent> events; // As played, in time order. Clean-up never edits these.
    int bars = 0;
    uint32_t seed = 1; // Humanizing restarts from here every pass, so each pass is the same.
    // Loop mix: -1 plays the layer as the style has it; otherwise its own level and on/off.
    std::array<int, loopLayerCount> level{-1, -1, -1, -1, -1, -1}, on{-1, -1, -1, -1, -1, -1};
    double length() const { return bars * 16.; }
};
// Chord changes as they will play. Each chord decides on its own, from the raw take: a chord
// shorter than `shortTicks` that leads into another chord joins it (the next chord starts when
// the short one did), a gap that short between two chords closes, and a chord that short at
// the loop's end is dropped (the next pass opens with its own chord). Then each remaining
// change moves to the nearest multiple of `snapTicks` from beat 1, again independently, so
// errors never add up; when two land together the later wins, and one landing on the loop's
// end is dropped. Notes and style changes keep their times.
std::vector<LoopEvent> cleanLoop(const std::vector<LoopEvent> &events, double length,
                                 int shortTicks, int snapTicks);
void writeLoop(const LoopTake &take, std::ostream &out);
bool readLoop(LoopTake &take, std::istream &in); // False when it is not a usable loop.

class Looper {
  public:
    Looper(Instrument &live, Sink &sink);
    std::string path; // The loop is saved here after each change; empty keeps it in memory.
    void load();      // From `path`, if a loop was saved.
    LoopState state() const { return state_; }
    const LoopTake &take() const { return take_; }
    // Enter: a tap acts on release, but a take ends when Enter went down.
    void tap(uint64_t pressedAt, uint64_t now);
    void discard(); // Hold while Armed or Recording: drop the take in progress.
    void clear();   // Delete the loop.
    void stop();    // Space: playback stops and the loop is kept.
    void tick(uint64_t now);
    // Recording, reported by the live instrument.
    void chordChanged(bool active, const LoopChord &chord, uint64_t at);
    void noteOn(uint32_t voice, LoopEvent note, uint64_t at);
    void noteOff(uint32_t voice, uint64_t at);
    void allNotesOff(uint64_t at);
    void styleChanged(const Settings &settings, uint64_t at);
    // Playback.
    bool playing() const { return state_ == LoopState::Playing; }
    // 16ths, while recording or playing; negative during the count-in, reaching 0 on beat 1.
    double position() const { return position_; }
    bool counting() const { return state_ == LoopState::Counting; }
    int recordedBars() const { return take_.bars; }
    bool chordActive() const { return playing() && engine_.active; }
    const Chord &chord() const { return engine_.chord; }
    bool blink() const { return (now_ / 400) % 2 == 0; } // The Armed light.
    // Loop mix: the take's (Recorded) or current (Follow current) layers, then the loop's own.
    Settings baseSettings() const;
    bool layerOn(int layer) const;
    int layerLevel(int layer) const;
    bool layerEdited(int layer) const;
    void setLayer(int layer, bool on, int level);
    void restoreLayer(int layer);
    void refresh(); // Style, Short chords or Snap changed: replay from the same place.
    bool missingSounds() const; // A sound of the take's style is no longer installed.
    const Instrument &engine() const { return engine_; }

  private:
    struct SlotSink : Sink {
        Sink &out;
        explicit SlotSink(Sink &sink) : out(sink) {}
        void send(const Command &c) override;
    };
    Instrument &live_;
    SlotSink sink_;
    Instrument engine_;
    LoopState state_ = LoopState::Empty;
    LoopTake take_;
    std::vector<LoopEvent> events_; // Cleaned, for playback.
    size_t next_ = 0, carry_ = size_t(-1); // carry_: a beat-1 chord already ringing.
    double position_ = 0;
    uint64_t clock_ = 0, now_ = 0, nextSync_ = 0;
    int style_ = 0;
    std::map<uint32_t, uint32_t> recording_; // Live voice ID -> note number in the take.
    uint32_t nextNote_ = 0, nextVoice_ = 0;
    std::map<uint32_t, uint32_t> sounding_; // Note number -> playback voice ID.
    Settings heardStyle_;
    double tickAt(uint64_t at) const;
    void begin(uint64_t at, bool byChord);
    void finish(double end);
    void add(LoopEvent e);
    void startPlayback(uint64_t now);
    void countIn(uint64_t now);
    double nextClick_ = 0;
    void wrap();
    void apply(const LoopEvent &e);
    void seek(); // Sets the chord and next event for position_ after a re-clean.
    void sync(bool force);
    void releaseNotes();
    void save() const;
    Settings effective(int style) const;
    Chord follow(const LoopChord &chord) const;
    int pitchFor(const LoopEvent &note) const;
};
} // namespace zc
