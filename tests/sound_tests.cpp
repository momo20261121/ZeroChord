#include "renderer.hpp"
#include "settings_editor.hpp"
#include "settings_io.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace zc;
namespace fs = std::filesystem;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x))                                                                                  \
            throw std::runtime_error(std::to_string(__LINE__) + ": " #x);                          \
    } while (0)
struct Capture : Sink {
    std::vector<Command> events;
    void send(const Command &c) override { events.push_back(c); }
};
void press(SettingsEditor &e, char key) {
    e.key(key, true);
    e.key(key, false);
}
void choose(SettingsEditor &e, int value) {
    CHECK(e.pickerOpen());
    auto groups = e.soundPicker() ? e.soundCategories().size() : 1;
    for (size_t group = 0; group < groups; group++) {
        for (size_t n = 0; n < e.choices().size(); n++) {
            if (e.choices()[e.highlighted()].value == value)
                return;
            press(e, 18);
        }
        if (e.soundPicker())
            press(e, 20);
    }
    throw std::runtime_error("Missing choice");
}
void row(SettingsEditor &e, int value) {
    const auto target = SettingsEditor::appField(value) ? SettingsEditor::Tab::App : SettingsEditor::Tab::Style;
    for (int n = 0; e.tab() != target && n < 3; ++n) press(e, 20);
    for (int n = 0; e.selected() != value && n < SettingsEditor::settingCount; n++)
        press(e, 18);
    CHECK(e.selected() == value);
}
void frontRow(SettingsEditor &e, int value) {
    for (int n = 0; e.selected() != value && n < 8; n++)
        press(e, 18);
    CHECK(e.selected() == value);
}
int main(int argc, char **argv) {
    if (argc != 2)
        return 2;
    auto dir = fs::temp_directory_path() /
               ("zerochord-sounds-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        auto &sounds = soundLibrary();
        CHECK(sounds.entries.size() == 14 && sounds.categories().size() == 4);
        for (const auto &sound : sounds.entries)
            CHECK(!sound.name.empty() && !sound.category.empty());
        CHECK(sounds.sampleRateLabel(0) == "Source rate unavailable");
        CHECK(soundfontSampleRates(nullptr, 0, 0).empty());
        auto pack = dir / "sounds/my-pack";
        fs::create_directories(pack);
        fs::create_symlink(fs::absolute(argv[1]), pack / "bank.sf2");
        std::ofstream(pack / "pack.toml")
            << "file='bank.sf2'\n[[sounds]]\nid='velvet'\nname='Velvet "
               "Pad'\ncategory='Pads'\nprogram=89\n[[sounds]]\nid='glass'\nname='Glass "
               "Harp'\ncategory='Custom'\nprogram=46\n";
        std::ofstream(dir / "sounds/invalid.sf2") << "not a SoundFont";
        fs::create_directories(dir / "sounds/broken");
        std::ofstream(dir / "sounds/broken/pack.toml") << "file='../outside.sf2'\n";
        sounds.load(dir.string(), argv[1]);
        CHECK(sounds.entries.size() == 16 && sounds.find("my-pack/velvet") &&
              sounds.categories().size() == 5);
        CHECK(sounds.sampleRateLabel(0) == "Source 22.05 kHz");
        CHECK(sounds.sampleRateLabel(48) == "Source 12 kHz");
        CHECK(sounds.sampleRateLabel(10) == "Source 22.321 kHz");
        CHECK(sounds.sampleRateLabel(8) == "Source 44.1 kHz");
        CHECK(sounds.sampleRateLabel(88) == "Source 22.05-44.1 kHz (mixed)");
        CHECK(sounds.sampleRateLabel(sounds.find("my-pack/velvet")->choice) == "Source 22.05 kHz");
        CHECK(sounds.sampleRateLabel(sounds.find("my-pack/glass")->choice) == "Source 44.1 kHz");
        CHECK(soundfontSampleRates(sounds.banks[0].data.get(), 999, 999).empty());
        {
            // New choices must exist in the actual bank, render across registers,
            // and be selectable through the existing category picker.
            const auto programs = soundfontPresets(sounds.banks[0].data.get());
            Capture captured;
            Instrument player(captured);
            SettingsEditor menu(player);
            press(menu, '\t');
            row(menu, 14);
            for (const char *id : {"steel-string-guitar", "clean-electric-guitar",
                                   "overdriven-guitar", "celesta"}) {
                const auto *sound = sounds.find(id);
                CHECK(sound && std::any_of(programs.begin(), programs.end(), [&](const auto &p) {
                    return p.bank == 0 && p.program == sound->program;
                }));
                press(menu, '\n');
                choose(menu, sound->choice);
                press(menu, '\n');
                CHECK(player.settings.harpProgram == sound->program);
                Renderer renderer(argv[1]);
                Command config{CommandType::Configure};
                config.settings.harpProgram = sound->program;
                config.settings.echo = 0;
                renderer.send(config);
                for (int pitch : {48, 60, 72}) {
                    renderer.send(Command{});
                    renderer.send({CommandType::On, 1, pitch, 500, Part::Harp});
                    std::vector<float> samples(48000);
                    renderer.render(samples.data(), 24000);
                    double energy = 0;
                    for (float sample : samples) {
                        CHECK(std::isfinite(sample) && std::abs(sample) < .99f);
                        energy += sample * sample;
                    }
                    CHECK(energy > .001);
                }
            }
        }
        CHECK(sounds.warning.find("Invalid SoundFont") != std::string::npos);
        int glass = sounds.find("my-pack/glass")->choice;
        CHECK(sounds.name(0, "my-pack/glass") == "Glass Harp");
        {
            // Custom pad/chord/front route through the loaded pack, with built-in fallback
            // retained.
            for (int part = 0; part < 3; part++) {
                Renderer custom(argv[1]), builtin(argv[1]);
                Command c{CommandType::Configure};
                c.settings.echo = 0;
                c.settings.mode = part == 0 ? PlaybackMode::Pad : PlaybackMode::Chord;
                c.settings.padProgram = c.settings.chordProgram = c.settings.harpProgram = 0;
                if (part == 0)
                    c.settings.padSound = "my-pack/velvet";
                if (part == 1)
                    c.settings.chordSound = "my-pack/velvet";
                if (part == 2)
                    c.settings.frontSound = "my-pack/glass";
                custom.send(c);
                c.settings.padSound.clear();
                c.settings.chordSound.clear();
                c.settings.frontSound.clear();
                if (part == 0)
                    c.settings.padProgram = 89;
                if (part == 1)
                    c.settings.chordProgram = 89;
                if (part == 2)
                    c.settings.harpProgram = 46;
                builtin.send(c);
                auto on =
                    Command{CommandType::On, 1, 60, 700, part == 2 ? Part::Harp : part == 0 ? Part::Pad : Part::Chord};
                custom.send(on);
                builtin.send(on);
                std::vector<float> a(96000), b(96000);
                custom.render(a.data(), 48000);
                builtin.render(b.data(), 48000);
                double energy = 0;
                for (size_t n = 0; n < a.size(); n++) {
                    CHECK(std::isfinite(a[n]) && std::abs(a[n] - b[n]) < 1e-6f);
                    energy += a[n] * a[n];
                }
                CHECK(energy > .001);
                custom.send(Command{});
                custom.render(a.data(), 48000);
                CHECK(std::all_of(a.begin(), a.end(), [](float v) { return v == 0; }));
            }
        }
        Capture sink;
        Instrument instrument(sink);
        SettingsEditor editor(instrument);
        press(editor, '\t');
        row(editor, 14);
        press(editor, '\n');
        CHECK(editor.soundPicker() && editor.soundCategory() == "Plucked");
        auto before = instrument.settings;
        choose(editor, glass);
        CHECK(instrument.settings.frontSound.empty());
        CHECK(std::any_of(sink.events.begin(), sink.events.end(), [](const Command &c) {
            return c.type == CommandType::On && c.part == Part::Harp;
        }));
        press(editor, 27);
        CHECK(instrument.settings.frontSound.empty());
        press(editor, '\n');
        choose(editor, glass);
        press(editor, '\n');
        CHECK(instrument.settings.frontSound == "my-pack/glass");
        press(editor, '\b');
        CHECK(instrument.settings.frontSound.empty());
        row(editor, 22);
        press(editor, '\n');
        CHECK(editor.page() == SettingsEditor::Page::FrontNotes && editor.frontKey() == -1);
        for (char key : std::string("vbnm12345678"))
            press(editor, key);
        CHECK(editor.frontKey() == -1 && editor.selected() == 0);
        press(editor, 20); // All -> Z
        press(editor, 20); // Z -> X
        CHECK(editor.frontKey() == 1);
        press(editor, '\n');
        CHECK(editor.choices()[editor.highlighted()].name.find("+4") != std::string::npos);
        auto highlight = editor.highlighted();
        for (char key : std::string("vbnm"))
            press(editor, key);
        CHECK(editor.highlighted() == highlight);
        choose(editor, 16 + 7); // Fixed fifth replaces the automatic third.
        press(editor, '\n');
        CHECK(instrument.settings.frontSource[1] == 4 && instrument.settings.frontOffset[1] == 7);
        frontRow(editor, 1);
        press(editor, '\n');
        choose(editor, 5);
        press(editor, '\n');
        CHECK(instrument.settings.frontOctave[1] == 5);
        press(editor, '\n');
        choose(editor, 3);
        press(editor, 27);
        CHECK(instrument.settings.frontOctave[1] == 5);
        frontRow(editor, 2);
        press(editor, '\n');
        choose(editor, 1);
        press(editor, 27);
        CHECK(instrument.settings.frontSource[1] == 4);
        press(editor, '\n');
        choose(editor, 1);
        press(editor, '\n');
        CHECK(instrument.settings.frontSource[1] == -1 && instrument.settings.frontOctave[1] == -2);
        press(editor, 19);
        press(editor, 19); // X -> Z -> All
        CHECK(editor.frontKey() == -1);
        frontRow(editor, 2);
        press(editor, '\n');
        choose(editor, 90);
        press(editor, '\n');
        CHECK(instrument.settings.strumMs == 90);
        frontRow(editor, 0);
        press(editor, '\n');
        choose(editor, 1);
        press(editor, '\n');
        CHECK(instrument.settings.frontMode == 1 && instrument.settings.frontAction[1] == 1);
        frontRow(editor, 1);
        press(editor, '\n');
        choose(editor, 0);
        press(editor, '\n');
        frontRow(editor, 2);
        press(editor, '\n');
        choose(editor, 330);
        press(editor, '\n');
        CHECK(instrument.settings.scaleStrumMs == 330 && instrument.settings.strumMs == 90);
        press(editor, 20); // Z in scale mode
        press(editor, '\n');
        choose(editor, 4);
        press(editor, '\n');
        frontRow(editor, 1);
        press(editor, '\n');
        choose(editor, 3);
        press(editor, '\n');
        CHECK(instrument.settings.frontDegree[0] == 4 && instrument.settings.scaleOctave[0] == 3);
        // All reset is confirmed and restores both modes without touching sound.
        press(editor, 19);
        frontRow(editor, 3);
        press(editor, '\n');
        choose(editor, 1);
        press(editor, '\n');
        CHECK(instrument.settings.frontMode == 0 && instrument.settings.frontDegree[0] == 0 &&
              instrument.settings.scaleOctave[0] == -2 && instrument.settings.scaleStrumMs == 210);
        Settings s;
        for (int quality = 0; quality < qualityCount; quality++)
            for (int tonic = 0; tonic < 12; tonic++) {
                Chord c{48 + tonic, -1, 6, Quality(quality)};
                CHECK(frontNotes(c, s) == harpNotes(c));
            }
        s.frontSource[0] = 1; // Chord-aware third changes with the harmony.
        CHECK(frontNotes({48, -1, 6, Quality::Major}, s)[0] == 64);
        CHECK(frontNotes({48, -1, 6, Quality::Minor}, s)[0] == 63);
        s.frontSource[0] = 4;
        s.frontOffset[0] = 4;
        CHECK(frontNotes({48, -1, 6, Quality::Minor}, s)[0] == 64);
        s.frontOctave[0] = 3;
        CHECK(frontNotes({48, -1, 6, Quality::Minor}, s)[0] == 52); // E3, actual octave.
        s.mode = PlaybackMode::FrontOnly;
        instrument.settings = s;
        instrument.configure();
        instrument.key('1', true, 9);
        instrument.key('8', true, 10);
        instrument.key('8', false, 11);
        CHECK(instrument.chord.root == 49); // Sharp chord also moves chord-based front notes.
        auto expected = instrument.playingFrontNotes();
        auto unshifted = frontNotes(instrument.chord, s);
        for (int n = 0; n < 7; n++) CHECK(expected[n] == unshifted[n] + 1);
        sink.events.clear();
        const char *keys = "zxcvbnm";
        for (int n = 0; n < 7; n++) {
            instrument.key(keys[n], true, 20 + n * 2);
            instrument.key(keys[n], false, 21 + n * 2);
        }
        std::vector<int> played;
        for (const auto &c : sink.events)
            if (c.type == CommandType::On && c.part == Part::Harp)
                played.push_back(c.pitch);
        CHECK(played == std::vector<int>(expected.begin(), expected.end()));
        for (char key : {'l', 'k'}) {
            sink.events.clear();
            instrument.key(key, true, 1000);
            instrument.key(key, false, 1001);
            instrument.tick(2000);
            played.clear();
            for (const auto &c : sink.events)
                if (c.type == CommandType::On && c.part == Part::Harp)
                    played.push_back(c.pitch);
            auto order = std::vector<int>(expected.begin(), expected.end());
            if (key == 'k')
                std::reverse(order.begin(), order.end());
            CHECK(played == order);
        }
        // Octave shifts emit no chord or harp commands, and leave backing harmony intact.
        instrument.settings.frontAction[0] = 1;
        sink.events.clear();
        auto chordBefore = instrument.chord;
        instrument.key('l', true, 2100);
        instrument.key('l', false, 2101);
        CHECK(sink.events.empty() && instrument.chord == chordBefore);
        auto raised = instrument.playingFrontNotes();
        for (int n = 0; n < 7; n++)
            CHECK(raised[n] == expected[n] + 12);
        instrument.key('k', true, 2102);
        instrument.key('k', false, 2103);
        CHECK(instrument.playingFrontNotes() == expected);
        instrument.key('1', false, 2104);
        CHECK(instrument.chord.root == 49 && instrument.playingFrontNotes() == unshifted);
        // Scale keyboard follows the selected key, not the chord root/quality/sharp.
        s = Settings{};
        s.frontMode = 1;
        const int intervals[2][7]{{0, 2, 4, 5, 7, 9, 11}, {0, 2, 3, 5, 7, 8, 10}};
        for (int tonic = 0; tonic < 12; tonic++)
            for (int scale = 0; scale < 2; scale++) {
                s.tonic = tonic;
                s.scale = scale;
                auto notes = frontNotes({55, -1, 3, Quality::Minor}, s);
                for (int n = 0; n < 7; n++)
                    CHECK(notes[n] == 60 + tonic + intervals[scale][n]);
                CHECK(notes == frontNotes({49, -1, 6, Quality::Seventh}, s));
            }
        s.tonic = 0;
        s.scale = 0;
        s.frontDegree[0] = 4;
        s.scaleOctave[0] = 3;
        CHECK(frontNotes({}, s)[0] == 55); // G3.
        instrument.panic();
        instrument.settings = s;
        sink.events.clear();
        instrument.key('z', true, 3000);
        instrument.key('z', false, 3001);
        CHECK(!instrument.active && sink.events.back().pitch == 55); // Works without backing.
        instrument.key('l', true, 3002);
        instrument.key('l', false, 3003);
        CHECK(instrument.playingFrontNotes()[0] == 67);
        for (int n = 0; n < 20; n++) {
            instrument.key('l', true, 3010 + n * 2);
            instrument.key('l', false, 3011 + n * 2);
        }
        auto bounded = instrument.playingFrontNotes();
        CHECK(*std::max_element(bounded.begin(), bounded.end()) <= 127);
        // Scale mode also supports sweeps, using its own duration.
        instrument.frontOctaveShift.fill(0);
        instrument.settings.frontAction[1] = 0;
        instrument.settings.scaleStrumMs = 600;
        sink.events.clear();
        instrument.key('l', true, 4000);
        instrument.key('l', false, 4001);
        instrument.tick(4000);
        CHECK(sink.events.size() == 1);
        instrument.tick(4599);
        CHECK(sink.events.size() == 6);
        instrument.tick(4600);
        CHECK(sink.events.size() == 7);
        s.frontSound = "my-pack/glass";
        s.padSound = "my-pack/velvet";
        s.frontAction = {1, 0};
        s.strumMs = 90;
        s.scaleStrumMs = 330;
        s.combinationMs = 45;
        saveSettings(s, (dir / "settings.conf").string());
        Settings loaded;
        loadSettings(loaded, (dir / "settings.conf").string());
        CHECK(loaded.frontSound == s.frontSound && loaded.frontSource == s.frontSource &&
              loaded.frontOffset == s.frontOffset && loaded.frontOctave == s.frontOctave &&
              loaded.frontMode == s.frontMode && loaded.frontAction == s.frontAction &&
              loaded.frontDegree == s.frontDegree && loaded.scaleOctave == s.scaleOctave &&
              loaded.strumMs == s.strumMs && loaded.scaleStrumMs == s.scaleStrumMs &&
              loaded.combinationMs == s.combinationMs);
        {
            auto legacy = (dir / "legacy.conf").string();
            std::ofstream(legacy) << "frontRegister 0\nfrontLow 0\nfrontHigh 127\n";
            Settings migrated;
            loadSettings(migrated, legacy);
            CHECK(migrated.frontOctave == Settings{}.frontOctave);
            Chord c{48, -1, 6, Quality::Major};
            CHECK(frontNotes(c, migrated) == harpNotes(c));
            // Preserve old fixed offsets, octave offsets, and clamped range pitches.
            std::ofstream(legacy) << "frontSource1 4\nfrontOffset1 -5\nfrontOctave1 1\n"
                                    "frontLow 60\nfrontHigh 80\n";
            migrated = Settings{};
            loadSettings(migrated, legacy);
            CHECK(frontNotes(c, migrated)[1] == 67);
            CHECK(frontNotes(c, migrated)[6] == 80);
            CHECK(migrated.frontSource[6] == 4 && migrated.frontOctave[6] == 5);
        }
        PresetDefinition definition{"custom", "Custom", s, false};
        writePreset(definition, (dir / "custom.toml").string());
        CHECK(samePresetSettings(s, readPreset((dir / "custom.toml").string()).sound));
        auto retained = s;
        PresetLibrary presets;
        presets.select(s, 1);
        presets.restore(s);
        CHECK(s.frontSource == retained.frontSource && s.frontMode == retained.frontMode &&
              s.frontDegree == retained.frontDegree && s.frontAction == retained.frontAction &&
              s.combinationMs == retained.combinationMs);
        // Reordering/removing a pack never replaces a saved ID with a different sound.
        sounds.load(dir.string(), argv[1]);
        CHECK(sounds.find("my-pack/glass") && sounds.find("my-pack/glass")->choice == glass);
        fs::create_symlink(fs::absolute(argv[1]), dir / "sounds/aaa.sf2");
        sounds.load(dir.string(), argv[1]);
        CHECK(sounds.find("my-pack/glass")->choice != glass);
        CHECK(sounds.name(0, retained.frontSound) == "Glass Harp");
        fs::remove(dir / "sounds/aaa.sf2");
        fs::remove_all(pack);
        sounds.load(dir.string(), argv[1]);
        CHECK(!sounds.find("my-pack/glass"));
        {
            Renderer missing(argv[1]), fallback(argv[1]);
            Command c{CommandType::Configure};
            c.settings.echo = 0;
            c.settings.harpProgram = 0;
            c.settings.frontSound = retained.frontSound;
            missing.send(c);
            c.settings.frontSound.clear();
            fallback.send(c);
            missing.send({CommandType::On, 1, 60, 700, Part::Harp});
            fallback.send({CommandType::On, 1, 60, 700, Part::Harp});
            std::vector<float> a(9600), b(9600);
            missing.render(a.data(), 4800);
            fallback.render(b.data(), 4800);
            CHECK(a == b);
        }
        fs::create_symlink(fs::absolute(argv[1]), dir / "sounds/direct.sf2");
        sounds.load(dir.string(), argv[1]);
        CHECK(sounds.find("direct/bank-0-program-0"));
        CHECK(sounds.find("direct/bank-0-program-0")->name != "");
        sounds = SoundLibrary{};
        fs::remove_all(dir);
        std::cout << "PASS: named categories, pack validation/routing/fallback, sound IDs, front "
                     "editing/persistence and mapped plucks/sweeps\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        soundLibrary() = SoundLibrary{};
        fs::remove_all(dir);
        return 1;
    }
}
