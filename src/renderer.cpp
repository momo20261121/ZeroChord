#define TSF_IMPLEMENTATION
#include "renderer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <tsf.h>
#include <vector>

namespace zc {
struct Renderer::Impl {
    tsf *synth = nullptr;
    int rate;
    Settings settings;
    struct Voice {
        uint32_t id = 0;
        int pitch = 0;
        uint64_t off = 0, age = 0;
        bool held = false;
        Part part = Part::Harp;
        float gain = 1.f;
    };
    std::array<Voice, 64> voices{};
    uint64_t frame = 0;
    std::vector<float> delay;
    size_t delayPos = 0;
    bool muted = true;
    float chordGain = 1.f;
    explicit Impl(int r) : rate(r), delay(size_t(r * 0.23) * 2, 0.f) {}
    ~Impl() {
        if (synth)
            tsf_close(synth);
    }
};
Renderer::Renderer(const std::string &path, int rate) : impl_(new Impl(rate)) {
    auto &s = *impl_;
    s.synth = tsf_load_filename(path.c_str());
    if (!s.synth)
        throw std::runtime_error("Cannot load sound bank: " + path);
    tsf_set_output(s.synth, TSF_STEREO_INTERLEAVED, rate, -17.f);
    tsf_set_max_voices(s.synth, 96);
    for (int i = 0; i < 64; i++) {
        tsf_channel_set_presetnumber(s.synth, i, 0, 0);
        tsf_channel_set_volume(s.synth, i, 1.f);
    }
}
Renderer::~Renderer() = default;
void Renderer::send(const Command &c) {
    auto &s = *impl_;
    if (c.type == CommandType::Panic) {
        for (int i = 0; i < 64; i++) {
            tsf_channel_sounds_off_all(s.synth, i);
            s.voices[i] = {};
        }
        // TSF sounds_off uses a short release. Panic must also prevent those
        // voices reappearing when a new note immediately unmutes the renderer.
        for (int n = 0; n < s.synth->voiceNum; n++)
            s.synth->voices[n].playingPreset = -1;
        s.muted = true;
        std::fill(s.delay.begin(), s.delay.end(), 0.f);
        return;
    }
    if (c.type == CommandType::Configure) {
        s.settings = c.settings;
        s.settings.clamp();
        s.chordGain = 1.f;
        return;
    }
    if (c.type == CommandType::ChordGain) {
        s.chordGain = std::clamp(c.gain, 0.f, 1.f);
        for (int i = 0; i < 64; i++)
            if (s.voices[i].part == Part::Chord)
                tsf_channel_set_volume(
                    s.synth, i, s.settings.chordVolume / 100.f * s.chordGain * s.voices[i].gain);
        return;
    }
    if (c.type == CommandType::StopChord) {
        for (int i = 0; i < 64; i++)
            if (s.voices[i].part == Part::Chord) {
                tsf_channel_sounds_off_all(s.synth, i);
                s.voices[i].held = false;
            }
        return;
    }
    if (c.type == CommandType::Off) {
        for (int i = 0; i < 64; i++)
            if (s.voices[i].id == c.id && s.voices[i].held) {
                tsf_channel_note_off(s.synth, i, s.voices[i].pitch);
                s.voices[i].held = false;
            }
        return;
    }
    int slot = 0;
    s.muted = false;
    auto priority = [](const Impl::Voice &v) {
        return v.id == 0 ? 0 : (!v.held ? 1 : (v.part == Part::Harp ? 2 : 3));
    };
    for (int i = 1; i < 64; i++) {
        int candidate = priority(s.voices[i]), current = priority(s.voices[slot]);
        if (candidate < current || (candidate == current && s.voices[i].age < s.voices[slot].age))
            slot = i;
    }
    tsf_channel_sounds_off_all(s.synth, slot);
    const auto &preset = presets[std::clamp(s.settings.preset, 0, 3)];
    int program = c.part == Part::Chord ? s.settings.chordProgram : s.settings.harpProgram;
    if (program < 0)
        program = c.part == Part::Chord ? preset.chordProgram : preset.harpProgram;
    tsf_channel_set_presetnumber(s.synth, slot, program, 0);
    tsf_channel_set_volume(
        s.synth, slot,
        (c.part == Part::Chord ? s.settings.chordVolume * s.chordGain : s.settings.harpVolume) /
            100.f * c.gain);
    tsf_channel_note_on(s.synth, slot, c.pitch, c.part == Part::Chord ? 0.68f : 0.9f);
    if (c.part == Part::Chord && c.durationMs > 0) {
        // Timed chord notes need articulation even when their preset is a pad.
        // Dream's bank envelope has a 606 ms attack and 2.57 s release, longer
        // than an arp step. Change only the new voices, never shared SF2 regions.
        // This adapter uses the envelope API internal to the pinned TSF version.
        for (int n = 0; n < s.synth->voiceNum; n++) {
            auto &voice = s.synth->voices[n];
            if (voice.playingPreset < 0 || voice.playingChannel != slot ||
                voice.ampenv.segment >= TSF_SEGMENT_RELEASE)
                continue;
            auto envelope = voice.region->ampenv;
            envelope.delay = 0;
            envelope.attack = std::min(envelope.attack, 0.005f);
            envelope.hold = 0;
            envelope.keynumToHold = 0;
            envelope.decay = 0.22f;
            envelope.sustain = 0.08f;
            envelope.keynumToDecay = 0;
            envelope.release = std::min(envelope.release, 0.03f);
            tsf_voice_envelope_setup(&voice.ampenv, &envelope, c.pitch, voice.ampenv.midiVelocity,
                                     TSF_TRUE, float(s.rate));
        }
    }
    s.voices[slot] = {
        c.id,        c.pitch, c.durationMs ? s.frame + uint64_t(c.durationMs) * s.rate / 1000 : 0,
        s.frame + 1, true,    c.part,
        c.gain};
}
void Renderer::render(float *stereo, int frames) {
    auto &s = *impl_;
    for (int start = 0; start < frames; start += 64) {
        int n = std::min(64, frames - start);
        for (int i = 0; i < 64; i++)
            if (s.voices[i].held && s.voices[i].off && s.voices[i].off <= s.frame) {
                tsf_channel_note_off(s.synth, i, s.voices[i].pitch);
                s.voices[i].held = false;
            }
        tsf_render_float(s.synth, stereo + start * 2, n, 0);
        s.frame += n;
    }
    const float wet = s.settings.echo / 100.f;
    if (s.muted) {
        std::fill(stereo, stereo + frames * 2, 0.f);
        return;
    }
    for (int i = 0; i < frames * 2; i++) {
        float dry = stereo[i], echo = s.delay[s.delayPos];
        s.delay[s.delayPos] = dry + echo * 0.25f;
        s.delayPos = (s.delayPos + 1) % s.delay.size();
        stereo[i] = std::clamp((dry + echo * wet) * (s.settings.masterVolume / 100.f) *
                                   (s.settings.presetLevel / 100.f),
                               -1.f, 1.f);
    }
}
} // namespace zc
