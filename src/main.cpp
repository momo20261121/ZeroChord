#include "audio.hpp"
#include "platform.hpp"
#include "settings_io.hpp"
#include "sounds.hpp"
#include "ui.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
namespace {
volatile std::sig_atomic_t stopping = 0;
void signalHandler(int) { stopping = 1; }
struct Silent : zc::Sink {
    void send(const zc::Command &) override {}
};
std::string configPath() {
    if (const char *xdg = std::getenv("XDG_CONFIG_HOME"))
        return std::string(xdg) + "/zerochord/settings.conf";
    if (const char *home = std::getenv("HOME"))
        return std::string(home) + "/.config/zerochord/settings.conf";
    return "/tmp/zerochord-settings.conf";
}
} // namespace
int main(int argc, char **argv) {
    try {
        bool headless = false, noAudio = false, noSave = false;
        uint64_t quitAfter = 0;
        std::string bank, capture, script;
        for (int n = 1; n < argc; n++) {
            std::string arg = argv[n];
            auto value = [&]() {
                if (n + 1 >= argc)
                    throw std::runtime_error("Missing value for " + arg);
                return std::string(argv[++n]);
            };
            if (arg == "--headless")
                headless = true;
            else if (arg == "--no-audio")
                noAudio = true;
            else if (arg == "--no-save")
                noSave = true;
            else if (arg == "--soundfont")
                bank = value();
            else if (arg == "--capture")
                capture = value();
            else if (arg == "--script")
                script = value();
            else if (arg == "--quit-after-ms")
                quitAfter = std::stoull(value());
            else if (arg == "--version") {
                std::cout << "ZeroChord " ZEROCHORD_VERSION "\n";
                return 0;
            } else if (arg == "--help") {
                std::cout << "ZeroChord " ZEROCHORD_VERSION "\nOptions: --version --soundfont PATH --headless --no-audio "
                             "--no-save --script PATH --capture PPM --quit-after-ms N\n";
                return 0;
            } else
                throw std::runtime_error("Unknown option: " + arg);
        }
        if (bank.empty())
            if (const char *env = std::getenv("ZEROCHORD_SOUNDFONT"))
                bank = env;
        if (bank.empty())
            for (const char *path :
                 {"/usr/share/sounds/sf2/TimGM6mb.sf2", ".tools/soundfonts/TimGM6mb.sf2"})
                if (std::filesystem::exists(path)) {
                    bank = path;
                    break;
                }
        if (bank.empty() && !noAudio)
            throw std::runtime_error("Install timgm6mb-soundfont or set ZEROCHORD_SOUNDFONT");
        struct Event {
            uint64_t at;
            char key;
            bool down;
        };
        std::vector<Event> events;
        if (!script.empty()) {
            std::ifstream in(script);
            if (!in)
                throw std::runtime_error("Cannot open event script");
            uint64_t at;
            std::string key, action;
            while (in >> at >> key >> action) {
                char code = key == "TAB"         ? '\t'
                            : key == "ESC"       ? 27
                            : key == "SPACE"     ? ' '
                            : key == "ENTER"     ? '\n'
                            : key == "BACKSPACE" ? '\b'
                            : key == "UP"        ? 17
                            : key == "DOWN"      ? 18
                            : key == "LEFT"      ? 19
                            : key == "RIGHT"     ? 20
                                                 : key.at(0);
                if (action != "down" && action != "up")
                    throw std::runtime_error("Script actions must be down/up");
                events.push_back({at, code, action == "down"});
            }
            if (!std::is_sorted(events.begin(), events.end(),
                                [](auto a, auto b) { return a.at < b.at; }))
                throw std::runtime_error("Script events must be time ordered");
        }
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        lv_init();
        zc::Platform platform(headless);
        zc::soundLibrary().load(std::filesystem::path(configPath()).parent_path().string(), bank);
        if (!zc::soundLibrary().warning.empty())
            std::cerr << zc::soundLibrary().warning;
        Silent silent;
        std::unique_ptr<zc::Audio> audio;
        if (!noAudio)
            audio = std::make_unique<zc::Audio>(bank);
        zc::Instrument instrument(audio ? static_cast<zc::Sink &>(*audio)
                                        : static_cast<zc::Sink &>(silent));
        zc::loadSettings(instrument.settings, configPath());
        instrument.library.load(std::filesystem::path(configPath()).parent_path().string(),
                                instrument.settings, !noSave);
        if (!instrument.library.warning.empty())
            std::cerr << instrument.library.warning;
        instrument.configure();
        zc::UI ui(instrument, [&] { return platform.systemVolume(); },
                  [&](int percent) { return platform.setSystemVolume(percent); });
        const auto start = std::chrono::steady_clock::now();
        uint64_t previous = 0, escAt = 0;
        bool escDown = false, quit = false;
        size_t eventIndex = 0;
        auto input = [&](char key, bool down, uint64_t now) {
            if (key == 27) {
                if (down && !escDown) {
                    escAt = now;
                    escDown = true;
                }
                if (!down && escDown) {
                    if (now - escAt < 800)
                        ui.key(key, true);
                    escDown = false;
                }
                return;
            }
            if (key == '\t') {
                ui.key(key, down);
                return;
            }
            if (ui.settingsOpen || ui.tempoOpen() || ui.backingOpen() || key == 'p' || key == 'o') {
                const bool quickBacking = ui.backingOpen();
                ui.key(key, down);
                // Releases still reach the instrument while O's quick Backing page is open.
                if ((!ui.settingsOpen || quickBacking) && !down)
                    instrument.key(key, false, now);
            }
            else
                instrument.key(key, down, now);
        };
        while (!stopping && !quit) {
            uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();
            lv_tick_inc(uint32_t(now - previous));
            previous = now;
            quit = !platform.poll([&](char key, bool down) { input(key, down, now); },
                                  [&]() {
                                      instrument.panic();
                                      escDown = false;
                                  });
            while (eventIndex < events.size() && events[eventIndex].at <= now) {
                auto e = events[eventIndex++];
                input(e.key, e.down, now);
            }
            if (escDown && now - escAt >= 800)
                quit = true;
            instrument.tick(now);
            ui.tick(now);
            ui.draw();
            lv_timer_handler();
            if (quitAfter && now >= quitAfter)
                quit = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        if (!capture.empty()) {
            lv_refr_now(nullptr);
            platform.capture(capture);
        }
        ui.cancelPending();
        instrument.panic();
        if (!noSave) {
            instrument.library.save(instrument.settings);
            zc::saveSettings(instrument.settings, configPath());
        }
        if (audio)
            std::cerr << "Audio callbacks=" << audio->callbacks()
                      << " queue_overflows=" << audio->overflows() << "\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "ZeroChord: " << e.what() << "\n";
        return 1;
    }
}
