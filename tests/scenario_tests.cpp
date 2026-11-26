#include "settings_editor.hpp"
#include "menu_navigation.hpp"
#include "settings_io.hpp"
#include "patterns.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
using namespace zc;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(std::to_string(__LINE__) + ": " #x); } while (0)
struct CheckedSink : Sink {
    uint64_t ons = 0;
    std::vector<Command> recent;
    void send(const Command &c) override {
        if (c.type == CommandType::On) {
            CHECK(c.pitch >= 0 && c.pitch <= 127);
            CHECK(c.durationMs >= 0 && c.durationMs <= 11000);
            CHECK(std::isfinite(c.gain) && c.gain >= 0 && c.gain <= 1);
            ++ons;
        }
        if (recent.size() == 128) recent.erase(recent.begin());
        recent.push_back(c);
    }
};
struct DeviceMenu : SettingsEditor {
    using SettingsEditor::SettingsEditor;
    void tap(char k) { key(k, true); key(k, false); }
    void row(int field) {
        zc::test::goToRow(*this, field);
        CHECK(selected() == field);
    }
    void pick(int value) {
        CHECK(pickerOpen());
        for (size_t n = 0; n < choices().size(); n++) {
            if (choices()[highlighted()].value == value) { tap('\n'); return; }
            tap('x');
        }
        throw std::runtime_error("Missing picker value");
    }
    void valid() {
        if (tempoOpen()) { CHECK(tempoText().size() <= 3); return; }
        if (page() == Page::Setup && tab() == Tab::About) {
            CHECK(rows().empty() && selected() == -1 && !pickerOpen());
            return;
        }
        std::vector<int> list = settingListPage() ? rows() : page() == Page::Combinations ? combinationRows() : page() == Page::FrontNotes ? frontRows() : std::vector<int>{0,1,2,3,4};
        CHECK(std::find(list.begin(), list.end(), selected()) != list.end());
        if (pickerOpen()) { CHECK(!choices().empty()); CHECK(highlighted() >= 0 && highlighted() < int(choices().size())); }
    }
};
int main() {
    try {
        // An upper-register root audition must never emit an invalid MIDI note.
        CheckedSink sink; Instrument player(sink); DeviceMenu menu(player);
        player.library.writable = false;
        player.settings.chordOctave = 7; player.settings.degreeChordRoot[6] = 11;
        menu.tap('\t'); menu.row(3); menu.tap('\n'); menu.tap('\n');
        menu.tap('x'); menu.tap(27); menu.tap(27);
        // Front volume must audition the front layer, even in Front only mode.
        player.settings.mode = PlaybackMode::FrontOnly;
        menu.row(SettingsEditor::volumeEffectsField); menu.tap('\n');
        menu.row(5); sink.recent.clear(); menu.tap('z');
        CHECK(std::any_of(sink.recent.begin(), sink.recent.end(), [](const Command &c) { return c.type == CommandType::On && c.part == Part::Harp; }));
        CHECK(std::none_of(sink.recent.begin(), sink.recent.end(), [](const Command &c) { return c.type == CommandType::On && c.part == Part::Chord; }));
        menu.row(4); sink.recent.clear(); menu.tap('z');
        CHECK(std::any_of(sink.recent.begin(), sink.recent.end(), [](const Command &c) { return c.type == CommandType::On && c.part == Part::Chord; }));
        menu.tap(27); menu.tap(27);
        // Every pattern with held/released combinations and simultaneous foreground playing.
        uint64_t now = 10000; unsigned scenarios = 0;
        const char *buttons[]{"2345678", "qwertyu", "asdfghj"};
        for (int preset = 0; preset < 9; preset++)
            for (auto mode : {PlaybackMode::Pad, PlaybackMode::Chord, PlaybackMode::Arp, PlaybackMode::FrontOnly})
                for (int pattern = 0; pattern < int(patterns(mode).size()); pattern++)
                    for (int quality = 0; quality < qualityCount; quality++) {
                        ++scenarios; player.panic(); player.library.select(player.settings, preset);
                        player.settings.mode = mode; setPatternIndex(player.settings, pattern);
                        player.settings.tonic = quality % 12;
                        player.settings.chordOctave = quality % 9 - 1;
                        player.settings.degreeChordRoot.fill(-1);
                        player.settings.combinations[0] = quality;
                        player.settings.hold = quality % 2;
                        player.settings.bpm = quality % 2 ? 240 : 40;
                        player.settings.combinationMs = quality % 3 ? 30 : 0;
                        player.configure();
                        int col = quality % 7; now += 10000;
                        player.key('1', true, now);
                        player.key(buttons[0][col], true, now + 1);
                        player.key(buttons[1][col], true, now + 18);
                        for (int step = 0; step < 500; step += 5) {
                            player.tick(now + 20 + step);
                            if (step % 50 == 0) {
                                char key = "zxcvbnm"[(step / 50) % 7];
                                player.key(key, true, now + 20 + step);
                                player.key(key, false, now + 21 + step);
                            }
                        }
                        player.key(buttons[0][col], false, now + 600);
                        player.key(buttons[1][col], false, now + 601);
                        player.tick(now + 1000);
                        CHECK(player.active == bool(player.settings.hold));
                        player.panic(); auto attacks = sink.ons; player.tick(now + 2000);
                        CHECK(sink.ons == attacks && !player.active);
                    }
        // Rolled playing, repeated key-up/down, sweeps and preset/mode switches in mixed orders.
        std::mt19937 playing(0x43686f72);
        std::string playingKeys = "12345678qwertyuasdfghjzxcvbnmkliop90 ";
        player.settings = Settings{}; player.configure();
        for (int step = 0; step < 60000; step++) {
            player.key(playingKeys[playing() % playingKeys.size()], playing() % 2, now + 3000 + step * 3);
            player.tick(now + 3000 + step * 3);
            if (step % 997 == 0) player.panic();
        }
        player.panic(); now += 200000;
        // Random physical-key navigation after arbitrary prior edits, plus menu auditions.
        player.settings = Settings{}; player.configure();
        std::mt19937 random(0x5a65726f);
        std::string keys = "zxcf\t\n\b\x1b 12345678p90";
        for (int step = 0; step < 40000; step++) {
            menu.tap(keys[random() % keys.size()]);
            menu.valid(); player.tick(now + 3000 + step * 7);
        }
        menu.cancelPending(); player.panic();
        // Fresh versus saved app-volume behavior, without altering user data.
        auto dir = std::filesystem::temp_directory_path() / "zerochord-scenario-volumes";
        auto path = (dir / "settings.conf").string();
        Settings fresh; CHECK(fresh.chordVolume == 80 && fresh.harpVolume == 100);
        fresh.chordVolume = 37; fresh.harpVolume = 62; saveSettings(fresh, path);
        Settings reload; loadSettings(reload, path);
        CHECK(reload.chordVolume == 37 && reload.harpVolume == 62);
        std::filesystem::remove_all(dir);
        std::cout << "PASS: " << scenarios << " performance scenarios, 60000 mixed playing events, 40000 physical menu actions, MIDI bounds, volume auditions and persistence\n";
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
