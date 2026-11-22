// Keep the synth implementation separate from the TOML catalog parser to bound
// compiler memory on the CardputerZero. Rendering behavior is unchanged.
#define TSF_IMPLEMENTATION
#include "sounds.hpp"
#include "music.hpp"
#include <cmath>
#include <algorithm>
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
void soundfontPanic(tsf *synth) {
    for (int n = 0; n < synth->voiceNum; n++)
        synth->voices[n].playingPreset = -1;
}
void soundfontEnvelope(tsf *synth, int channel, int pitch, int rate, const Envelope &settings) {
    if (settings == Envelope{})
        return; // Keep the original sample envelope and rhythmic articulation byte-for-byte.
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
void soundfontArticulate(tsf *synth, int channel, int pitch, int rate) {
    for (int n = 0; n < synth->voiceNum; n++) {
        auto &voice = synth->voices[n];
        if (voice.playingPreset < 0 || voice.playingChannel != channel ||
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
        tsf_voice_envelope_setup(&voice.ampenv, &envelope, pitch, voice.ampenv.midiVelocity,
                                 TSF_TRUE, float(rate));
    }
}
} // namespace zc
