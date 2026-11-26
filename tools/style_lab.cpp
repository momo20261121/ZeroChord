// Render every built-in style playing I-vi-IV-V with a simple front melody, and report each
// layer's level and where its energy sits (low, low-mid, mid, high), to balance the mixes.
// Usage: zerochord-styles bank.sf2 output-directory [style-id]
//        zerochord-styles bank.sf2 --levels   (each program's measured loudness and gain)
// Set ZEROCHORD_SOUNDS_DIR to include the optional sound packs, and ZEROCHORD_LAB_FULL to
// measure each solo at 100%.
#include "effects.hpp"
#include "renderer.hpp"
#include "patterns.hpp"
#include "presets.hpp"
#include "sounds.hpp"
#include "instrument.hpp"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace zc;
namespace {
// Lowest and highest note each part played (Part order), for checking registers.
std::array<std::pair<int, int>, 6> ranges;
struct Direct : Sink {
    Renderer &renderer;
    explicit Direct(Renderer &r) : renderer(r) {}
    void send(const Command &c) override {
        if (c.type == CommandType::On) {
            auto &r = ranges[size_t(c.part)];
            r = {std::min(r.first, c.pitch), std::max(r.second, c.pitch)};
        }
        renderer.send(c);
    }
};
void little(std::ostream &out, unsigned value, int bytes) {
    for (int i = 0; i < bytes; i++) out.put(char((value >> (8 * i)) & 255));
}
void writeWav(const std::string &path, const std::vector<float> &samples) {
    std::ofstream out(path, std::ios::binary);
    const unsigned bytes = unsigned(samples.size() * 2);
    out << "RIFF";
    little(out, 36 + bytes, 4);
    out << "WAVEfmt ";
    little(out, 16, 4), little(out, 1, 2), little(out, 2, 2), little(out, 48000, 4);
    little(out, 48000 * 4, 4), little(out, 4, 2), little(out, 16, 2);
    out << "data";
    little(out, bytes, 4);
    for (float s : samples) little(out, unsigned(int16_t(std::clamp(s, -1.f, 1.f) * 32767)), 2);
}
struct Take {
    std::vector<float> samples;
    float peak = 0; // Before the limiter.
    double meanSquare = 0;
};
Take play(const std::string &bank, Settings settings, bool melody) {
    Renderer renderer(bank);
    Direct sink(renderer);
    Instrument player(sink);
    player.settings = settings;
    player.settings.combinationMs = 0;
    player.configure();
    const bool waltz = patterns(PlaybackMode::Chord)[settings.chordPattern].ticks % 12 == 0 ||
                       patterns(PlaybackMode::Drums)[settings.drumPattern].ticks % 12 == 0;
    const int beats = waltz ? 3 : 4;
    const double beatMs = 60000. / settings.bpm, barMs = beats * beatMs;
    const char chords[]{'8', '6', '4', '5'};
    const char melodyKeys[]{'c', 'v', 'b', 'v', 'n', 'b', 'v', 'c'};
    const int bars = 8;
    const int total = int(bars * barMs) + 2500;
    Take take;
    std::vector<float> block(960);
    double sum = 0;
    int frames = 0;
    int lastBar = -1, lastBeat = -1;
    char held = 0;
    for (int ms = 0; ms < total; ms += 10) {
        const int bar = int(ms / barMs), beat = int(ms / beatMs);
        if (bar < bars && bar != lastBar) {
            lastBar = bar;
            const char key = chords[bar % 4];
            player.key(key, true, uint64_t(ms));
            player.key(key, false, uint64_t(ms) + 1);
        }
        if (bar >= bars && lastBar < bars + 1) {
            lastBar = bars + 1;
            player.key('i', true, uint64_t(ms)); // Hold off ends the chord.
            player.key('i', false, uint64_t(ms) + 1);
        }
        if (melody && bar < bars && beat != lastBeat && beat % 2 == 0) {
            if (held) player.key(held, false, uint64_t(ms));
            held = melodyKeys[(beat / 2) % 8];
            player.key(held, true, uint64_t(ms) + 1);
        }
        if (held && bar >= bars) player.key(held, false, uint64_t(ms)), held = 0;
        lastBeat = beat;
        player.tick(uint64_t(ms) + 2);
        renderer.render(block.data(), 480);
        const auto m = renderer.meter();
        take.peak = std::max(take.peak, m.peak);
        if (ms >= 500 && bar < bars) sum += m.meanSquare * 480, frames += 480;
        take.samples.insert(take.samples.end(), block.begin(), block.end());
    }
    take.meanSquare = frames ? sum / frames : 0;
    return take;
}
double db(double power) { return 10 * std::log10(std::max(power, 1e-12)); }
// Share of energy below 150 Hz, 150-600 Hz, 600-2500 Hz and above, from the energy under
// each cutoff (fourth-order low-passes), so the shares add up.
std::array<double, 4> bands(const std::vector<float> &stereo) {
    const double cutoffs[3]{150, 600, 2500};
    std::array<double, 4> under{};
    double total = 0;
    for (int k = 0; k < 3; k++) {
        auto a = Biquad::lowpass(cutoffs[k], 48000), b = a;
        for (size_t n = 0; n + 1 < stereo.size(); n += 2) {
            const float x = (stereo[n] + stereo[n + 1]) * .5f;
            const float y = b.process(a.process(x, 0), 0);
            under[size_t(k)] += y * y;
            if (k == 0) total += x * x;
        }
    }
    std::array<double, 4> e{under[0], under[1] - under[0], under[2] - under[1], total - under[2]};
    for (auto &v : e) v = total > 0 ? std::max(0., 100 * v / total) : 0;
    return e;
}
} // namespace
int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: zerochord-styles bank.sf2 output-directory [style-id]\n";
        return 2;
    }
    soundLibrary().load("/nonexistent", argv[1]);
    if (std::string(argv[2]) == "--levels") {
        for (int program = 0; program < 128; program++) {
            const float loudness = soundfontLoudness(soundLibrary().banks[0].data.get(), 0, program);
            Settings s;
            s.chordProgram = program;
            soundLibrary().prepareGains(s);
            std::printf("%3d %-24s %6.1f dB  gain %+5.1f dB\n", program, programName(program).c_str(),
                        20 * std::log10(std::max(loudness, 1e-6f)),
                        20 * std::log10(soundLibrary().gain("", program)));
        }
        return 0;
    }
    std::filesystem::create_directories(argv[2]);
    PresetLibrary styles;
    std::printf("%-7s %-6s %7s %7s  %5s %5s %5s %5s\n", "style", "layer", "rms", "peak", "<150",
                "-600", "-2.5k", ">2.5k");
    for (const auto &entry : styles.entries) {
        if (argc > 3 && entry.id != argv[3]) continue;
        ranges.fill({127, 0});
        auto full = play(argv[1], entry.sound, true);
        std::printf("%-7s notes: ", entry.id.c_str());
        static const char *parts[]{"chord", "front", "pad", "arp", "drums", "bass"};
        for (int p = 0; p < 6; p++)
            if (p != 4 && ranges[size_t(p)].first <= ranges[size_t(p)].second)
                std::printf("%s %s-%s  ", parts[p], (noteName(ranges[size_t(p)].first) + std::to_string(ranges[size_t(p)].first / 12 - 1)).c_str(),
                            (noteName(ranges[size_t(p)].second) + std::to_string(ranges[size_t(p)].second / 12 - 1)).c_str());
        std::printf("\n");
        writeWav((std::filesystem::path(argv[2]) / (entry.id + ".wav")).string(), full.samples);
        auto b = bands(full.samples);
        std::printf("%-7s %-6s %7.1f %7.1f  %5.0f %5.0f %5.0f %5.0f\n", entry.id.c_str(), "mix",
                    db(full.meanSquare), 20 * std::log10(std::max(full.peak, 1e-6f)), b[0], b[1],
                    b[2], b[3]);
        const char *names[]{"chord", "arp", "pad", "bass", "drums"};
        for (int n = 0; n < 6; n++) {
            Settings solo = entry.sound;
            if (std::getenv("ZEROCHORD_LAB_FULL")) { // Solos at full level, to set mixes.
                solo.layerMix.fill(100);
                solo.frontMix = 100;
            }
            if (n < 5) solo.setEnabledLayers(backingLayers(backingLayerOrder[size_t(n)]));
            else solo.setEnabledLayers(0);
            if (n < 5 && !entry.sound.audible(backingLayerOrder[size_t(n)])) continue;
            auto take = play(argv[1], solo, n == 5);
            auto e = bands(take.samples);
            std::printf("%-7s %-6s %7.1f %7.1f  %5.0f %5.0f %5.0f %5.0f\n", "", n < 5 ? names[n] : "front",
                        db(take.meanSquare), 20 * std::log10(std::max(take.peak, 1e-6f)), e[0], e[1],
                        e[2], e[3]);
        }
    }
}
