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
void Instrument::hit(unsigned mask, int duration, float accent) {
    auto notes = chordNotes(chord);
    for (int n = 0; n < 4; n++)
        if (mask & (1u << n)) {
            auto id = ++nextId_;
            if (!duration)
                chordVoices_.push_back(id);
            Command c{CommandType::On, id, notes[n], duration, Part::Chord};
            c.gain = accent;
            sink_.send(c);
        }
}
void Instrument::resetTransport() {
    phase_ = 0;
    nextTick_ = 0;
    patternStart_ = 0;
    lastTick_ = now_;
    playingPattern_ = patternIndex(settings);
    lastGain_ = 1;
}
void Instrument::soundChord() {
    stopChord();
    if (!active)
        return;
    if (settings.mode == PlaybackMode::Pad)
        hit(15, 0);
    else if (settings.mode == PlaybackMode::Chord && settings.chordPattern == 0)
        hit(15, 400);
}
void Instrument::select(int column, uint64_t now) {
    if (baseColumn_ < 0 || !mask(baseColumn_))
        baseColumn_ = column;
    Chord next{rootFor(settings.tonic, settings.scale, baseColumn_) + (pressed_['1'] ? 1 : 0), -1,
               baseColumn_, settings.quality(mask(baseColumn_), baseColumn_)};
    if (column != baseColumn_)
        next.bass = rootFor(settings.tonic, settings.scale, column) + (pressed_['1'] ? 1 : 0);
    else if (active && chord.bass >= 0)
        for (int n = 0; n < 7; n++)
            if (n != baseColumn_ && mask(n))
                next.bass = rootFor(settings.tonic, settings.scale, n) + (pressed_['1'] ? 1 : 0);
    bool starting = !active;
    selectedMask_ = mask(baseColumn_);
    selectedSharp_ = pressed_['1'];
    chord = next;
    active = true;
    pending_.clear();
    if (starting) {
        now_ = now;
        resetTransport();
    }
    soundChord();
}
void Instrument::pluck(int pitch, int index, uint64_t now) {
    sink_.send(Command{CommandType::On, ++nextId_, pitch, 1600, Part::Harp});
    harpFlash = index;
    harpFlashUntil = now + 140;
}
void Instrument::key(char key, bool down, uint64_t now) {
    now_ = now;
    auto k = static_cast<unsigned char>(key);
    if (k >= 128 || pressed_[k] == down)
        return;
    pressed_[k] = down;
    int col = columnFor(key);
    if (!down) {
        if (col >= 0 && !anyChordKey()) {
            baseColumn_ = -1;
            if (!settings.hold) {
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
    if (harp != std::string::npos && active)
        pluck(harpNotes(chord)[harp], int(harp), now);
    if ((key == 'k' || key == 'l') && active) {
        pending_.clear();
        auto notes = harpNotes(chord);
        for (int i = 0; i < 7; i++) {
            int n = key == 'l' ? i : 6 - i;
            pending_.push_back({now + uint64_t(i * settings.strumMs / 6), notes[n], n});
        }
    }
    if (key == 'i') {
        settings.hold = !settings.hold;
        if (!settings.hold && !anyChordKey()) {
            stopChord();
            sink_.send(Command{CommandType::StopChord});
            active = false;
            pending_.clear();
        }
    }
    if (key == 'o') {
        sink_.send(Command{CommandType::StopChord});
        settings.mode = PlaybackMode((int(settings.mode) + 1) % 3);
        pending_.clear();
        resetTransport();
        soundChord();
    }
    if (key == '9' || key == '0') {
        Settings previous = settings;
        library.select(settings,
                       (settings.preset + (key == '0' ? 1 : int(library.entries.size()) - 1)) %
                           int(library.entries.size()));
        if (active) {
            chord.root += rootFor(settings.tonic, settings.scale, chord.column) -
                          rootFor(previous.tonic, previous.scale, chord.column);
            // Preserve the selected fingering under the new preset's mappings.
            chord.quality = settings.quality(selectedMask_, chord.column);
            if (chord.bass >= 0) {
                int oldBass = (chord.bass - (selectedSharp_ ? 1 : 0)) % 12;
                for (int col = 0; col < 7; col++)
                    if (rootFor(previous.tonic, previous.scale, col) % 12 == oldBass) {
                        chord.bass =
                            rootFor(settings.tonic, settings.scale, col) + (selectedSharp_ ? 1 : 0);
                        break;
                    }
            }
        }
        resetTransport();
        configure();
    }
    if (key == 'p') {
        if (lastTap_ && now > lastTap_ && now - lastTap_ >= 250 && now - lastTap_ <= 1500)
            settings.bpm = int(60000 / (now - lastTap_));
        lastTap_ = now;
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
    if (active) {
        phase_ += double(now - lastTick_) * settings.bpm / 15000.;
        const auto &list = patterns(settings.mode);
        playingPattern_ = std::clamp(playingPattern_, 0, int(list.size()) - 1);
        if (nextTick_ + 32 < phase_)
            nextTick_ = int64_t(phase_); // Do not flood audio after a stall.
        while (nextTick_ <= phase_) {
            if ((nextTick_ - patternStart_) % list[playingPattern_].ticks == 0 &&
                playingPattern_ != patternIndex(settings)) {
                playingPattern_ = patternIndex(settings);
                patternStart_ = nextTick_;
            }
            const auto &p = list[playingPattern_];
            int at = int((nextTick_ - patternStart_) % p.ticks);
            for (const auto &step : p.steps)
                if (step.tick == at)
                    hit(unsigned(step.notes), std::max(20, int(step.gate * 15000 / settings.bpm)),
                        step.accent);
            ++nextTick_;
        }
        if (settings.mode == PlaybackMode::Pad) {
            constexpr double pi = 3.141592653589793;
            float gain = 1;
            if (playingPattern_ == 1)
                gain = float(.7 + .3 * std::sin(2 * pi * phase_ / 64 - pi / 2));
            if (playingPattern_ == 2)
                gain = float(.65 + .35 * std::sin(2 * pi * phase_ / 8 - pi / 2));
            if (playingPattern_ == 3)
                gain = float(.3 + .7 * std::exp(-std::fmod(phase_, 4.) * 1.2));
            if (now >= lastGainAt_ + 20 && std::abs(gain - lastGain_) > .015f) {
                Command c{CommandType::ChordGain};
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
    sink_.send(Command{});
    chordVoices_.clear();
    pending_.clear();
    active = false;
    previewUntil_ = 0;
    clearKeys();
    harpFlash = -1;
}
void Instrument::configure() {
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
        sink_.send(Command{CommandType::On, ++nextId_, c.root + 24, 900, Part::Harp});
    }
    settings = saved;
    chord = savedChord;
}
} // namespace zc
