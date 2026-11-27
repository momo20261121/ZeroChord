#include "renderer.hpp"
#include "effects.hpp"
#include "equalizer.hpp"
#include "patterns.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <stdexcept>
#include <tsf.h>
#include <vector>

namespace zc {
namespace {
constexpr int partCount = 6; // Part values index the buses.
constexpr int block = 64;
// The synth's own level. Layers no longer share a power budget, so headroom comes from the
// limiter; this sits 15 dB above the earlier -17 dB.
constexpr float synthGainDb = -2.f;
int fxPart(Part part) {
    switch (part) {
    case Part::Harp: return FrontFx;
    case Part::Chord: return ChordFx;
    case Part::Arp: return ArpFx;
    case Part::Pad: return PadFx;
    case Part::Bass: return BassFx;
    case Part::Drums: return DrumsFx;
    }
    return ChordFx;
}
} // namespace
struct Renderer::Impl {
    // Each bank has one synth per part, sharing its samples, so every part renders to its
    // own bus for panning, reverb and echo sends.
    std::map<std::string, std::array<tsf *, partCount>> synths;
    tsf *builtin(Part part) const { return synths.at("")[size_t(part)]; }
    int rate;
    Settings settings;
    Equalizer equalizer;
    Reverb reverb;
    Echo echo;
    Limiter limiter;
    bool headphones = true; // Jack state for Auto; the desktop preview has no speaker.
    bool speaker = false;
    Biquad dcBlock, speakerHighpass, bassBand, harmonicHighpass, harmonicLowpass, drumClick;
    struct Voice {
        uint32_t id = 0;
        int pitch = 0;
        uint64_t off = 0, age = 0;
        bool held = false;
        Part part = Part::Harp;
        float gain = 1.f;
        tsf *synth = nullptr;
        bool releaseOverride = false;
        float level = 1.f; // The sound's level-matching gain.
        bool live = false;  // A MIDI note: chord changes leave it alone.
    };
    // The built-in kits' kicks are 13-42 ms clicks: add a short sine "thump" beneath them,
    // falling from about 140 to 50 Hz.
    struct Thump {
        float phase = 0, time = -1, amplitude = 0;
    };
    std::array<Thump, 4> thumps{};
    int nextThump = 0;
    float thump() {
        float sum = 0;
        const float dt = 1.f / rate;
        for (auto &t : thumps) {
            if (t.time < 0) continue;
            const float frequency = 50.f + 90.f * std::exp(-t.time / .035f);
            t.phase += 6.2831853f * frequency * dt;
            if (t.phase > 6.2831853f) t.phase -= 6.2831853f;
            const float envelope = (1.f - std::exp(-t.time / .0015f)) * std::exp(-t.time / .14f);
            sum += std::sin(t.phase) * envelope * t.amplitude;
            t.time += dt;
            if (t.time > .8f) t.time = -1;
        }
        return sum;
    }
    std::array<Voice, 64> voices{};
    struct Pending {
        uint64_t at;
        uint32_t id;
        int pitch, durationMs;
        Part part;
        float gain;
        bool live;
    };
    std::array<Pending, 128> pending{};
    int pendingCount = 0;
    uint64_t frame = 0;
    float meterPeak = 0;
    double meterSum = 0;
    uint64_t meterFrames = 0;
    bool muted = true;
    float chordGain = 1.f;
    std::array<std::array<float, block * 2>, partCount> bus{};
    explicit Impl(int r) : rate(r), reverb(r), echo(r), limiter(r) {
        dcBlock = Biquad::highpass(22, r); // Sample offsets and rumble.
        speakerHighpass = Biquad::highpass(70, r);
        bassBand = Biquad::lowpass(110, r);
        harmonicHighpass = Biquad::highpass(140, r);
        harmonicLowpass = Biquad::lowpass(520, r);
        drumClick = Biquad::highShelf(3200, r, 4);
    }
    // A fader: 100% is full level, 50% about -12 dB. Layers are independent.
    static float fader(int percent) { return std::pow(percent / 100.f, 2.f); }
    float layerGain(Part part) const {
        if (part == Part::Harp) return 1.f;
        const auto layer = part == Part::Pad    ? PlaybackMode::Pad
                           : part == Part::Arp   ? PlaybackMode::Arp
                           : part == Part::Drums ? PlaybackMode::Drums
                           : part == Part::Bass  ? PlaybackMode::Bass
                                                 : PlaybackMode::Chord;
        return fader(settings.mix(layer));
    }
    float volume(const Voice &v) const {
        // The front sits a little above the backing: 1.6 is about +4 dB.
        if (v.part == Part::Harp)
            return 1.6f * settings.harpVolume / 100.f * fader(settings.frontMix) * v.gain * v.level;
        // Backing accents arrive as velocity; the channel volume carries only the mix.
        return settings.chordVolume / 100.f * (v.part == Part::Pad ? chordGain : 1.f) *
               layerGain(v.part) * v.level;
    }
    void route() {
        speaker = settings.output == OutputSpeaker ||
                  (settings.output == OutputAuto && !headphones);
    }
    void resetEffects() {
        reverb.reset(), echo.reset(), limiter.reset(), equalizer.reset();
        for (auto *f : {&dcBlock, &speakerHighpass, &bassBand, &harmonicHighpass, &harmonicLowpass,
                        &drumClick})
            f->reset();
    }
    void start(const Pending &note);
    ~Impl() {
        for (auto &[id, parts] : synths)
            for (auto *synth : parts) tsf_close(synth);
    }
};
Renderer::Renderer(const std::string &path, int rate) : impl_(new Impl(rate)) {
    auto &s = *impl_;
    const auto &library = soundLibrary();
    std::map<std::string, tsf *> sources;
    for (const auto &bank : library.banks) {
        if (bank.id.empty() && bank.path != path) continue;
        sources.emplace(bank.id, bank.data.get());
    }
    tsf *own = nullptr;
    if (!sources.count("")) {
        own = tsf_load_filename(path.c_str());
        if (!own) throw std::runtime_error("Cannot load sound bank: " + path);
        sources[""] = own;
    }
    for (auto &[id, source] : sources) {
        std::array<tsf *, partCount> parts{};
        for (auto &synth : parts) {
            synth = tsf_copy(source);
            if (!synth) throw std::runtime_error("Cannot load sound bank: " + path);
            tsf_set_output(synth, TSF_STEREO_INTERLEAVED, rate, synthGainDb);
            tsf_set_max_voices(synth, 48);
            for (int i = 0; i < 64; i++) {
                tsf_channel_set_presetnumber(synth, i, 0, 0);
                tsf_channel_set_volume(synth, i, 1.f);
            }
        }
        s.synths.emplace(id, parts);
    }
    if (own) tsf_close(own); // The copies keep the shared samples.
    s.echo.setTime(400);
}
Renderer::~Renderer() = default;
void Renderer::send(const Command &c) {
    auto &s = *impl_;
    if (c.type == CommandType::Panic) {
        for (int i = 0; i < 64; i++) {
            for (auto &[id, parts] : s.synths)
                for (auto *synth : parts) tsf_channel_sounds_off_all(synth, i);
            s.voices[i] = {};
        }
        // TSF sounds_off uses a short release. Panic must also prevent those
        // voices reappearing when a new note immediately unmutes the renderer.
        for (auto &[id, parts] : s.synths)
            for (auto *synth : parts) soundfontPanic(synth);
        s.pendingCount = 0;
        for (auto &t : s.thumps) t.time = -1;
        s.muted = true;
        s.resetEffects();
        return;
    }
    if (c.type == CommandType::Configure) {
        s.settings = c.settings;
        s.settings.clamp();
        s.equalizer.configure(s.settings.eqDb, s.rate);
        s.reverb.configure(s.settings.reverbMs);
        s.echo.setTime(std::clamp(45000 / std::max(1, s.settings.bpm), 150, 700));
        s.route();
        s.chordGain = 1.f;
        for (int i = 0; i < 64; i++) // Mix changes reach notes already ringing.
            if (s.voices[i].synth && s.voices[i].part != Part::Harp)
                tsf_channel_set_volume(s.voices[i].synth, i, s.volume(s.voices[i]));
        return;
    }
    if (c.type == CommandType::Tempo) { // A dotted eighth, as a delay in time with the music.
        s.settings.bpm = std::clamp(c.pitch, 40, 240);
        s.echo.setTime(std::clamp(45000 / s.settings.bpm, 150, 700));
        return;
    }
    if (c.type == CommandType::Route) {
        s.headphones = c.pitch != 0;
        s.route();
        return;
    }
    if (c.type == CommandType::ChordGain) {
        s.chordGain = std::clamp(c.gain, 0.f, 1.f);
        for (int i = 0; i < 64; i++)
            if (s.voices[i].part == Part::Pad && s.voices[i].synth)
                tsf_channel_set_volume(s.voices[i].synth, i, s.volume(s.voices[i]));
        return;
    }
    if (c.type == CommandType::StopChord) {
        for (int i = 0; i < 64; i++) // Drum hits ring out on their own.
            if (s.voices[i].part != Part::Harp && s.voices[i].part != Part::Drums &&
                s.voices[i].synth && !s.voices[i].live) {
                if (s.voices[i].releaseOverride) {
                    if (s.voices[i].held)
                        tsf_channel_note_off(s.voices[i].synth, i, s.voices[i].pitch);
                } else tsf_channel_sounds_off_all(s.voices[i].synth, i);
                s.voices[i].held = false;
            }
        // Strum notes still waiting belong to the stopped chord.
        int kept = 0;
        for (int n = 0; n < s.pendingCount; ++n)
            if (s.pending[n].part == Part::Harp || s.pending[n].part == Part::Drums ||
                s.pending[n].live)
                s.pending[kept++] = s.pending[n];
        s.pendingCount = kept;
        return;
    }
    if (c.type == CommandType::Off) {
        for (int n = 0; n < s.pendingCount; ++n) // Not started yet: start and end at once.
            if (s.pending[n].id == c.id && !s.pending[n].durationMs) s.pending[n].durationMs = 60;
        for (int i = 0; i < 64; i++)
            if (s.voices[i].id == c.id && s.voices[i].held) {
                auto &v = s.voices[i];
                // A quick front tap still sounds: release no sooner than 60 ms after the start.
                const uint64_t earliest = v.age - 1 + uint64_t(s.rate) * 60 / 1000;
                if (v.part == Part::Harp && s.frame < earliest) {
                    v.off = earliest;
                    continue;
                }
                tsf_channel_note_off(v.synth, i, v.pitch);
                v.held = false;
            }
        return;
    }
    s.muted = false;
    Impl::Pending note{s.frame + uint64_t(std::max(0, c.delayUs)) * uint64_t(s.rate) / 1000000,
                       c.id, c.pitch, c.durationMs, c.part, c.gain, c.live};
    if (c.delayUs <= 0 || s.pendingCount == int(s.pending.size())) {
        s.start(note);
        return;
    }
    s.pending[s.pendingCount++] = note;
}
void Renderer::Impl::start(const Pending &c) {
    auto &s = *this;
    int slot = 0;
    auto priority = [](const Impl::Voice &v) {
        return v.id == 0 ? 0 : (!v.held ? 1 : (v.part == Part::Harp ? 2 : 3));
    };
    for (int i = 1; i < 64; i++) {
        int candidate = priority(s.voices[i]), current = priority(s.voices[slot]);
        if (candidate < current || (candidate == current && s.voices[i].age < s.voices[slot].age))
            slot = i;
    }
    if (s.voices[slot].synth) tsf_channel_sounds_off_all(s.voices[slot].synth, slot);
    const size_t bus = size_t(c.part);
    const float pan = .5f + s.settings.pan[size_t(fxPart(c.part))] / 100.f;
    if (c.part == Part::Drums) {
        // A kit from the drum bank (128): the built-in bank, or a pack kit by ID.
        tsf *synth = s.builtin(c.part);
        int bank = 128, kit = s.settings.drumKit;
        bool builtinBank = true;
        if (auto *sound = soundLibrary().find(s.settings.drumSound); sound && !sound->bank.empty())
            if (auto it = s.synths.find(sound->bank); it != s.synths.end()) {
                synth = it->second[bus];
                bank = sound->bankNumber;
                kit = sound->program;
                builtinBank = false;
            }
        tsf_channel_sounds_off_all(synth, slot);
        if (builtinBank) tsf_channel_set_presetnumber(synth, slot, kit, 1);
        else tsf_channel_set_bank_preset(synth, slot, bank, kit);
        Impl::Voice voice{};
        voice.part = c.part;
        voice.gain = c.gain;
        tsf_channel_set_pan(synth, slot, pan);
        tsf_channel_set_volume(synth, slot, s.volume(voice));
        tsf_channel_note_on(synth, slot, c.pitch, std::clamp(.85f * c.gain, 0.f, 1.f));
        if (builtinBank && (c.pitch == 35 || c.pitch == 36) && kit != 48) {
            auto &t = s.thumps[size_t(s.nextThump++ % int(s.thumps.size()))];
            t = {0.f, 0.f, .22f * std::clamp(c.gain, 0.f, 1.f) * s.volume(voice)};
        }
        s.voices[slot] = {c.id, c.pitch, s.frame + uint64_t(c.durationMs) * s.rate / 1000,
                          s.frame + 1, true, c.part, c.gain, synth, false, 1.f, c.live};
        return;
    }
    const auto &preset = presets[std::clamp(s.settings.preset, 0, 3)];
    const bool front = c.part == Part::Harp;
    const bool pad = c.part == Part::Pad;
    const bool arp = c.part == Part::Arp;
    const bool bass = c.part == Part::Bass;
    int program = front ? s.settings.harpProgram
                  : pad ? s.settings.padProgram
                  : bass ? s.settings.bassProgram
                  : arp && s.settings.arpProgram >= 0 ? s.settings.arpProgram
                        : s.settings.chordProgram;
    if (program < 0)
        program = front ? preset.harpProgram : pad && preset.padProgram >= 0
                      ? preset.padProgram : preset.chordProgram;
    tsf *synth = s.builtin(c.part);
    bool builtinBank = true;
    const std::string &soundId = front ? s.settings.frontSound
                                 : pad ? s.settings.padSound
                                 : bass ? s.settings.bassSound
                                 : arp && s.settings.arpProgram >= 0 ? s.settings.arpSound
                                       : s.settings.chordSound;
    auto *sound = soundLibrary().find(soundId);
    if (sound && !sound->bank.empty()) {
        auto it = s.synths.find(sound->bank);
        if (it != s.synths.end()) {
            synth = it->second[bus];
            program = sound->program;
            builtinBank = false;
        }
    }
    tsf_channel_sounds_off_all(synth, slot);
    if (builtinBank) tsf_channel_set_presetnumber(synth, slot, program, 0);
    else tsf_channel_set_bank_preset(synth, slot, sound->bankNumber, program);
    Impl::Voice voice{};
    voice.part = c.part;
    voice.gain = c.gain;
    voice.level = soundLibrary().gain(builtinBank ? std::string() : sound->bank, program);
    tsf_channel_set_pan(synth, slot, pan);
    tsf_channel_set_volume(synth, slot, s.volume(voice));
    tsf_channel_note_on(synth, slot, c.pitch, front ? 0.9f : std::clamp(.68f * c.gain, 0.f, 1.f));
    if (!front && !pad)
        soundfontArticulate(synth, slot, c.pitch, s.rate, c.durationMs == 0);
    int envelopePart = front ? FrontEnvelope : pad ? PadEnvelope : arp ? ArpEnvelope
                       : bass ? BassEnvelope : ChordEnvelope;
    soundfontEnvelope(synth, slot, c.pitch, s.rate, s.settings.envelopes[envelopePart]);
    s.voices[slot] = {
        c.id,        c.pitch, c.durationMs ? s.frame + uint64_t(c.durationMs) * s.rate / 1000 : 0,
        s.frame + 1, true,    c.part,
        c.gain,      synth, s.settings.envelopes[envelopePart].values[3] >= 0, voice.level,
        c.live};
}
void Renderer::render(float *stereo, int frames) {
    auto &s = *impl_;
    std::array<float, fxPartCount> reverbSend{}, echoSend{};
    for (int p = 0; p < partCount; ++p) {
        const int fx = fxPart(Part(p));
        reverbSend[size_t(p)] = s.settings.reverbSend[size_t(fx)] / 100.f;
        echoSend[size_t(p)] = s.settings.echoSend[size_t(fx)] / 100.f;
    }
    const float echoLevel = s.settings.echo / 100.f;
    const float master = s.settings.masterVolume / 100.f;
    for (int start = 0; start < frames; start += block) {
        const int n = std::min(block, frames - start);
        for (int i = 0; i < 64; i++)
            if (s.voices[i].held && s.voices[i].off && s.voices[i].off <= s.frame) {
                tsf_channel_note_off(s.voices[i].synth, i, s.voices[i].pitch);
                s.voices[i].held = false;
            }
        for (int k = 0; k < s.pendingCount;) {
            if (s.pending[k].at <= s.frame) {
                const auto note = s.pending[k];
                s.pending[k] = s.pending[--s.pendingCount];
                s.start(note);
            } else ++k;
        }
        for (int p = 0; p < partCount; ++p) {
            auto &bus = s.bus[size_t(p)];
            std::fill(bus.begin(), bus.begin() + n * 2, 0.f);
            for (auto &[id, parts] : s.synths) tsf_render_float(parts[size_t(p)], bus.data(), n, 1);
        }
        s.frame += uint64_t(n);
        float *out = stereo + start * 2;
        if (s.muted) {
            std::fill(out, out + n * 2, 0.f);
            continue;
        }
        for (int i = 0; i < n; ++i) {
            float left = 0, right = 0, reverbLeft = 0, reverbRight = 0, echoLeft = 0, echoRight = 0;
            for (int p = 0; p < partCount; ++p) {
                float l = s.bus[size_t(p)][size_t(i) * 2], r = s.bus[size_t(p)][size_t(i) * 2 + 1];
                if (p == int(Part::Drums)) {
                    const float thump = s.thump();
                    l += thump, r += thump;
                    if (s.speaker) // A little stick click for small speakers.
                        l = s.drumClick.process(l, 0), r = s.drumClick.process(r, 1);
                }
                left += l, right += r;
                reverbLeft += l * reverbSend[size_t(p)], reverbRight += r * reverbSend[size_t(p)];
                echoLeft += l * echoSend[size_t(p)], echoRight += r * echoSend[size_t(p)];
            }
            float wetLeft, wetRight, delayLeft, delayRight;
            s.reverb.process(reverbLeft, reverbRight, wetLeft, wetRight);
            s.echo.process(echoLeft, echoRight, delayLeft, delayRight);
            left += wetLeft + delayLeft * echoLevel;
            right += wetRight + delayRight * echoLevel;
            if (s.speaker) {
                // Small speakers cannot play the bass fundamental: add its octave harmonics
                // (a rectified low band) where the speaker can, and spare it the deep lows.
                const float low = s.bassBand.process((left + right) * .5f, 0);
                const float harmonics =
                    s.harmonicLowpass.process(s.harmonicHighpass.process(std::fabs(low), 0), 0);
                left = s.speakerHighpass.process(left, 0) + harmonics * 1.6f;
                right = s.speakerHighpass.process(right, 1) + harmonics * 1.6f;
            }
            left = s.equalizer.process(s.dcBlock.process(left, 0), 0);
            right = s.equalizer.process(s.dcBlock.process(right, 1), 1);
            s.meterPeak = std::max(s.meterPeak, std::max(std::fabs(left), std::fabs(right)));
            s.meterSum += double(left) * left + double(right) * right;
            ++s.meterFrames;
            s.limiter.process(left, right);
            out[i * 2] = left * master;
            out[i * 2 + 1] = right * master;
        }
    }
}
Renderer::Meter Renderer::meter() {
    auto &s = *impl_;
    Meter m{s.meterPeak, s.meterFrames ? s.meterSum / (2. * double(s.meterFrames)) : 0.};
    s.meterPeak = 0, s.meterSum = 0, s.meterFrames = 0;
    return m;
}
} // namespace zc
