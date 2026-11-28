#include "instrument.hpp"
#include "looper.hpp"
#include "patterns.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace zc {
Instrument::Instrument(Sink &sink) : sink_(sink) {}
unsigned Instrument::mask(int col) const {
    static const char *rows[] = {"2345678", "qwertyu", "asdfghj"};
    unsigned out = 0;
    for (int r = 0; r < 3; r++)
        if (pressed_[int(rows[r][col])])
            out |= 1u << r;
    return out;
}
bool Instrument::anyChordKey() const {
    for (int i = 0; i < 7; i++)
        if (mask(i))
            return true;
    return false;
}
void Instrument::stopChord() {
    for (auto id : chordVoices_)
        sink_.send(Command{CommandType::Off, id});
    chordVoices_.clear();
}
float Instrument::random() {
    random_ = random_ * 1664525u + 1013904223u;
    return float(random_ >> 8) / float(1u << 24);
}
bool Instrument::strummed() const {
    int program = settings.chordProgram;
    if (auto *sound = soundLibrary().find(settings.chordSound); sound && !sound->bank.empty())
        program = sound->program;
    // Steel-string and electric guitars, harp, and the plucked ethnic strings. Nylon (24)
    // stays fingerpicked, as in bossa: its notes sound together.
    return (program >= 25 && program <= 31) || program == 46 || (program >= 104 && program <= 107);
}
std::array<int, 4> Instrument::partNotes(Part part) {
    const bool bassLayer = settings.audible(PlaybackMode::Bass);
    if (part == Part::Arp) {
        // Arpeggios run an octave above the chord voicing. With a bass line, a slash bass
        // belongs to the bass, so the arp keeps the chord's own root.
        Chord own = chord;
        if (bassLayer) own.bass = -1;
        auto notes = chordNotes(own);
        for (auto &note : notes) note = std::clamp(note + 12, 0, 127);
        return notes;
    }
    auto &cache = voiced_[part == Part::Pad ? 1 : 0];
    const int voicing = part == Part::Pad ? settings.padVoicing : settings.chordVoicing;
    if (!(cache.chord == chord) || cache.voicing != voicing || cache.notes[1] < 0) {
        auto previous = cache.voicing == voicing ? cache.notes : std::array<int, 4>{-1, -1, -1, -1};
        cache.notes = voicedChord(chord, voicing, previous);
        cache.chord = chord;
        cache.voicing = voicing;
    }
    return cache.notes;
}
void Instrument::hit(unsigned mask, int duration, float accent, Part part, bool bass,
                     int transpose, int tick) {
    auto notes = partNotes(part);
    // With a bass line playing the root, full chords leave it out; a pattern's lone root
    // and power chords keep it.
    const int voicing = part == Part::Pad ? settings.padVoicing : settings.chordVoicing;
    if (part != Part::Arp && mask != 1 && voicing != VoicingPower &&
        settings.audible(PlaybackMode::Bass))
        notes[0] = -1;
    // Bass-register steps keep the root-position chord tone an octave down.
    const auto tones = chordNotes(chord);
    int count = 0;
    for (int n = 0; n < 4; n++)
        if ((mask & (1u << n)) && (bass || notes[n] >= 0)) ++count;
    // Plucked strings strum: down on the beat, up (quicker, softer) on the offbeats.
    const bool strum = part == Part::Chord && strummed() && count > 1;
    const bool up = strum && tick >= 0 && tick % 4 == 2;
    int order = 0;
    for (int step = 0; step < 4; step++) {
        const int n = up ? 3 - step : step;
        if (!(mask & (1u << n))) continue;
        const int note = bass ? tones[n] - 12 : notes[n];
        if (note < 0) continue;
        auto id = ++nextId_;
        if (!duration)
            chordVoices_.push_back(id);
        Command c{CommandType::On, id, std::clamp(note + transpose, 0, 127), duration, part};
        // A little human variation: notes land up to a few ms late, a few percent apart.
        const int late = int(random() * (part == Part::Pad ? 2000 : 4000));
        c.delayUs = late + (strum ? order * (up ? 6000 : 9000) : count > 1 ? int(random() * 2500) : 0);
        c.gain = std::clamp(accent * (up ? .85f : 1.f) * (.95f + .1f * random()), 0.f, 1.f);
        sink_.send(c);
        ++order;
    }
}
void Instrument::resetTransport() {
    phase_ = 0;
    nextTick_ = 0;
    if (looper && looper->playing()) { // Live patterns keep the loop's beat.
        phase_ = looper->position();
        nextTick_ = int64_t(std::ceil(phase_));
    }
    patternStarts_.fill(0);
    lastTick_ = now_;
    playingPatterns_ = {settings.chordPattern, settings.pattern, settings.drumPattern,
                        settings.bassPattern};
    lastGain_ = 1;
}
void Instrument::soundChord() {
    stopChord();
    if (!active)
        return;
    if (settings.audible(PlaybackMode::Pad))
        hit(15, 0, 1.f, Part::Pad);
    if (settings.audible(PlaybackMode::Chord)) {
        if (settings.chordPattern == 0) hit(15, 400);
        else if (settings.chordPattern == sustainedChordPattern) hit(15, 0);
    }
}
void Instrument::bassHit(const PatternStep &step, int duration) {
    // The chord's lowest note (its root, or a slash bass) sets the bass register.
    const auto tones = chordNotes(chord);
    int root = tones[0] - 12;
    int shift = -12;
    while (root > 52) root -= 12, shift -= 12;
    while (root < 28) root += 12, shift += 12;
    for (int n = 0; n < 4; n++)
        if (step.notes & (1 << n)) {
            Command c{CommandType::On, ++nextId_,
                      std::clamp(tones[n] + shift + step.transpose, 0, 127), duration, Part::Bass};
            c.gain = std::clamp(step.accent * (.95f + .1f * random()), 0.f, 1.f);
            c.delayUs = int(random() * 3000);
            sink_.send(c);
        }
}
void Instrument::select(int column, uint64_t now) {
    if (baseColumn_ < 0 || !mask(baseColumn_))
        baseColumn_ = column;
    Chord next{settings.chordRoot(baseColumn_) + (pressed_['1'] ? 1 : 0), -1,
               baseColumn_, settings.quality(mask(baseColumn_), baseColumn_)};
    bassColumn_ = -1;
    if (column != baseColumn_) {
        next.bass = settings.chordRoot(column) + (pressed_['1'] ? 1 : 0);
        bassColumn_ = column;
    } else if (active && chord.bass >= 0)
        for (int n = 0; n < 7; n++)
            if (n != baseColumn_ && mask(n)) {
                next.bass = settings.chordRoot(n) + (pressed_['1'] ? 1 : 0);
                bassColumn_ = n;
            }
    bool starting = !active;
    selectedMask_ = mask(baseColumn_);
    selectedSharp_ = pressed_['1'];
    chord = next;
    active = true;
    pending_.clear();
    if (!chordPending_) {
        chordStarting_ = starting;
        chordDue_ = now + uint64_t(settings.combinationMs);
    }
    chordPending_ = true;
    commitChord(now);
}
void Instrument::commitChord(uint64_t now) {
    if (!chordPending_ || now < chordDue_)
        return;
    chordPending_ = false;
    // The loop hears the chord from the first key of its combination.
    chordAt_ = std::max<int64_t>(0, int64_t(chordDue_) - settings.combinationMs);
    if (chordStarting_) {
        now_ = now;
        resetTransport();
    }
    soundChord();
}
void Instrument::pluck(int pitch, int index, uint64_t now, bool held) {
    const auto &envelope = settings.envelopes[0].values;
    // A held key sounds until release; strummed notes keep a fixed length.
    int duration = held ? 0 : std::max(1600, std::max(0, envelope[0]) + std::max(0, envelope[1]) + 300);
    sink_.send(Command{CommandType::On, ++nextId_, pitch, duration, Part::Harp});
    reportNote(nextId_, pitch, Part::Harp, duration, 1.f, settings.frontMode,
               index, pitch - frontNotes(frontChord(), settings)[size_t(index)]);
    if (held) frontVoices_[index] = nextId_;
    harpFlash = index;
    harpFlashUntil = now + 140;
}
void Instrument::key(char key, bool down, uint64_t now) {
    keyPress(key, down, now);
    reportChord(now);
}
void Instrument::keyPress(char key, bool down, uint64_t now) {
    now_ = now;
    auto k = static_cast<unsigned char>(key);
    if (k >= 128 || pressed_[k] == down)
        return;
    commitChord(now);
    pressed_[k] = down;
    if (key == '1')
        return;
    if (allFront) {
        allFrontKey(key, down);
        return;
    }
    int col = columnFor(key);
    auto harp = std::string("zxcvbnm").find(key);
    if (!down && harp != std::string::npos) {
        if (frontVoices_[harp]) {
            reportOff(frontVoices_[harp]);
            sink_.send(Command{CommandType::Off, frontVoices_[harp]});
        }
        frontVoices_[harp] = 0;
        return;
    }
    if (!down) {
        if (col >= 0 && !anyChordKey()) {
            baseColumn_ = -1;
            if (!settings.hold) {
                // Preserve short taps without letting a delayed chord sound after release.
                chordDue_ = now;
                commitChord(now);
                stopChord();
                sink_.send(Command{CommandType::StopChord});
                active = false;
                pending_.clear();
            }
        }
        return;
    }
    if (col >= 0) {
        select(col, now);
        return;
    }
    if (harp != std::string::npos && (frontChordActive() || settings.frontMode == 1))
        pluck(playingFrontNotes()[harp], int(harp), now, true);
    if (key == 'k' || key == 'l') {
        pending_.clear();
        auto notes = playingFrontNotes();
        if (settings.frontAction[settings.frontMode] == 1) {
            int step = key == 'l' ? 1 : -1;
            auto limits = std::minmax_element(notes.begin(), notes.end());
            if (*limits.first + step * 12 >= 0 && *limits.second + step * 12 <= 127)
                frontOctaveShift[settings.frontMode] += step;
        } else if (frontChordActive() || settings.frontMode == 1) {
            int time = settings.frontMode ? settings.scaleStrumMs : settings.strumMs;
            for (int i = 0; i < 7; i++) {
                int n = key == 'l' ? i : 6 - i;
                pending_.push_back({now + uint64_t(i * time / 6), notes[n], n});
            }
        }
    }
    if (key == 'i') {
        settings.hold = !settings.hold;
        if (!settings.hold && !anyChordKey()) {
            chordPending_ = false;
            stopChord();
            sink_.send(Command{CommandType::StopChord});
            active = false;
            pending_.clear();
        }
    }
    if (key == '9' || key == '0') {
        Settings previous = settings;
        const auto order = library.order();
        int position = int(std::find(order.begin(), order.end(), settings.preset) - order.begin());
        library.select(settings, order[(position + (key == '0' ? 1 : int(order.size()) - 1)) %
                                       int(order.size())]);
        if (active) {
            chord.root += settings.chordRoot(chord.column) -
                          previous.chordRoot(chord.column);
            // Preserve the selected fingering and app-wide mappings.
            chord.quality = settings.quality(selectedMask_, chord.column);
            if (chord.bass >= 0) {
                int oldBass = (chord.bass - (selectedSharp_ ? 1 : 0)) % 12;
                for (int col = 0; col < 7; col++)
                    if (previous.chordRoot(col) % 12 == oldBass) {
                        chord.bass =
                            settings.chordRoot(col) + (selectedSharp_ ? 1 : 0);
                        break;
                    }
            }
        }
        resetTransport();
        configure();
    }
    if (key == ' ')
        stopEverything();
}
double Instrument::swingOffset(int64_t tick) const {
    // Ticks are 16ths. 100% swing puts the delayed note on the triplet: a swung 16th moves a
    // third of a 16th; a swung eighth moves two thirds of a 16th, its 16ths in proportion.
    const double amount = settings.swingAmount / 100.;
    const int position = int(((tick % 4) + 4) % 4);
    if (settings.swingMode == SwingSixteenths) return position % 2 ? amount / 3. : 0.;
    if (settings.swingMode == SwingEighths) {
        const double shift = amount * 2. / 3.;
        return position == 2 ? shift : position % 2 ? shift / 2. : 0.;
    }
    return 0.;
}
void Instrument::sendSettings(const Settings &s) {
    soundLibrary().prepareGains(s);
    Command c{CommandType::Configure};
    c.settings = s;
    sink_.send(c);
}
void Instrument::setJack(int state) {
    if (state == jack) return;
    jack = state;
    Command c{CommandType::Route};
    c.pitch = state != 0; // Unknown plays full range, as on headphones.
    sink_.send(c);
}
void Instrument::runTick(uint64_t now) {
    if (sentBpm_ > 0 && settings.bpm != sentBpm_) { // The echo follows the tempo live.
        sentBpm_ = settings.bpm;
        Command c{CommandType::Tempo};
        c.pitch = settings.bpm;
        sink_.send(c);
    }
    for (auto it = pending_.begin(); it != pending_.end();) {
        if (it->at <= now) {
            pluck(it->pitch, it->index, now);
            it = pending_.erase(it);
        } else
            ++it;
    }
    if (active && settings.audibleLayers())
        phase_ += double(now - lastTick_) * settings.bpm / 15000.;
    if (active && !chordPending_ && settings.audibleLayers()) {
        if (nextTick_ + 32 < phase_)
            nextTick_ = int64_t(phase_); // Do not flood audio after a stall.
        while (nextTick_ + swingOffset(nextTick_) <= phase_) {
            for (int layer = 0; layer < 4; ++layer) {
                static constexpr PlaybackMode modes[]{PlaybackMode::Chord, PlaybackMode::Arp,
                                                      PlaybackMode::Drums, PlaybackMode::Bass};
                const auto mode = modes[layer];
                if (!settings.audible(mode)) continue;
                const auto &list = patterns(mode);
                int &playing = playingPatterns_[layer];
                playing = std::clamp(playing, 0, int(list.size()) - 1);
                int desired = layer == 0   ? settings.chordPattern
                              : layer == 1 ? settings.pattern
                              : layer == 2 ? settings.drumPattern
                                           : settings.bassPattern;
                auto &start = patternStarts_[layer];
                if ((nextTick_ - start) % list[playing].ticks == 0 && playing != desired) {
                    playing = desired;
                    start = nextTick_;
                }
                const auto &p = list[playing];
                int at = int((nextTick_ - start) % p.ticks);
                for (const auto &step : p.steps) {
                    if (step.tick != at) continue;
                    const int duration = int(step.gate * 15000 / settings.bpm);
                    if (mode == PlaybackMode::Bass) {
                        bassHit(step, std::max(40, duration));
                    } else if (mode == PlaybackMode::Drums) {
                        // One drum note per step, independent of the chord.
                        Command c{CommandType::On, ++nextId_, step.notes, std::max(60, duration),
                                  Part::Drums};
                        c.gain = std::clamp(step.accent * (.94f + .12f * random()), 0.f, 1.f);
                        c.delayUs = int(random() * 3000);
                        sink_.send(c);
                    } else
                        hit(unsigned(step.notes), std::max(20, duration), step.accent,
                            layer == 0 ? Part::Chord : Part::Arp, step.bass, step.transpose, at);
                }
            }
            ++nextTick_;
        }
        if (settings.audible(PlaybackMode::Pad)) {
            constexpr double pi = 3.141592653589793;
            float gain = 1;
            if (settings.padMotion == 1)
                gain = float(.7 + .3 * std::sin(2 * pi * phase_ / 64 - pi / 2));
            if (settings.padMotion == 2)
                gain = float(.65 + .35 * std::sin(2 * pi * phase_ / 8 - pi / 2));
            if (settings.padMotion == 3)
                gain = float(.3 + .7 * std::exp(-std::fmod(phase_, 4.) * 1.2));
            if (now >= lastGainAt_ + 20 && std::abs(gain - lastGain_) > .015f) {
                Command c{CommandType::ChordGain}; // Pad motion never modulates Chord or Arp.
                c.gain = gain;
                sink_.send(c);
                lastGain_ = gain;
                lastGainAt_ = now;
            }
        }
    }
    lastTick_ = now;
    if (now >= harpFlashUntil)
        harpFlash = -1;
}
void Instrument::tick(uint64_t now) {
    now_ = now;
    if (looper) looper->tick(now);
    commitChord(now);
    reportChord(now);
    if (previewUntil_) {
        if (now >= previewUntil_) {
            configure();
            return;
        }
        if (previewLoop_) {
            Settings saved = settings;
            Chord savedChord = chord;
            bool savedActive = active;
            settings = previewSettings_;
            chord = previewChord_;
            active = true;
            runTick(now);
            settings = saved;
            chord = savedChord;
            active = savedActive;
        }
    } else
        runTick(now);
}
void Instrument::releaseHeldNotes() {
    auto release = [&](auto &voices) {
        for (auto &voice : voices)
            if (voice) reportOff(voice), sink_.send(Command{CommandType::Off, voice}), voice = 0;
    };
    release(frontVoices_);
    release(allFrontVoices_);
}
void Instrument::allFrontKey(char key, bool down) {
    const int index = allFrontIndex(key);
    if (index >= 0) {
        auto &voice = allFrontVoices_[size_t(index)];
        if (!down) {
            if (voice) reportOff(voice), sink_.send(Command{CommandType::Off, voice});
            voice = 0;
            return;
        }
        const int pitch = playingAllFrontNotes()[size_t(index)];
        if (pitch < 0) return; // A gap in the piano layout.
        sink_.send(Command{CommandType::On, ++nextId_, pitch, 0, Part::Harp});
        reportNote(nextId_, pitch, Part::Harp, 0, 1.f, int(LoopNoteSource::AllFront), index,
                   pitch - allFrontNotes(settings)[size_t(index)]);
        voice = nextId_;
        return;
    }
    if (!down) return;
    if (key == '2' || key == '3') { // Octave shift, while every sounding key stays in range.
        const int step = key == '3' ? 1 : -1;
        const auto notes = allFrontNotes(settings, allFrontShift + step);
        const auto current = allFrontNotes(settings, allFrontShift);
        for (size_t n = 0; n < notes.size(); n++)
            if (current[n] >= 0 && notes[n] < 0) return;
        allFrontShift += step;
    }
    if (key == ' ') stopEverything();
}
void Instrument::setAllFront(bool on) {
    if (on == allFront) return;
    // Every key changes meaning: end held notes and treat held chord keys as released.
    releaseHeldNotes();
    pending_.clear();
    const bool chordHeld = anyChordKey();
    const bool sharp = pressed_['1'];
    std::memset(pressed_, 0, sizeof(pressed_));
    pressed_['1'] = sharp;
    baseColumn_ = -1;
    if (chordHeld && !settings.hold) {
        chordPending_ = false;
        stopChord();
        sink_.send(Command{CommandType::StopChord});
        active = false;
    }
    allFront = on;
}
void Instrument::clearKeys() {
    std::memset(pressed_, 0, sizeof(pressed_));
    frontVoices_.fill(0);
    allFrontVoices_.fill(0);
    baseColumn_ = -1;
}
void Instrument::endMidiNote(size_t index) {
    reportOff(midiNotes_[index].id);
    sink_.send(Command{CommandType::Off, midiNotes_[index].id});
    midiNotes_.erase(midiNotes_.begin() + long(index));
}
void Instrument::midiReleaseAll() {
    while (!midiNotes_.empty()) endMidiNote(0);
}
void Instrument::midiController(CommandType type, int value) {
    int &current = type == CommandType::Bend ? midiBend_ : midiModulation_;
    if (current == value) return;
    current = value;
    Command c{type, 0, value};
    sink_.send(c);
    if (looper) looper->controller(type == CommandType::Bend, value, now_);
}
void Instrument::midiResetControllers() {
    midiController(CommandType::Bend, 0);
    midiController(CommandType::Modulation, 0);
}
void Instrument::midi(const MidiEvent &e, uint64_t now) {
    now_ = now;
    if (e.type == MidiEvent::PitchBend) {
        midiController(CommandType::Bend, std::clamp(e.data1, 0, 16383) - 8192);
        return;
    }
    if (e.type == MidiEvent::Control) {
        if (e.data1 == 64) { // Sustain pedal: lifting it ends the notes it was holding.
            sustain_ = e.data2 >= 64;
            if (!sustain_)
                for (size_t n = midiNotes_.size(); n-- > 0;)
                    if (!midiNotes_[n].down) endMidiNote(n);
        } else if (e.data1 == 1) { // Mod wheel: vibrato depth.
            midiController(CommandType::Modulation, std::clamp(e.data2, 0, 127));
        } else if (e.data1 == 121) { // Reset all controllers.
            midiResetControllers();
        } else if (e.data1 == 120 || e.data1 == 123) { // All sound off, all notes off.
            midiReleaseAll();
        }
        return;
    }
    const int pitch = std::clamp(e.data1, 0, 127);
    if (e.type == MidiEvent::NoteOff) {
        for (size_t n = 0; n < midiNotes_.size(); ++n)
            if (midiNotes_[n].pitch == pitch && midiNotes_[n].down) {
                if (sustain_) midiNotes_[n].down = false;
                else endMidiNote(n);
                break;
            }
        return;
    }
    midiLastNote = pitch;
    midiLastVelocity = e.data2;
    midiFlashUntil_ = now + 150;
    static constexpr Part parts[]{Part::Harp, Part::Chord, Part::Arp,
                                  Part::Pad,  Part::Bass,  Part::Drums};
    const Part part = parts[std::clamp(settings.midiTarget, 0, midiTargetCount - 1)];
    Command c{CommandType::On, ++nextId_, pitch, 0, part};
    c.gain = midiGain(e.data2, settings.midiVelocity);
    c.live = true;
    if (part == Part::Drums) { // Hits ring out like the drum layer's; releases do nothing.
        c.durationMs = 1500;
        sink_.send(c);
        reportNote(c.id, pitch, part, c.durationMs, c.gain, int(LoopNoteSource::Midi), 0, 0);
        return;
    }
    for (size_t n = 0; n < midiNotes_.size(); ++n)
        if (midiNotes_[n].pitch == pitch) { // The same key again: restart it.
            endMidiNote(n);
            break;
        }
    if (midiNotes_.size() >= maxMidiNotes) { // Make way: a pedal-held note first, else the oldest.
        size_t oldest = 0;
        while (oldest < midiNotes_.size() && midiNotes_[oldest].down) ++oldest;
        endMidiNote(oldest < midiNotes_.size() ? oldest : 0);
    }
    sink_.send(c);
    reportNote(c.id, pitch, part, 0, c.gain, int(LoopNoteSource::Midi), 0, 0);
    midiNotes_.push_back({pitch, c.id, true});
}
void Instrument::panic() {
    if (looper) looper->allNotesOff(now_);
    chordPending_ = false;
    sink_.send(Command{});
    midiNotes_.clear(); // The pedal keeps its physical state.
    chordVoices_.clear();
    pending_.clear();
    active = false;
    previewUntil_ = 0;
    frontAudition_ = false;
    clearKeys();
    harpFlash = -1;
}
void Instrument::setBackingMode(PlaybackMode mode) {
    chordPending_ = false;
    stopChord();
    sink_.send(Command{CommandType::StopChord});
    settings.mode = mode;
    sendSettings(settings);
    sentBpm_ = settings.bpm;
    reportStyle();
    resetTransport();
    soundChord();
}
void Instrument::setBacking(unsigned layers, const std::array<int, 5> &mix) {
    const unsigned before = settings.audibleLayers();
    settings.setEnabledLayers(layers);
    settings.layerMix = mix;
    settings.clamp();
    if (settings.audibleLayers() != before) {
        setBackingMode(settings.mode); // Start or stop layers with the current chord.
        return;
    }
    sendSettings(settings); // Ringing notes follow the new levels.
    sentBpm_ = settings.bpm;
    reportStyle();
    lastGain_ = 1; // Configure resets Pad motion; send its next level promptly.
}
void Instrument::configure() {
    if (looper) looper->allNotesOff(now_);
    chordPending_ = false;
    frontAudition_ = false;
    settings.clamp();
    previewUntil_ = 0;
    sink_.send(Command{});
    midiNotes_.clear();
    chordVoices_.clear();
    pending_.clear();
    lastGain_ = 1;
    sendSettings(settings);
    sentBpm_ = settings.bpm;
    reportStyle();
    soundChord();
}
const Chord &Instrument::frontChord() const {
    return !active && looper && looper->chordActive() ? looper->chord() : chord;
}
bool Instrument::frontChordActive() const {
    return active || (looper && looper->chordActive());
}
std::array<int, 7> Instrument::playingFrontNotes() const {
    auto notes = frontNotes(frontChord(), settings);
    for (auto &note : notes)
        note = std::clamp(note + 12 * frontOctaveShift[settings.frontMode] + semitoneShift(), 0, 127);
    return notes;
}
void Instrument::stopAudition() {
    if (!previewUntil_ && !frontAudition_) return;
    configure(); // Silence it and restore the real settings, resounding a live chord.
}
void Instrument::auditionFront(const Settings &s, int key) {
    panic();
    frontAudition_ = true;
    sendSettings(s);
    Chord c{s.chordRoot(6), -1, 6, Quality::Major};
    const auto &envelope = s.envelopes[0].values;
    int duration = std::max(900, std::max(0, envelope[0]) + std::max(0, envelope[1]) + 300);
    sink_.send(Command{CommandType::On, ++nextId_, frontNotes(c, s)[key],
                       duration, Part::Harp});
}
void Instrument::audition(const Chord &c, const Settings &s, bool loop) {
    panic();
    previewSettings_ = s;
    previewChord_ = c;
    previewLoop_ = loop;
    previewUntil_ = now_ + (loop ? 8000 : 1200);
    sendSettings(s);
    auto saved = settings;
    auto savedChord = chord;
    settings = s;
    chord = c;
    resetTransport();
    if (loop) {
        active = true;
        soundChord();
        active = false;
    } else {
        hit(15, 650);
        sink_.send(Command{CommandType::On, ++nextId_, std::clamp(c.root + 24, 0, 127), 900, Part::Harp});
    }
    settings = saved;
    chord = savedChord;
}
void Instrument::stopEverything() {
    if (looper) looper->stop();
    panic();
}
void Instrument::playChord(const Chord &c, uint64_t now) {
    now_ = now;
    const bool starting = !active;
    chord = c;
    active = true;
    chordPending_ = false;
    if (starting) resetTransport();
    soundChord();
}
void Instrument::releaseChord() {
    if (!active) return;
    stopChord();
    sink_.send(Command{CommandType::StopChord});
    active = false;
}
void Instrument::restartTransport(uint64_t now) {
    now_ = now;
    resetTransport();
}
void Instrument::adopt(const Settings &s) {
    const unsigned before = settings.audibleLayers();
    settings = s;
    settings.clamp();
    sendSettings(settings);
    sentBpm_ = settings.bpm;
    lastGain_ = 1;
    if (active && settings.audibleLayers() != before) { // Start or stop layers on the beat.
        stopChord();
        sink_.send(Command{CommandType::StopChord});
        soundChord();
    }
}
void Instrument::reportChord(uint64_t now) {
    if (!looper || chordPending_) return;
    const bool same = active == heardActive_ &&
                      (!active || (chord == heardChord_ && chord.column == heardChord_.column &&
                                   selectedMask_ == heardMask_));
    const int64_t at = chordAt_;
    chordAt_ = -1;
    if (same) return;
    heardActive_ = active;
    heardChord_ = chord;
    heardMask_ = selectedMask_;
    looper->chordChanged(active, LoopChord{chord, selectedMask_, selectedSharp_, bassColumn_},
                         active && at >= 0 ? uint64_t(at) : now);
}
void Instrument::reportNote(uint32_t id, int pitch, Part part, int durationMs, float gain,
                            int source, int index, int shift) {
    if (!looper) return;
    LoopEvent e;
    e.pitch = pitch;
    e.part = part;
    e.durationMs = durationMs;
    e.gain = gain;
    e.source = LoopNoteSource(source);
    e.index = index;
    e.shift = shift;
    e.chord = LoopChord{chord, selectedMask_, selectedSharp_, bassColumn_};
    looper->noteOn(id, e, now_);
}
void Instrument::reportOff(uint32_t id) {
    if (looper) looper->noteOff(id, now_);
}
void Instrument::reportStyle() {
    if (looper) looper->styleChanged(settings, now_);
}
} // namespace zc
