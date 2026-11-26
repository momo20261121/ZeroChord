// Keep the synth implementation separate from the TOML catalog parser to bound
// compiler memory on the CardputerZero. Rendering behavior is unchanged.
#define TSF_IMPLEMENTATION
#include "sounds.hpp"
#include "music.hpp"
#include <cmath>
#include <algorithm>
#include <set>
#include <optional>
#include <array>
#include <tsf.h>
namespace zc {
std::vector<SoundFontPreset> soundfontPresets(tsf *synth) {
    std::vector<SoundFontPreset> result;
    for (int n = 0; n < tsf_get_presetcount(synth); n++) {
        const auto &p = synth->presets[n];
        result.push_back({tsf_get_presetname(synth, n), p.bank, p.preset});
    }
    return result;
}
std::vector<int> soundfontSampleRates(tsf *synth, int bank, int program) {
    if (!synth) return {};
    int index = tsf_get_presetindex(synth, bank, program);
    if (index < 0) return {};
    const auto &preset = synth->presets[index];
    std::set<int> rates;
    for (int n = 0; n < preset.regionNum; ++n)
        if (preset.regions[n].sample_rate)
            rates.insert(int(preset.regions[n].sample_rate));
    return {rates.begin(), rates.end()};
}
std::optional<std::array<int, 4>> soundfontEnvelopeDefaults(tsf *synth, int bank, int program,
                                                            int key) {
    if (!synth) return std::nullopt;
    int index = tsf_get_presetindex(synth, bank, program);
    if (index < 0 || !synth->presets[index].regionNum) return std::nullopt;
    const auto &preset = synth->presets[index];
    const tsf_region *region = &preset.regions[0];
    for (int n = 0; n < preset.regionNum; ++n)
        if (preset.regions[n].lokey <= key && key <= preset.regions[n].hikey &&
            preset.regions[n].lovel <= 100 && 100 <= preset.regions[n].hivel) {
            region = &preset.regions[n];
            break;
        }
    auto e = region->ampenv;
    // Key-dependent decay stays in timecents until a note starts.
    if (e.keynumToDecay)
        e.decay = e.decay < -10000.f ? 0.f : tsf_timecents2Secsf(e.decay + e.keynumToDecay * (60.f - key));
    float decay = e.decay;
    // Match soundfontEnvelope's decay mapping: show the time to reach the sustain level.
    if (e.sustain > 0.f && e.sustain < 1.f) decay *= -std::log(e.sustain) / 9.226f;
    return std::array<int, 4>{int(std::lround(e.attack * 1000)), int(std::lround(decay * 1000)),
                              int(std::lround(std::clamp(e.sustain, 0.f, 1.f) * 100)),
                              int(std::lround(e.release * 1000))};
}
void soundfontPanic(tsf *synth) {
    for (int n = 0; n < synth->voiceNum; n++)
        synth->voices[n].playingPreset = -1;
}
void soundfontEnvelope(tsf *synth, int channel, int pitch, int rate, const Envelope &settings) {
    if (settings == Envelope{})
        return; // Keep the native sample envelope (with a bounded attack/release for backing).
    const auto &v = settings.values;
    for (int n = 0; n < synth->voiceNum; n++) {
        auto &voice = synth->voices[n];
        if (voice.playingPreset < 0 || voice.playingChannel != channel ||
            voice.playingKey != pitch || voice.ampenv.segment >= TSF_SEGMENT_RELEASE)
            continue;
        // These parameters already include note-dependent timing and any chord articulation.
        auto envelope = voice.ampenv.parameters;
        envelope.keynumToHold = envelope.keynumToDecay = 0;
        if (v[0] >= 0) { envelope.delay = 0; envelope.attack = v[0] / 1000.f; }
        if (v[2] >= 0) envelope.sustain = v[2] / 100.f;
        if (v[1] >= 0) {
            envelope.hold = 0;
            // TSF's exponential decay duration is specified to silence; expose time to sustain.
            envelope.decay = v[1] / 1000.f;
            if (envelope.sustain > 0.f && envelope.sustain < 1.f)
                envelope.decay *= 9.226f / -std::log(envelope.sustain);
        }
        if (v[3] >= 0) envelope.release = v[3] / 1000.f;
        tsf_voice_envelope_setup(&voice.ampenv, &envelope, pitch, voice.ampenv.midiVelocity,
                                 TSF_TRUE, float(rate));
    }
}
void soundfontArticulate(tsf *synth, int channel, int pitch, int rate, bool sustained) {
    for (int n = 0; n < synth->voiceNum; n++) {
        auto &voice = synth->voices[n];
        if (voice.playingPreset < 0 || voice.playingChannel != channel ||
            voice.ampenv.segment >= TSF_SEGMENT_RELEASE)
            continue;
        auto envelope = voice.ampenv.parameters;
        // Keep each instrument's native decay/sustain and note-dependent timing.
        // Only prevent slow pad attacks/tails from obscuring rhythmic backing.
        envelope.delay = 0;
        envelope.attack = std::min(envelope.attack, 0.01f);
        envelope.keynumToHold = envelope.keynumToDecay = 0;
        envelope.release = std::min(envelope.release, sustained ? 0.25f : 0.2f);
        tsf_voice_envelope_setup(&voice.ampenv, &envelope, pitch, voice.ampenv.midiVelocity,
                                 TSF_TRUE, float(rate));
    }
}
} // namespace zc
