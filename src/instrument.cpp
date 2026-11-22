#include "instrument.hpp"
#include "patterns.hpp"
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
void Instrument::hit(unsigned mask, int duration, float accent, Part part, bool bass) {
    auto notes = chordNotes(chord);
    for (int n = 0; n < 4; n++)
        if (mask & (1u << n)) {
            auto id = ++nextId_;
            if (!duration)
                chordVoices_.push_back(id);
            Command c{CommandType::On, id, std::max(0, notes[n] - (bass ? 12 : 0)), duration, part};
            c.gain = accent;
            sink_.send(c);
        }
}
void Instrument::resetTransport() {
    phase_ = 0;
    nextTick_ = 0;
    patternStarts_.fill(0);
    lastTick_ = now_;
    playingPatterns_ = {settings.chordPattern, settings.pattern};
    lastGain_ = 1;
}
void Instrument::soundChord() {
    stopChord();
    if (!active)
        return;
    if (layerEnabled(settings.mode, PlaybackMode::Pad))
        hit(15, 0, 1.f, Part::Pad);
    if (layerEnabled(settings.mode, PlaybackMode::Chord)) {
        if (settings.chordPattern == 0) hit(15, 400);
        else if (settings.chordPattern == sustainedChordPattern) hit(15, 0);
    }
}
void Instrument::select(int column, uint64_t now) {
    if (baseColumn_ < 0 || !mask(baseColumn_))
        baseColumn_ = column;
    Chord next{settings.chordRoot(baseColumn_) + (pressed_['1'] ? 1 : 0), -1,
               baseColumn_, settings.quality(mask(baseColumn_), baseColumn_)};
    if (column != baseColumn_)
        next.bass = settings.chordRoot(column) + (pressed_['1'] ? 1 : 0);
    else if (active && chord.bass >= 0)
        for (int n = 0; n < 7; n++)
            if (n != baseColumn_ && mask(n))
                next.bass = settings.chordRoot(n) + (pressed_['1'] ? 1 : 0);
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
    if (chordStarting_) {
        now_ = now;
        resetTransport();
    }
    soundChord();
}
void Instrument::pluck(int pitch, int index, uint64_t now) {
    const auto &envelope = settings.envelopes[0].values;
    int duration = std::max(1600, std::max(0, envelope[0]) + std::max(0, envelope[1]) + 300);
    sink_.send(Command{CommandType::On, ++nextId_, pitch, duration, Part::Harp});
    harpFlash = index;
    harpFlashUntil = now + 140;
}
void Instrument::key(char key, bool down, uint64_t now) {
    now_ = now;
    auto k = static_cast<unsigned char>(key);
    if (k >= 128 || pressed_[k] == down)
        return;
    commitChord(now);
    pressed_[k] = down;
    if (key == '1')
        return;
    int col = columnFor(key);
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
    auto harp = std::string("zxcvbnm").find(key);
    if (harp != std::string::npos && (active || settings.frontMode == 1))
        pluck(playingFrontNotes()[harp], int(harp), now);
    if (key == 'k' || key == 'l') {
        pending_.clear();
        auto notes = playingFrontNotes();
        if (settings.frontAction[settings.frontMode] == 1) {
            int step = key == 'l' ? 1 : -1;
            auto limits = std::minmax_element(notes.begin(), notes.end());
            if (*limits.first + step * 12 >= 0 && *limits.second + step * 12 <= 127)
                frontOctaveShift[settings.frontMode] += step;
        } else if (active || settings.frontMode == 1) {
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
        panic();
}
void Instrument::runTick(uint64_t now) {
    for (auto it = pending_.begin(); it != pending_.end();) {
        if (it->at <= now) {
            pluck(it->pitch, it->index, now);
            it = pending_.erase(it);
        } else
            ++it;
    }
    if (active && backingLayers(settings.mode))
        phase_ += double(now - lastTick_) * settings.bpm / 15000.;
    if (active && !chordPending_ && backingLayers(settings.mode)) {
        if (nextTick_ + 32 < phase_)
            nextTick_ = int64_t(phase_); // Do not flood audio after a stall.
        while (nextTick_ <= phase_) {
            for (int layer = 0; layer < 2; ++layer) {
                auto mode = layer == 0 ? PlaybackMode::Chord : PlaybackMode::Arp;
                if (!layerEnabled(settings.mode, mode)) continue;
                const auto &list = patterns(mode);
                int &playing = playingPatterns_[layer];
                playing = std::clamp(playing, 0, int(list.size()) - 1);
                int desired = layer == 0 ? settings.chordPattern : settings.pattern;
                auto &start = patternStarts_[layer];
                if ((nextTick_ - start) % list[playing].ticks == 0 && playing != desired) {
                    playing = desired;
                    start = nextTick_;
                }
                const auto &p = list[playing];
                int at = int((nextTick_ - start) % p.ticks);
                for (const auto &step : p.steps)
                    if (step.tick == at)
                        hit(unsigned(step.notes), std::max(20, int(step.gate * 15000 / settings.bpm)),
                            step.accent, layer == 0 ? Part::Chord : Part::Arp, step.bass);
            }
            ++nextTick_;
        }
        if (layerEnabled(settings.mode, PlaybackMode::Pad)) {
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
    commitChord(now);
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
void Instrument::clearKeys() {
    std::memset(pressed_, 0, sizeof(pressed_));
    baseColumn_ = -1;
}
void Instrument::panic() {
    chordPending_ = false;
    sink_.send(Command{});
    chordVoices_.clear();
    pending_.clear();
    active = false;
    previewUntil_ = 0;
    clearKeys();
    harpFlash = -1;
}
void Instrument::setBackingMode(PlaybackMode mode) {
    chordPending_ = false;
    stopChord();
    sink_.send(Command{CommandType::StopChord});
    settings.mode = mode;
    Command c{CommandType::Configure};
    c.settings = settings;
    sink_.send(c);
    resetTransport();
    soundChord();
}
void Instrument::configure() {
    chordPending_ = false;
    settings.clamp();
    previewUntil_ = 0;
    sink_.send(Command{});
    chordVoices_.clear();
    pending_.clear();
    lastGain_ = 1;
    Command c{CommandType::Configure};
    c.settings = settings;
    sink_.send(c);
    soundChord();
}
std::array<int, 7> Instrument::playingFrontNotes() const {
    auto notes = frontNotes(chord, settings);
    for (auto &note : notes)
        note = std::clamp(note + 12 * frontOctaveShift[settings.frontMode] + semitoneShift(), 0, 127);
    return notes;
}
void Instrument::auditionFront(const Settings &s, int key) {
    panic();
    Command config{CommandType::Configure};
    config.settings = s;
    sink_.send(config);
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
    Command config{CommandType::Configure};
    config.settings = s;
    sink_.send(config);
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
} // namespace zc
