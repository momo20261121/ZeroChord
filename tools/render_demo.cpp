#include "renderer.hpp"
#include <fstream>
#include <iostream>
#include <vector>
using namespace zc;
struct Offline : Sink {
    Renderer renderer;
    explicit Offline(const char *path) : renderer(path) {}
    void send(const Command &c) override { renderer.send(c); }
};
static void little(std::ostream &out, unsigned value, int bytes) {
    for (int i = 0; i < bytes; i++)
        out.put(char((value >> (8 * i)) & 255));
}
int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: zerochord-render bank.sf2 demo.wav\n";
        return 2;
    }
    Offline sink(argv[1]);
    Instrument instrument(sink);
    instrument.configure();
    constexpr int rate = 48000, seconds = 12, frames = rate * seconds;
    std::ofstream out(argv[2], std::ios::binary);
    out << "RIFF";
    little(out, 36 + frames * 4, 4);
    out << "WAVEfmt ";
    little(out, 16, 4);
    little(out, 1, 2);
    little(out, 2, 2);
    little(out, rate, 4);
    little(out, rate * 4, 4);
    little(out, 4, 2);
    little(out, 16, 2);
    out << "data";
    little(out, frames * 4, 4);
    const char progression[] = {'8', '5', 't', '4'};
    std::vector<float> block(480 * 2);
    for (int tick = 0; tick < seconds * 100; tick++) {
        uint64_t now = uint64_t(tick) * 10;
        if (tick % 250 == 0 && tick < 1000) {
            char key = progression[tick / 250];
            instrument.key(key, true, now);
            instrument.key(key, false, now);
            instrument.key('l', true, now + 1);
            instrument.key('l', false, now + 2);
        }
        if (tick == 1000)
            instrument.panic();
        instrument.tick(now);
        sink.renderer.render(block.data(), 480);
        for (float sample : block)
            little(out, unsigned(int16_t(sample * 32767)), 2);
    }
    std::cout << "Rendered I-V-vi-IV, ascending strums, and final panic to " << argv[2] << "\n";
}
