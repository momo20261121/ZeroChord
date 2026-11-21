#include "presets.hpp"
#include "settings_editor.hpp"
#include "settings_io.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x))                                                                                  \
            throw std::runtime_error(std::to_string(__LINE__) + ": " #x);                          \
    } while (0)
using namespace zc;
namespace fs = std::filesystem;
struct Capture : Sink {
    std::vector<Command> events;
    void send(const Command &c) override { events.push_back(c); }
};
void press(SettingsEditor &e, char k) {
    e.key(k, true);
    e.key(k, false);
}
void row(SettingsEditor &e, int r) {
    while (e.selected() != r)
        press(e, 18);
}
void choose(SettingsEditor &e, int value) {
    for (size_t n = 0; n < e.choices().size(); n++) {
        if (e.choices()[e.highlighted()].value == value)
            return;
        press(e, 18);
    }
    throw std::runtime_error("Choice missing");
}
int main() {
    auto dir = fs::temp_directory_path() /
               ("zerochord-presets-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        Settings legacy;
        legacy.tonic = 2;
        legacy.scale = 1;
        legacy.preset = 1;
        legacy.bpm = 87;
        legacy.combinations[0] = int(Quality::No3);
        legacy.degreeCombinations[3][3] = int(Quality::Sus4);
        legacy.masterVolume = 47;
        legacy.theme = 1;
        PresetLibrary lib;
        lib.load(dir.string(), legacy);
        CHECK(lib.entries.size() == 9 && lib.modified(legacy));
        CHECK(legacy.chordProgram == 89 && legacy.harpProgram == 10);
        CHECK(fs::exists(dir / "presets/factory/pop.toml"));
        auto dream = legacy;
        lib.select(legacy, 4);
        CHECK(legacy.bpm == 112 && legacy.tonic == 0 && legacy.mode == PlaybackMode::Arp);
        CHECK(legacy.masterVolume == 47 && legacy.theme == 1);
        legacy.bpm = 123;
        legacy.chordProgram = 2;
        legacy.presetLevel = 76;
        lib.select(legacy, 1);
        CHECK(samePresetSettings(dream, legacy));
        lib.select(legacy, 4);
        CHECK(legacy.bpm == 123 && legacy.chordProgram == 2 && legacy.presetLevel == 76);
        lib.save(legacy);
        saveSettings(legacy, (dir / "settings.conf").string());
        Settings loaded;
        loadSettings(loaded, (dir / "settings.conf").string());
        PresetLibrary reloaded;
        reloaded.load(dir.string(), loaded);
        CHECK(samePresetSettings(legacy, loaded) && loaded.masterVolume == 47 && loaded.theme == 1);
        reloaded.select(loaded, 1);
        CHECK(samePresetSettings(dream, loaded));
        reloaded.restore(loaded);
        CHECK(!reloaded.modified(loaded));
        CHECK(loaded.bpm == 100 && loaded.tonic == 0 &&
              loaded.combinations[0] == int(Quality::Diminished));
        CHECK(loaded.masterVolume == 47 && loaded.theme == 1);
        loaded.bpm = 135;
        loaded.mode = PlaybackMode::Chord;
        loaded.chordPattern = 3;
        loaded.padMotion = 2;
        loaded.hold = 0;
        loaded.echo = 31;
        loaded.degreeCombinations[0][1] = int(Quality::Sus2);
        auto before = loaded;
        reloaded.saveAs(loaded);
        CHECK(reloaded.current(loaded).name == "User001" && !reloaded.modified(loaded));
        auto user = readPreset((dir / "presets/user/user-001.toml").string());
        CHECK(samePresetSettings(before, user.sound));
        reloaded.saveAs(loaded);
        CHECK(reloaded.current(loaded).id == "user-002");
        reloaded.save(loaded);
        PresetLibrary third;
        Settings fresh;
        fresh.masterVolume = 39;
        third.load(dir.string(), fresh);
        CHECK(third.current(fresh).id == "user-002" && fresh.bpm == 135 &&
              fresh.masterVolume == 39);
        // Each musical field can reset independently; picker reset remains transactional.
        Capture sink;
        Instrument instrument(sink);
        instrument.library.load(dir.string(), instrument.settings);
        SettingsEditor editor(instrument);
        press(editor, '\t');
        instrument.settings.tonic = 7;
        instrument.settings.bpm = 211;
        row(editor, 0);
        press(editor, '\b');
        CHECK(instrument.settings.tonic == 0 && instrument.settings.bpm == 211);
        row(editor, 8);
        press(editor, '\n');
        press(editor, '\b');
        CHECK(instrument.settings.bpm == 211);
        CHECK(editor.choices()[editor.highlighted()].value == 135);
        press(editor, 27);
        CHECK(instrument.settings.bpm == 211);
        press(editor, '\n');
        press(editor, '\b');
        press(editor, '\n');
        CHECK(instrument.settings.bpm == 135);
        row(editor, 10);
        press(editor, 20);
        CHECK(instrument.settings.mode == PlaybackMode::Arp);
        press(editor, '\b');
        CHECK(instrument.settings.mode == PlaybackMode::Chord);
        row(editor, 3);
        press(editor, '\n');
        press(editor, '2');
        press(editor, 18);
        press(editor, 20);
        CHECK(instrument.settings.degreeCombinations[0][1] != int(Quality::Sus2));
        press(editor, '\b');
        CHECK(instrument.settings.degreeCombinations[0][1] == int(Quality::Sus2));
        press(editor, 27);
        row(editor, 17);
        instrument.settings.masterVolume = 17;
        press(editor, '\b');
        CHECK(instrument.settings.masterVolume == 100);
        row(editor, 20);
        instrument.settings.tonic = 9;
        press(editor, '\n');
        choose(editor, 1);
        press(editor, 27);
        CHECK(instrument.settings.tonic == 9);
        press(editor, '\n');
        choose(editor, 1);
        press(editor, '\n');
        CHECK(instrument.settings.tonic == 0 && instrument.settings.masterVolume == 100);
        // Factory TOML edits define restore defaults without replacing remembered working values.
        auto definition = readPreset((dir / "presets/factory/pop.toml").string());
        definition.sound.bpm = 119;
        writePreset(definition, (dir / "presets/factory/pop.toml").string());
        PresetLibrary edited;
        Settings editedSettings;
        edited.load(dir.string(), editedSettings);
        edited.select(editedSettings, 4);
        CHECK(editedSettings.bpm == 123);
        edited.restore(editedSettings);
        CHECK(editedSettings.bpm == 119);
        // Malformed, unsafe, mistyped and unsupported user files fail closed, with a warning.
        const std::vector<std::string> bad = {
            "schema_version=9\nid='x'\nname='X'\n",
            "schema_version=1\nid='../x'\nname='X'\n",
            "schema_version=1\nid='x'\nname='X'\nbpm=1000\n",
            "schema_version=1\nid='x'\nname='X'\nbpm=100.5\n",
            "schema_version=1\nid='x'\nname='X'\narp_pattern='missing'\n",
            "schema_version=1\nid='x'\nname='X'\ncombinations=['wrong']\n",
            "not valid TOML"};
        for (const auto &text : bad) {
            std::ofstream(dir / "bad.toml") << text;
            bool rejected = false;
            try {
                readPreset((dir / "bad.toml").string());
            } catch (...) {
                rejected = true;
            }
            CHECK(rejected);
        }
        std::ofstream(dir / "bad.toml") << std::string(131073, '#');
        bool rejected = false;
        try {
            readPreset((dir / "bad.toml").string());
        } catch (...) {
            rejected = true;
        }
        CHECK(rejected);
        std::ofstream(dir / "presets/user/bad.toml") << "bad TOML";
        PresetLibrary robust;
        Settings unchanged;
        robust.load(dir.string(), unchanged);
        CHECK(!robust.warning.empty() && robust.entries.size() == 11);
        auto readonlyDir = dir / "readonly";
        PresetLibrary readonly;
        Settings ro;
        readonly.load(readonlyDir.string(), ro, false);
        readonly.saveAs(ro);
        readonly.save(ro);
        CHECK(!fs::exists(readonlyDir));
        // A first launch honors edited defaults, and save-as avoids filename collisions.
        auto firstDir = dir / "first-launch";
        PresetLibrary first;
        auto factory = first.entries[0];
        factory.sound.bpm = 143;
        writePreset(factory, (firstDir / "presets/factory/harp.toml").string());
        Settings initial;
        first.load(firstDir.string(), initial);
        CHECK(initial.bpm == 143 && !first.modified(initial));
        fs::create_directories(firstDir / "presets/user");
        std::ofstream(firstDir / "presets/user/user-001.toml") << "keep this unfinished file";
        first.saveAs(initial);
        CHECK(first.current(initial).id == "user-002");
        std::ifstream protectedFile(firstDir / "presets/user/user-001.toml");
        std::string protectedText;
        std::getline(protectedFile, protectedText);
        CHECK(protectedText == "keep this unfinished file");
        fs::remove_all(dir);
        std::cout << "PASS: TOML validation, migration, remembered edits, globals, "
                     "per-field/preset restore, save-as and read-only mode\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        fs::remove_all(dir);
        return 1;
    }
}
