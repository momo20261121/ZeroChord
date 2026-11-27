#include "renderer.hpp"
#include "equalizer.hpp"
#include "patterns.hpp"
#include "presets.hpp"
#include "sounds.hpp"
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace zc;
struct DirectSink : Sink {
    Renderer &renderer;
    explicit DirectSink(Renderer &r) : renderer(r) {}
    void send(const Command &command) override { renderer.send(command); }
};
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    try {
        // Measure EQ response with tones; test boost/cut at two output rates,
        // exact neutral bypass and independent left/right filter state.
        for (int rate : {44100, 48000}) {
            auto response = [&](int band, int gain, double hz) {
                Equalizer eq;
                std::array<int, 3> gains{}; gains[band] = gain;
                eq.configure(gains, rate);
                double input = 0, output = 0;
                for (int n = 0; n < rate; n++) {
                    float x = float(.1 * std::sin(6.283185307179586 * hz * n / rate));
                    float y = eq.process(x, 0);
                    if (gain == 0 && y != x) throw std::runtime_error("neutral EQ changed samples");
                    if (!std::isfinite(y) || eq.process(0, 1) != 0)
                        throw std::runtime_error("EQ unstable or stereo crosstalk");
                    if (n > rate / 2) { input += x * x; output += y * y; }
                }
                return 10 * std::log10(output / input);
            };
            for (int band = 0; band < 3; band++) {
                const double hz[]{60, 1000, 12000};
                for (int gain : {-12, -6, 0, 6, 12})
                    if (std::abs(response(band, gain, hz[band]) - gain) > .25)
                        throw std::runtime_error("EQ gain differs from its dB label");
                if (band != 1 && std::abs(response(band, 12, hz[2 - band])) > .1)
                    throw std::runtime_error("shelf EQ changed the opposite band");
            }
        }
        // A MIDI note (live) keeps sounding through a chord stop; a backing note does not.
        {
            std::vector<float> buffer(2 * 9600);
            auto afterStop = [&](bool live) {
                Renderer r(argv[1]);
                Command config{CommandType::Configure};
                config.settings.echo = 0;
                config.settings.reverbSend.fill(0);
                r.send(config);
                Command note{CommandType::On, 1, 60, 0, Part::Pad};
                note.live = live;
                r.send(note);
                r.render(buffer.data(), 9600);
                r.send(Command{CommandType::StopChord});
                for (int n = 0; n < 3; n++) r.render(buffer.data(), 9600);
                r.meter();
                r.render(buffer.data(), 9600);
                return r.meter().peak;
            };
            const float live = afterStop(true), backing = afterStop(false);
            if (!(live > .01f && backing < live * .05f))
                throw std::runtime_error("chord stop cut a MIDI note, or kept a backing note");
        }
        // EQ must be wired after echo on the complete mix, before master volume.
        {
            Renderer flat(argv[1]), shaped(argv[1]);
            Command config{CommandType::Configure}; config.settings.echo = 40;
            // Below the limiter, which would otherwise act differently on the two mixes.
            config.settings.chordVolume = config.settings.harpVolume = 30;
            flat.send(config);
            config.settings.eqDb = {6, -3, 2}; shaped.send(config);
            for (auto *voice : {&flat, &shaped}) {
                voice->send({CommandType::On, 1, 48, 300, Part::Chord});
                voice->send({CommandType::On, 2, 72, 200, Part::Harp});
            }
            std::vector<float> dry(96000), actual(96000);
            flat.render(dry.data(), 48000); shaped.render(actual.data(), 48000);
            Equalizer expected; expected.configure(config.settings.eqDb, 48000);
            double difference = 0;
            for (size_t n = 0; n < actual.size(); n++) {
                float value = std::clamp(expected.process(dry[n], int(n % 2)), -1.f, 1.f);
                if (!std::isfinite(actual[n]) || std::abs(actual[n] - value) > .000001f)
                    throw std::runtime_error("EQ did not process the complete dry/wet stereo mix");
                difference += std::abs(actual[n] - dry[n]);
            }
            if (difference < .01) throw std::runtime_error("EQ has no audible signal effect");
            shaped.send(Command{}); shaped.render(actual.data(), 48000);
            for (float value : actual)
                if (value != 0) throw std::runtime_error("panic left EQ ringing");
            config.settings.masterVolume = 0; shaped.send(config);
            shaped.send({CommandType::On, 3, 48, 0, Part::Chord});
            shaped.render(actual.data(), 48000);
            for (float value : actual)
                if (value != 0) throw std::runtime_error("EQ bypassed master mute");
        }
        Renderer r(argv[1]);
        std::vector<float> samples(48000 * 2);
        float peak = 0;
        for (int preset = 0; preset < 4; preset++) {
            Command config;
            config.type = CommandType::Configure;
            config.settings.preset = preset;
            r.send(config);
            r.send({CommandType::On, 1, 60, 0, Part::Chord});
            r.send({CommandType::On, 2, 60, 400, Part::Harp});
            r.render(samples.data(), 48000);
            double energy = 0;
            for (float x : samples) {
                if (!std::isfinite(x))
                    throw std::runtime_error("nonfinite audio");
                energy += x * x;
                peak = std::max(peak, std::abs(x));
            }
            if (energy < 0.0001)
                throw std::runtime_error("silent preset");
            r.send({CommandType::Off, 2});
            r.render(samples.data(), 48000);
            energy = 0;
            for (float x : samples)
                energy += x * x;
            if (energy < 0.0001)
                throw std::runtime_error("releasing overlapping harp killed chord");
            r.send(Command{});
            r.render(samples.data(), 48000);
            for (float x : samples)
                if (x != 0)
                    throw std::runtime_error("panic left audible samples");
        }
        // Exercise every assignable quality through the actual performance path,
        // including the new no3/sus voicings and their seven harp notes.
        DirectSink sink(r);
        Instrument instrument(sink);
        instrument.settings.combinationMs = 0; // Rendering checks; recognition has its own tests.
        for (int preset = 0; preset < 4; preset++)
            for (int quality = 0; quality < qualityCount; quality++) {
                instrument.panic();
                instrument.settings.preset = preset;
                instrument.settings.combinations[0] = quality;
                instrument.configure();
                instrument.key('8', true, 1);
                instrument.key('u', true, 2);
                for (char key : std::string("zxcvbnm"))
                    instrument.key(key, true, 3);
                r.render(samples.data(), 24000);
                double energy = 0;
                for (int n = 0; n < 48000; n++) {
                    float sample = samples[n];
                    if (!std::isfinite(sample))
                        throw std::runtime_error("nonfinite customized chord audio");
                    energy += sample * sample;
                    peak = std::max(peak, std::abs(sample));
                }
                if (energy < 0.0001)
                    throw std::runtime_error("silent customized chord");
            }
        instrument.panic();
        // A pad preset must make a short audible arp note, then leave a gap.
        // Previously Dream's 606 ms attack / 2.57 s release blurred every step.
        for (int preset = 0; preset < 4; preset++) {
            r.send(Command{});
            Command articulated;
            articulated.type = CommandType::Configure;
            articulated.settings.preset = preset;
            articulated.settings.echo = 0;
            articulated.settings.reverbSend.fill(0); // The dry note itself.
            r.send(articulated);
            r.send({CommandType::On, 2000, 60, 100, Part::Chord});
            r.render(samples.data(), 24000);
            double attackEnergy = 0, tailEnergy = 0;
            for (int n = 0; n < 4800; n++)
                attackEnergy += samples[n] * samples[n];
            for (int n = 28800; n < 48000; n++)
                tailEnergy += samples[n] * samples[n];
            // The tail is judged against the attack: about 90 dB down is silence.
            if (attackEnergy < 0.0001 || tailEnergy > attackEnergy * 1e-9)
                throw std::runtime_error("arp note lacks a clear attack or quiet gap: preset " +
                                         std::to_string(preset) + " attack " +
                                         std::to_string(attackEnergy) + " tail " +
                                         std::to_string(tailEnergy * 1e8) + "e-8");
        }
        // Sounds are level-matched: a loud square lead and a quiet clean guitar land within a
        // few dB of each other at the same Front level (they were 14 dB apart).
        {
            soundLibrary().load("/nonexistent", argv[1]);
            auto level = [&](int program) {
                Renderer matched(argv[1]); DirectSink direct(matched); Instrument player(direct);
                player.settings.frontMode = 1; player.settings.harpProgram = program;
                player.settings.echo = 0; player.settings.reverbSend.fill(0);
                player.configure(); player.key('c', true, 1);
                std::vector<float> block(960);
                double sum = 0;
                for (int ms = 0; ms < 800; ms += 10) {
                    player.tick(ms + 2); matched.render(block.data(), 480);
                    for (float x : block) sum += x * x;
                }
                return 10 * std::log10(sum);
            };
            const double square = level(80), guitar = level(27);
            if (std::abs(square - guitar) > 5)
                throw std::runtime_error("front sounds are not level-matched: " +
                                         std::to_string(square) + " vs " + std::to_string(guitar));
        }
        // Run the real O control and scheduler with Dream, the reported failure.
        Instrument modes(sink);
        modes.settings.combinationMs = 0;
        modes.settings.preset = 1;
        modes.settings.echo = 0;
        modes.settings.reverbSend.fill(0); // Judge the notes, not the room.
        modes.settings.bpm = 100;
        modes.configure();
        modes.key('8', true, 0);
        modes.key('8', false, 1);
        r.render(samples.data(), 48000);
        auto energy = [&](int first, int end) {
            double sum = 0;
            for (int n = first * 96; n < end * 96; n++)
                sum += samples[n] * samples[n];
            return sum;
        };
        if (energy(800, 1000) < .0001)
            throw std::runtime_error("Pad does not sustain");
        modes.key('o', true, 1000);
        modes.setBackingMode(PlaybackMode((int(modes.settings.mode) + 1) % 4));
        modes.key('o', false, 1001);
        for (int ms = 0; ms < 1000; ms += 10) {
            modes.tick(1000 + ms);
            r.render(samples.data() + ms * 96, 480);
        }
        if (energy(0, 300) < .0001 || energy(800, 1000) > .00001)
            throw std::runtime_error("Strum does not play once then become quiet");
        modes.key('o', true, 2000);
        modes.setBackingMode(PlaybackMode((int(modes.settings.mode) + 1) % 4));
        modes.key('o', false, 2001);
        for (int ms = 0; ms < 1000; ms += 10) {
            modes.tick(2000 + ms);
            r.render(samples.data() + ms * 96, 480);
        }
        for (int start : {0, 300, 600})
            if (energy(start, start + 50) < .0001 || energy(start + 180, start + 260) < .00001)
                throw std::runtime_error("Flowing arp has a silent onset or a gap between steps");
        modes.panic();
        r.send({CommandType::On, 3000, 72, 800, Part::Harp});
        r.send({CommandType::StopChord});
        r.render(samples.data(), 4800);
        if (energy(0, 100) < .0001)
            throw std::runtime_error("changing chord mode silenced the harp");
        r.send(Command{});
        Command config;
        config.type = CommandType::Configure;
        config.settings.preset = 3;
        config.settings.echo = 0; config.settings.reverbSend.fill(0);
        r.send(config);
        r.send({CommandType::On, 1000, 48, 0, Part::Chord});
        for (unsigned n = 0; n < 100; n++) {
            r.send({CommandType::On, 1001 + n, 72 + int(n % 12), 100, Part::Harp});
            r.render(samples.data(), 960);
        }
        for (int second = 0; second < 8; second++)
            r.render(samples.data(), 48000);
        double sustainedEnergy = 0;
        for (float x : samples)
            sustainedEnergy += x * x;
        if (sustainedEnergy < 0.0001)
            throw std::runtime_error("rapid harp playing stole the held chord");
        // App Vol is a linear amplitude control for the complete mix with EQ and echo.
        Renderer full(argv[1]);
        Command volume{CommandType::Configure};
        volume.settings.echo = 40;
        volume.settings.eqDb = {3, -2, 2};
        full.send(volume);
        auto playMix = [](Renderer &renderer) {
            renderer.send({CommandType::On, 4000, 60, 400, Part::Chord});
            renderer.send({CommandType::On, 4001, 72, 800, Part::Harp});
        };
        std::vector<float> reference(48000 * 2), quiet(48000 * 2);
        playMix(full);
        full.render(reference.data(), 48000);
        for (int percent : {95, 75, 50, 25, 10, 5, 0}) {
            Renderer attenuated(argv[1]);
            volume.settings.masterVolume = percent;
            attenuated.send(volume);
            playMix(attenuated);
            attenuated.render(quiet.data(), 48000);
            for (size_t n = 0; n < reference.size(); n++)
                if (std::abs(quiet[n] - reference[n] * (percent / 100.f)) > 1e-6f)
                    throw std::runtime_error("App Vol did not scale the mix at " +
                                             std::to_string(percent) + "%");
        }
        // Pad motion changes only the backing layer, never the foreground.
        full.send(Command{});
        Renderer scaled(argv[1]);
        volume.settings = Settings{};
        full.send(volume);
        scaled.send(volume);
        full.send({CommandType::On, 5000, 72, 800, Part::Harp});
        scaled.send({CommandType::On, 5000, 72, 800, Part::Harp});
        Command motion{CommandType::ChordGain};
        motion.gain = .2f;
        scaled.send(motion);
        full.render(reference.data(), 48000);
        scaled.render(quiet.data(), 48000);
        for (size_t n = 0; n < reference.size(); n++)
            if (std::abs(reference[n] - quiet[n]) > 1e-6f)
                throw std::runtime_error("pad motion changed foreground sound");
        // Each native instrument keeps its own decay/sustain; a manual hit still ends on time.
        for (int program : {89, 16, 48}) {
            r.send(Command{});
            Command c{CommandType::Configure};
            c.settings.chordProgram = program;
            c.settings.echo = 0;
            c.settings.reverbSend.fill(0);
            r.send(c);
            for (int pitch : {48, 52, 55, 60})
                r.send({CommandType::On, uint32_t(6000 + pitch), pitch, 400, Part::Chord});
            r.render(samples.data(), 48000);
            if (energy(5, 70) < .0001 || energy(800, 1000) > .00001)
                throw std::runtime_error("chord hit lacks attack or ignores its gate");
        }
        // Program routing is independent: Pad uses pad sound; Chord/Arp use chord sound.
        auto renderProgram = [&](PlaybackMode mode, int pad, int chord, Part part) {
            Renderer voice(argv[1]);
            Command config{CommandType::Configure};
            config.settings.mode = mode;
            config.settings.padProgram = pad;
            config.settings.chordProgram = chord;
            config.settings.harpProgram = 46;
            config.settings.echo = 0; config.settings.reverbSend.fill(0);
            voice.send(config);
            voice.send({CommandType::On, 1, 60, mode == PlaybackMode::Pad ? 0 : 400,
                        part == Part::Chord && mode == PlaybackMode::Pad ? Part::Pad : part});
            std::vector<float> result(24000 * 2);
            voice.render(result.data(), 24000);
            return result;
        };
        for (auto mode : {PlaybackMode::Pad, PlaybackMode::Chord, PlaybackMode::Arp}) {
            auto base = renderProgram(mode, 10, 16, Part::Chord);
            auto changedPad = renderProgram(mode, 46, 16, Part::Chord);
            auto changedChord = renderProgram(mode, 10, 24, Part::Chord);
            if ((base == changedPad) != (mode != PlaybackMode::Pad) ||
                (base == changedChord) != (mode == PlaybackMode::Pad))
                throw std::runtime_error("pad/chord program routing was not independent");
            if (renderProgram(mode, 10, 16, Part::Harp) != renderProgram(mode, 46, 24, Part::Harp))
                throw std::runtime_error("backing sound changed foreground timbre");
        }
        // Arp has its own sound, and each layer's mix scales only that layer.
        auto renderLayer = [&](int chord, int arp, Part part, int arpMix = 100) {
            Renderer voice(argv[1]);
            Command config{CommandType::Configure};
            config.settings.mode = PlaybackMode::ChordArp;
            config.settings.chordProgram = chord;
            config.settings.arpProgram = arp;
            config.settings.mix(PlaybackMode::Arp) = arpMix;
            config.settings.echo = 0; config.settings.reverbSend.fill(0);
            voice.send(config);
            voice.send({CommandType::On, 1, 60, 400, part});
            std::vector<float> result(24000 * 2);
            voice.render(result.data(), 24000);
            return result;
        };
        if (renderLayer(16, 16, Part::Arp) == renderLayer(16, 24, Part::Arp) ||
            renderLayer(16, 16, Part::Chord) != renderLayer(16, 24, Part::Chord) ||
            renderLayer(16, 24, Part::Arp) != renderLayer(4, 24, Part::Arp))
            throw std::runtime_error("arp sound routing was not independent of chord");
        auto layerEnergy = [](const std::vector<float> &v) {
            double total = 0;
            for (float x : v) total += x * x;
            return total;
        };
        const double fullArp = layerEnergy(renderLayer(16, 24, Part::Arp));
        if (!(layerEnergy(renderLayer(16, 24, Part::Arp, 50)) < fullArp * .5) || fullArp <= 0 ||
            layerEnergy(renderLayer(16, 24, Part::Arp, 0)) != 0)
            throw std::runtime_error("layer mix did not scale its layer");
        // All complete factory presets produce audible, finite audio through their own patterns.
        for (int n = 0; n < int(instrument.library.entries.size()); n++) {
            instrument.panic();
            instrument.library.select(instrument.settings, n);
            instrument.configure();
            instrument.key('8', true, 10000);
            instrument.key('8', false, 10001);
            double total = 0;
            for (int ms = 0; ms < 2400; ms += 10) {
                instrument.tick(10000 + ms);
                r.render(samples.data(), 480);
                for (int frame = 0; frame < 960; frame++) {
                    if (!std::isfinite(samples[frame]))
                        throw std::runtime_error("nonfinite style audio");
                    total += samples[frame] * samples[frame];
                    peak = std::max(peak, std::abs(samples[frame]));
                }
            }
            if (total < .0001)
                throw std::runtime_error("silent factory style");
        }
        // Layer combinations render together without silencing the front, clipping, or leaking Pad motion.
        for (unsigned layers = 0; layers < 8; ++layers) {
            Renderer render(argv[1]); DirectSink direct(render); Instrument p(direct);
            p.settings = p.library.entries[1].sound;
            p.settings.mode = modeForLayers(layers); p.settings.combinationMs = 0;
            p.settings.echo = 0; p.settings.reverbSend.fill(0); p.settings.chordPattern = 1; p.settings.padMotion = 3;
            p.configure(); p.key('8', true, 10); p.key('8', false, 11);
            double total = 0;
            for (int ms = 0; ms < 1600; ms += 10) {
                if (ms % 200 == 0) { p.key('z', true, ms + 12); p.key('z', false, ms + 13); }
                p.tick(ms + 14); render.render(samples.data(), 480);
                for (int n = 0; n < 960; ++n) {
                    if (!std::isfinite(samples[n]) || std::abs(samples[n]) >= .99f)
                        throw std::runtime_error("combined backing layers clipped or became nonfinite");
                    total += samples[n] * samples[n];
                }
            }
            if (total < .0001) throw std::runtime_error("combined layers silenced the front");
            p.panic(); render.render(samples.data(), 480);
            for (int n = 0; n < 960; ++n)
                if (samples[n] != 0) throw std::runtime_error("panic left a backing layer sounding");
        }
        for (auto part : {Part::Harp, Part::Chord, Part::Arp, Part::Pad}) {
            Renderer base(argv[1]), moved(argv[1]);
            Command config{CommandType::Configure}; config.settings.mode = PlaybackMode::All;
            config.settings.harpProgram = config.settings.chordProgram = config.settings.padProgram = 16;
            config.settings.echo = 0; config.settings.reverbSend.fill(0); base.send(config); moved.send(config);
            for (auto *renderer : {&base, &moved}) renderer->send({CommandType::On, 1, 60, 0, part});
            Command motion{CommandType::ChordGain}; motion.gain = .2f; moved.send(motion);
            std::vector<float> a(9600), b(9600); base.render(a.data(), 4800); moved.render(b.data(), 4800);
            double difference = 0;
            for (size_t n = 0; n < a.size(); ++n) difference += std::abs(a[n] - b[n]);
            if ((difference > .001) != (part == Part::Pad))
                throw std::runtime_error("Pad motion leaked into a different backing layer");
        }
        // Each envelope changes only its routed part; ADSR affects actual rendered audio.
        auto envelopeAudio = [&](int part, const Envelope &own, bool other = false, bool stop = false) {
            Renderer renderer(argv[1]);
            Command config{CommandType::Configure};
            config.settings.echo = 0; config.settings.reverbSend.fill(0);
            config.settings.harpProgram = config.settings.chordProgram = config.settings.padProgram = 16;
            config.settings.mode = part == 2 ? PlaybackMode::Pad : PlaybackMode::Chord;
            config.settings.envelopes[other ? (part + 1) % 3 : part] = own;
            renderer.send(config);
            renderer.send({CommandType::On, 9000, 60, 0, part == 0 ? Part::Harp : part == 2 ? Part::Pad : Part::Chord});
            std::vector<float> out(48000 * 2);
            renderer.render(out.data(), 19200); // End note at 400 ms.
            renderer.send({stop ? CommandType::StopChord : CommandType::Off, 9000});
            renderer.render(out.data() + 38400, 28800);
            return out;
        };
        auto envelopeEnergy = [](const std::vector<float> &out, int begin, int end) {
            double sum = 0;
            for (int n = begin * 96; n < end * 96; n++) {
                if (!std::isfinite(out[n])) throw std::runtime_error("nonfinite envelope audio");
                sum += out[n] * out[n];
            }
            return sum;
        };
        for (int part = 0; part < 3; part++) {
            Envelope slow; slow.values = {500, 0, 100, 500};
            auto original = envelopeAudio(part, Envelope{});
            if (original != envelopeAudio(part, slow, true))
                throw std::runtime_error("envelope leaked into another part");
            auto shaped = envelopeAudio(part, slow);
            Envelope quick; quick.values = {0, 0, 100, 10};
            auto fast = envelopeAudio(part, quick);
            if (envelopeEnergy(shaped, 0, 40) >= envelopeEnergy(fast, 0, 40) * .1)
                throw std::runtime_error("custom attack did not soften the onset");
            Envelope release; release.values = {0, 0, 100, 500};
            auto tail = envelopeAudio(part, release);
            if (envelopeEnergy(tail, 430, 500) <= envelopeEnergy(fast, 430, 500) + .00001)
                throw std::runtime_error("custom release did not retain a tail");
            if (part == 2 && tail != envelopeAudio(part, release, false, true))
                throw std::runtime_error("stopping backing bypassed custom pad release");
            Envelope decay; decay.values = {0, 100, 10, 10};
            auto faded = envelopeAudio(part, decay);
            if (envelopeEnergy(faded, 250, 350) >= envelopeEnergy(fast, 250, 350) * .1)
                throw std::runtime_error("decay/sustain did not lower the held level");
        }
        // A slow front envelope must have time to develop in actual playing, as in preview.
        {
            Renderer slow(argv[1]); DirectSink direct(slow); Instrument player(direct);
            player.settings.frontMode = 1; player.settings.harpProgram = 16;
            player.settings.echo = 0; player.settings.envelopes[0].values = {3000, 0, 100, 10};
            player.settings.reverbSend.fill(0);
            player.configure(); player.key('z', true, 1);
            double heldEnergy = 0, releasedEnergy = 0;
            for (int ms = 0; ms < 2800; ms += 10) {
                if (ms == 2500) player.key('z', false, ms + 2); // Letting go starts the release.
                player.tick(ms + 3); slow.render(samples.data(), 480);
                for (int n = 0; n < 960; n++) {
                    if (ms >= 2200 && ms < 2500) heldEnergy += samples[n] * samples[n];
                    if (ms >= 2700) releasedEnergy += samples[n] * samples[n];
                }
            }
            if (heldEnergy < .0001) throw std::runtime_error("slow front attack was cut short while held");
            if (releasedEnergy > heldEnergy * .01)
                throw std::runtime_error("releasing a front key did not end its note");
            player.panic(); slow.render(samples.data(), 480);
            for (int n = 0; n < 960; n++)
                if (samples[n] != 0) throw std::runtime_error("panic did not stop a long front envelope");
        }
        // Sustained chord has a prompt attack and audible late body, unlike a
        // one-time hit, while foreground and the separate Pad route remain intact.
        for (int program : {16, 89}) {
            Renderer backing(argv[1]); DirectSink direct(backing); Instrument player(direct);
            player.settings.mode = PlaybackMode::Chord;
            player.settings.chordPattern = sustainedChordPattern;
            player.settings.chordProgram = program;
            player.settings.echo = 0; player.settings.reverbSend.fill(0); player.settings.hold = 0;
            player.settings.combinationMs = 0;
            player.configure(); player.key('8', true, 1);
            double attack = 0, late = 0;
            for (int ms = 0; ms < 2400; ms += 10) {
                player.tick(ms + 1); backing.render(samples.data(), 480);
                for (int n = 0; n < 960; n++) {
                    if (!std::isfinite(samples[n])) throw std::runtime_error("sustained nonfinite audio");
                    if (ms < 50) attack += samples[n] * samples[n];
                    if (ms >= 2000) late += samples[n] * samples[n];
                }
            }
            if (attack < .0001 || late < .0001)
                throw std::runtime_error("sustained chord lacks attack or held body");
            player.key('8', false, 2401);
            backing.render(samples.data(), 48000);
            double tail = 0;
            for (int n = 48000; n < 96000; n++) tail += samples[n] * samples[n];
            if (tail > .00000001) throw std::runtime_error("sustained chord ignored release");
        }
        // Drums: every built-in kit sounds; a 0% mix is silent; every style stays unclipped
        // with all of its layers, including the drum groove.
        {
            Renderer drums(argv[1]);
            std::vector<float> hit(9600 * 2);
            auto energy = [&] {
                double sum = 0;
                for (float x : hit) {
                    if (!std::isfinite(x)) throw std::runtime_error("nonfinite drum audio");
                    sum += x * x;
                }
                return sum;
            };
            for (int kit : {0, 8, 16, 24, 25, 32, 40, 48})
                for (int mix : {60, 0}) {
                    Command config{CommandType::Configure};
                    config.settings.mode = PlaybackMode::FrontOnly;
                    config.settings.drumKit = kit;
                    config.settings.mix(PlaybackMode::Drums) = mix;
                    drums.send(Command{});
                    drums.send(config);
                    drums.send({CommandType::On, 1, 38, 200, Part::Drums});
                    drums.render(hit.data(), 9600);
                    if (mix ? energy() < 0.0001 : energy() > 0)
                        throw std::runtime_error("drum kit " + std::to_string(kit) +
                                                 (mix ? " is silent" : " ignores its mix"));
                }
            // Optional sound packs (ZEROCHORD_SOUNDS_DIR): every pack kit plays through Drums.
            if (std::getenv("ZEROCHORD_SOUNDS_DIR")) {
                soundLibrary().load("/nonexistent", argv[1]);
                Renderer packs(argv[1]);
                int kits = 0;
                for (const auto &sound : soundLibrary().entries) {
                    if (sound.category != "Drums") continue;
                    Command config{CommandType::Configure};
                    config.settings.mode = PlaybackMode::FrontOnly;
                    config.settings.drumSound = sound.id;
                    config.settings.mix(PlaybackMode::Drums) = 60;
                    packs.send(Command{});
                    packs.send(config);
                    packs.send({CommandType::On, 1, 38, 200, Part::Drums});
                    packs.render(hit.data(), 9600);
                    if (energy() < 0.0001) throw std::runtime_error(sound.id + " is silent");
                    ++kits;
                }
                std::cout << "Pack kits played: " << kits << "\n";
                soundLibrary().load("/nonexistent", argv[1]);
            }
            PresetLibrary styles;
            for (int n = 0; n < int(styles.entries.size()); n++) {
                Renderer style(argv[1]);
                DirectSink direct(style);
                Instrument player(direct);
                player.library.select(player.settings, n);
                player.settings.combinationMs = 0;
                player.configure();
                player.key('8', true, 0);
                double drumsOnly = 0;
                for (int ms = 0; ms < 4000; ms += 10) {
                    player.tick(ms);
                    style.render(hit.data(), 480);
                    for (int i = 0; i < 960; i++) {
                        if (!std::isfinite(hit[i])) throw std::runtime_error("nonfinite style");
                        peak = std::max(peak, std::abs(hit[i]));
                        drumsOnly += hit[i] * hit[i];
                    }
                }
                if (drumsOnly < 0.001)
                    throw std::runtime_error(styles.entries[n].id + " style is silent");
            }
        }
        {
            // The loop has its own slot: live Panic and StopChord leave its notes sounding,
            // and stopping the loop leaves live playing alone.
            Renderer loop(argv[1]);
            std::vector<float> block(4800 * 2);
            auto energy = [&] {
                loop.render(block.data(), 4800);
                double sum = 0;
                for (float x : block) sum += double(x) * x;
                return sum;
            };
            Command config{CommandType::Configure};
            loop.send(config);
            config.slot = 1;
            loop.send(config);
            Command looped{CommandType::On, 0x80000001u, 60, 0, Part::Pad};
            looped.slot = 1;
            loop.send(looped);
            energy();
            loop.send(Command{});                     // Live Panic.
            loop.send(Command{CommandType::StopChord}); // Live chord released.
            if (energy() < 1e-5) throw std::runtime_error("live panic stopped the loop");
            loop.send({CommandType::On, 7, 72, 0, Part::Harp});
            Command stop{};
            stop.slot = 1; // The loop stops.
            loop.send(stop);
            for (int n = 0; n < 5; ++n) energy(); // The loop's release fades.
            if (energy() < 1e-6) throw std::runtime_error("stopping the loop stopped live notes");
        }
        if (peak >= 0.99f)
            throw std::runtime_error("preset clipping");
        std::cout << "PASS: four presets, finite nonzero audio, independent overlapping notes, "
                     "panic; peak="
                  << peak << "\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
