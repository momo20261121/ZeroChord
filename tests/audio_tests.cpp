#include "renderer.hpp"
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
            r.send(articulated);
            r.send({CommandType::On, 2000, 60, 100, Part::Chord});
            r.render(samples.data(), 24000);
            double attackEnergy = 0, tailEnergy = 0;
            for (int n = 0; n < 4800; n++)
                attackEnergy += samples[n] * samples[n];
            for (int n = 28800; n < 48000; n++)
                tailEnergy += samples[n] * samples[n];
            if (attackEnergy < 0.0001 || tailEnergy > 0.00000001)
                throw std::runtime_error("arp note lacks a clear attack or quiet gap");
        }
        // Run the real O control and scheduler with Dream, the reported failure.
        Instrument modes(sink);
        modes.settings.preset = 1;
        modes.settings.echo = 0;
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
        modes.key('o', false, 1001);
        for (int ms = 0; ms < 1000; ms += 10) {
            modes.tick(1000 + ms);
            r.render(samples.data() + ms * 96, 480);
        }
        if (energy(0, 300) < .0001 || energy(800, 1000) > .00000001)
            throw std::runtime_error("Strum does not play once then become quiet");
        modes.key('o', true, 2000);
        modes.key('o', false, 2001);
        for (int ms = 0; ms < 1000; ms += 10) {
            modes.tick(2000 + ms);
            r.render(samples.data() + ms * 96, 480);
        }
        for (int start : {0, 300, 600})
            if (energy(start, start + 50) < .0001 || energy(start + 240, start + 290) > .00000001)
                throw std::runtime_error("Arp does not produce distinct pulses");
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
        config.settings.echo = 0;
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
        // Master and preset level multiply the entire output, including echoes.
        Renderer full(argv[1]), scaled(argv[1]);
        Command volume{CommandType::Configure};
        volume.settings.echo = 40;
        full.send(volume);
        volume.settings.masterVolume = 50;
        volume.settings.presetLevel = 60;
        scaled.send(volume);
        for (auto *renderer : {&full, &scaled}) {
            renderer->send({CommandType::On, 4000, 60, 400, Part::Chord});
            renderer->send({CommandType::On, 4001, 72, 800, Part::Harp});
        }
        std::vector<float> reference(48000 * 2), quiet(48000 * 2);
        full.render(reference.data(), 48000);
        scaled.render(quiet.data(), 48000);
        for (size_t n = 0; n < reference.size(); n++)
            if (std::abs(quiet[n] - reference[n] * .3f) > 1e-6f)
                throw std::runtime_error(
                    "master/preset level did not scale dry and wet audio together");
        volume.settings.masterVolume = 0;
        scaled.send(volume);
        scaled.render(quiet.data(), 48000);
        for (float v : quiet)
            if (v != 0)
                throw std::runtime_error("zero master volume was audible");
        // Pad motion changes only the backing layer, never the foreground.
        full.send(Command{});
        scaled.send(Command{});
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
        // The simultaneous chord envelope falls audibly before release, even on Dream/Organ.
        for (int program : {89, 16, 48}) {
            r.send(Command{});
            Command c{CommandType::Configure};
            c.settings.chordProgram = program;
            c.settings.echo = 0;
            r.send(c);
            for (int pitch : {48, 52, 55, 60})
                r.send({CommandType::On, uint32_t(6000 + pitch), pitch, 400, Part::Chord});
            r.render(samples.data(), 48000);
            if (energy(5, 70) < .0001 || energy(260, 330) >= energy(5, 70) * .2)
                throw std::runtime_error("chord hit did not decay clearly");
        }
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
