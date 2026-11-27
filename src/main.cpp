#include "audio.hpp"
#include "looper.hpp"
#include "midi_device.hpp"
#include "platform.hpp"
#include "settings_io.hpp"
#include "sounds.hpp"
#include "storage.hpp"
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
std::string dataDir; // ~/ZeroChord, or ZEROCHORD_HOME; chosen once at startup.
std::string configPath() { return (std::filesystem::path(dataDir) / "settings.conf").string(); }
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
        // Everything lives in ~/ZeroChord/; the first launch moves the old ~/.config/zerochord/
        // there. --no-save writes nothing, so it reads whichever folder holds the settings.
        dataDir = zc::dataDirectory();
        const std::string legacy = zc::legacyDirectory();
        if (!noSave) {
            std::string moved;
            dataDir = zc::migrateData(legacy, dataDir, moved);
            if (!moved.empty()) std::cerr << "ZeroChord: " << moved << "\n";
        } else if (!legacy.empty() && !std::filesystem::exists(configPath()) &&
                   std::filesystem::exists(std::filesystem::path(legacy) / "settings.conf"))
            dataDir = legacy;
        // Packs someone still copies to the old folder keep working.
        std::vector<std::string> oldSounds;
        if (!legacy.empty() && legacy != dataDir)
            oldSounds.push_back((std::filesystem::path(legacy) / "sounds").string());
        zc::soundLibrary().load(dataDir, bank, oldSounds);
        if (!zc::soundLibrary().warning.empty())
            std::cerr << zc::soundLibrary().warning;
        Silent silent;
        std::unique_ptr<zc::Audio> audio;
        if (!noAudio)
            audio = std::make_unique<zc::Audio>(bank);
        zc::Instrument instrument(audio ? static_cast<zc::Sink &>(*audio)
                                        : static_cast<zc::Sink &>(silent));
        zc::loadSettings(instrument.settings, configPath());
        instrument.library.load(dataDir, instrument.settings, !noSave);
        if (!instrument.library.warning.empty())
            std::cerr << instrument.library.warning;
        instrument.configure();
        // The loop lives beside the settings; --no-save leaves it untouched.
        zc::Looper looper(instrument, audio ? static_cast<zc::Sink &>(*audio)
                                            : static_cast<zc::Sink &>(silent));
        instrument.looper = &looper;
        looper.path = (std::filesystem::path(dataDir) / "loop.txt").string();
        looper.load();
        if (noSave) looper.path.clear();
        zc::UI ui(instrument, [&] { return platform.systemVolume(); },
                  [&](int percent) { return platform.setSystemVolume(percent); });
        const auto start = std::chrono::steady_clock::now();
        uint64_t previous = 0, escAt = 0;
        bool escDown = false, quit = false;
        // While playing, a Space tap stops (on release) and a hold switches All front.
        uint64_t spaceAt = 0;
        bool spaceDown = false, spaceHeld = false;
        constexpr uint64_t spaceHoldMs = 600;
        // Enter works the loop: a tap acts on release, a hold drops a take or asks to delete.
        uint64_t enterAt = 0;
        bool enterDown = false, enterHeld = false;
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
            if (key == ' ' && (spaceDown || (down && !ui.settingsOpen && !ui.tempoOpen()))) {
                if (down && !spaceDown) {
                    spaceDown = true, spaceHeld = false, spaceAt = now;
                } else if (!down && spaceDown) {
                    spaceDown = false;
                    if (!spaceHeld) {
                        instrument.key(' ', true, now);
                        instrument.key(' ', false, now);
                    }
                }
                return;
            }
            if (key == '\n' && (enterDown || (down && !ui.settingsOpen && !ui.tempoOpen() &&
                                               !ui.loopQuestion()))) {
                if (down && !enterDown) {
                    enterDown = true, enterHeld = false, enterAt = now;
                } else if (!down && enterDown) {
                    enterDown = false;
                    if (!enterHeld) looper.tap(enterAt, now);
                }
                return;
            }
            const bool shortcut = !instrument.allFront && (key == 'p' || key == 'o'); // Notes there.
            if (ui.settingsOpen || ui.tempoOpen() || ui.backingOpen() || ui.loopQuestion() ||
                shortcut) {
                const bool quickBacking = ui.backingOpen();
                ui.key(key, down);
                // Releases still reach the instrument while O's quick Backing page is open.
                if ((!ui.settingsOpen || quickBacking) && !down)
                    instrument.key(key, false, now);
            }
            else
                instrument.key(key, down, now);
        };
        uint64_t nextJackCheck = 0;
        zc::MidiInput midi;
        auto elapsed = [&] {
            return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - start)
                                .count());
        };
        // Keys and the backing pattern; run again before painting so a repaint delays neither.
        auto play = [&](uint64_t now) {
            if (!platform.poll([&](char key, bool down) { input(key, down, now); },
                               [&]() {
                                   instrument.panic();
                                   escDown = false;
                               }))
                quit = true;
            if (!midi.poll([&](const zc::MidiEvent &e) { instrument.midi(e, now); })) {
                instrument.midiReleaseAll(); // Unplugged: nothing may hang or stay bent.
                instrument.midiResetControllers();
                instrument.midiDevice = midi.name();
            }
            while (eventIndex < events.size() && events[eventIndex].at <= now) {
                auto e = events[eventIndex++];
                input(e.key, e.down, now);
            }
            instrument.tick(now);
        };
        while (!stopping && !quit) {
            const uint64_t now = elapsed();
            lv_tick_inc(uint32_t(now - previous));
            previous = now;
            play(now);
            if (escDown && now - escAt >= 800)
                quit = true;
            if (enterDown && !enterHeld && now - enterAt >= spaceHoldMs) {
                enterHeld = true;
                const auto state = looper.state();
                if (state == zc::LoopState::Armed || state == zc::LoopState::Recording)
                    looper.discard();
                else if (state != zc::LoopState::Empty)
                    ui.askDeleteLoop();
            }
            if (spaceDown && !spaceHeld && now - spaceAt >= spaceHoldMs && !ui.settingsOpen) {
                spaceHeld = true;
                instrument.setAllFront(!instrument.allFront);
            }
            if (now >= nextJackCheck) { // Auto output follows the headphone jack.
                instrument.setJack(platform.headphones());
                midi.scan(); // Keyboards may be plugged in at any time.
                instrument.midiDevice = midi.name();
                nextJackCheck = now + 1000;
            }
            ui.tick(now);
            ui.draw();
            play(elapsed());
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
