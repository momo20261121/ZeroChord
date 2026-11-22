#include "presets.hpp"
#include "patterns.hpp"
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
    const auto target = SettingsEditor::appField(r) ? SettingsEditor::Tab::App : SettingsEditor::Tab::Style;
    for (int n = 0; e.tab() != target && n < 3; ++n) press(e, 20);
    while (e.selected() != r)
        press(e, 18);
}
void choose(SettingsEditor &e, int value) {
    const auto groups = e.soundPicker() ? e.soundCategories().size() : 1;
    for (size_t group = 0; group < groups; group++) {
        for (size_t n = 0; n < e.choices().size(); n++) {
            if (e.choices()[e.highlighted()].value == value)
                return;
            press(e, 18);
        }
        if (e.soundPicker())
            press(e, 20);
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
        legacy.eqDb = {6, -2, 3};
        legacy.theme = 1;
        PresetLibrary lib;
        lib.load(dir.string(), legacy);
        CHECK(lib.entries.size() == 9 && lib.modified(legacy));
        CHECK(legacy.chordProgram == 88 && legacy.padProgram == 89 && legacy.harpProgram == 10);
        CHECK(fs::exists(dir / "presets/factory/pop.toml"));
        auto dream = legacy;
        lib.select(legacy, 4);
        CHECK(legacy.bpm == 112 && legacy.tonic == 2 && legacy.mode == PlaybackMode::Arp);
        CHECK(legacy.masterVolume == 47 && legacy.theme == 1);
        CHECK((legacy.eqDb == std::array<int, 3>{6, -2, 3}));
        legacy.bpm = 123;
        legacy.chordProgram = 2;
        legacy.chordVolume = 76;
        legacy.harpVolume = 61;
        legacy.echo = 27;
        legacy.strumMs = 330;
        lib.select(legacy, 1);
        CHECK(samePresetSettings(dream, legacy));
        lib.select(legacy, 4);
        CHECK(legacy.bpm == 123 && legacy.chordProgram == 2 && legacy.chordVolume == 76 &&
              legacy.harpVolume == 61 && legacy.echo == 27 && legacy.strumMs == 330);
        lib.save(legacy);
        saveSettings(legacy, (dir / "settings.conf").string());
        Settings loaded;
        loadSettings(loaded, (dir / "settings.conf").string());
        PresetLibrary reloaded;
        reloaded.load(dir.string(), loaded);
        CHECK(samePresetSettings(legacy, loaded) && loaded.masterVolume == 47 && loaded.theme == 1);
        CHECK(loaded.eqDb == legacy.eqDb);
        reloaded.select(loaded, 1);
        CHECK(samePresetSettings(dream, loaded));
        reloaded.restore(loaded);
        CHECK(!reloaded.modified(loaded));
        CHECK(loaded.bpm == 80 && loaded.tonic == 2 && loaded.scale == 1 &&
              loaded.combinations[0] == int(Quality::No3) &&
              loaded.degreeCombinations[3][3] == int(Quality::Sus4));
        CHECK(loaded.masterVolume == 47 && loaded.theme == 1);
        CHECK(loaded.eqDb == legacy.eqDb);
        CHECK(loaded.chordVolume == 76 && loaded.harpVolume == 61 && loaded.echo == 27 &&
              loaded.strumMs == 330);
        loaded.padProgram = 10;
        loaded.bpm = 135;
        loaded.mode = PlaybackMode::FrontOnly;
        loaded.chordPattern = 5; // New stable ID also round-trips through preset TOML.
        loaded.padMotion = 2;
        loaded.hold = 0;
        loaded.echo = 31;
        loaded.degreeCombinations[0][1] = int(Quality::Sus2);
        auto before = loaded;
        reloaded.saveAs(loaded);
        CHECK(reloaded.current(loaded).name == "User001" && !reloaded.modified(loaded));
        auto appOnly = loaded;
        appOnly.tonic = 11;
        appOnly.scale = 0;
        appOnly.hold = 1;
        appOnly.combinations[0] = int(Quality::Sus4);
        appOnly.masterVolume = 2;
        appOnly.theme = 0;
        appOnly.chordVolume = 5;
        appOnly.harpVolume = 6;
        appOnly.echo = 7;
        appOnly.strumMs = 90;
        CHECK(samePresetSettings(appOnly, loaded));
        CHECK(!reloaded.modified(appOnly));
        auto user = readPreset((dir / "presets/user/user-001.toml").string());
        CHECK(samePresetSettings(before, user.sound) && user.sound.padProgram == 10);
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
        row(editor, 21);
        press(editor, '\n');
        choose(editor, 46);
        CHECK(instrument.settings.padProgram == 10);
        press(editor, '\n');
        CHECK(instrument.settings.padProgram == 46 && instrument.settings.chordProgram == 88);
        press(editor, '\b');
        CHECK(instrument.settings.padProgram == 10 && instrument.settings.chordProgram == 88);
        row(editor, 13);
        press(editor, '\n');
        choose(editor, 4);
        press(editor, '\n');
        CHECK(instrument.settings.chordProgram == 4 && instrument.settings.padProgram == 10);
        press(editor, '\b');
        CHECK(instrument.settings.chordProgram == 88 && instrument.settings.padProgram == 10);
        instrument.settings.tonic = 7;
        instrument.settings.bpm = 211;
        row(editor, 3); press(editor, '\n');
        press(editor, 18); // Octave -> Key.
        press(editor, '\b');
        press(editor, 27);
        CHECK(instrument.settings.tonic == 0 && instrument.settings.bpm == 211);
        row(editor, 8);
        press(editor, '\n');
        press(editor, 'r');
        CHECK(instrument.settings.bpm == 211);
        CHECK(editor.tempoOpen() && editor.tempoText() == "135");
        press(editor, 27);
        CHECK(instrument.settings.bpm == 211);
        press(editor, '\n');
        press(editor, 'r');
        press(editor, '\n');
        CHECK(instrument.settings.bpm == 135);
        row(editor, 10);
        press(editor, '\n');
        CHECK(editor.backingOpen());
        press(editor, 'x'); press(editor, 'x'); press(editor, '\n'); // Arp only.
        press(editor, 'x'); press(editor, '\n'); // Apply.
        CHECK(instrument.settings.mode == PlaybackMode::Arp);
        press(editor, '\b');
        CHECK(instrument.settings.mode == PlaybackMode::FrontOnly);
        row(editor, 3);
        press(editor, '\n');
        press(editor, 17); // Remembered Key -> Octave.
        press(editor, 18); press(editor, 18); // Octave -> Key -> first combination.
        press(editor, '2');
        press(editor, 18);
        press(editor, '\n');
        choose(editor, int(Quality::Sus2));
        press(editor, '\n');
        CHECK(instrument.settings.degreeCombinations[0][1] == int(Quality::Sus2));
        press(editor, '\b');
        CHECK(instrument.settings.degreeCombinations[0][1] == -1);
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
        CHECK(instrument.settings.tonic == 9 && instrument.settings.masterVolume == 100);
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
        // Restoring one style never resets another style's remembered edits, including on disk.
        edited.select(editedSettings, 1);
        editedSettings.bpm = 157;
        editedSettings.chordProgram = 24;
        const auto other = editedSettings;
        edited.select(editedSettings, 4);
        editedSettings.bpm = 182;
        edited.restore(editedSettings);
        CHECK(editedSettings.bpm == 119);
        edited.select(editedSettings, 1);
        CHECK(samePresetSettings(editedSettings, other));
        edited.save(editedSettings);
        PresetLibrary isolation;
        Settings isolated;
        isolation.load(dir.string(), isolated);
        CHECK(samePresetSettings(isolated, other));
        isolation.select(isolated, 4);
        CHECK(isolated.bpm == 119);
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
        // Upgrade from v1 snapshots: settings.conf owns the current app controls.
        auto upgrade = dir / "upgrade";
        fs::create_directories(upgrade / "working");
        std::ofstream(upgrade / "session.toml") << "schema_version=1\nselected='dream'\n";
        std::ofstream(upgrade / "working/dream.toml")
            << "schema_version=1\nid='dream'\nname='Dream'\nbpm=91\nbacking_sound=16\n"
               "backing_volume=3\nforeground_volume=4\necho=1\nsweep_ms=60\nlevel=20\n"
               "tonic=11\nscale=0\nhold=1\ncombinations=['sus4','maj7','m7','aug']\n";
        Settings upgradeSettings = legacy;
        upgradeSettings.hold = 0;
        auto appBefore = upgradeSettings;
        PresetLibrary upgradeLibrary;
        upgradeLibrary.load(upgrade.string(), upgradeSettings);
        CHECK(upgradeSettings.padProgram == 16 && upgradeSettings.chordProgram == 16 &&
              upgradeSettings.bpm == 91 && upgradeSettings.tonic == appBefore.tonic &&
              upgradeSettings.scale == appBefore.scale && upgradeSettings.hold == 0 &&
              upgradeSettings.combinations == appBefore.combinations &&
              upgradeSettings.degreeCombinations == appBefore.degreeCombinations &&
              upgradeSettings.chordVolume == appBefore.chordVolume &&
              upgradeSettings.harpVolume == appBefore.harpVolume &&
              upgradeSettings.echo == appBefore.echo &&
              upgradeSettings.strumMs == appBefore.strumMs);
        upgradeLibrary.save(upgradeSettings);
        std::ifstream upgraded(upgrade / "working/dream.toml");
        std::string upgradedText((std::istreambuf_iterator<char>(upgraded)), {});
        CHECK(upgradedText.find("tonic") == std::string::npos &&
              upgradedText.find("combinations") == std::string::npos &&
              upgradedText.find("backing_volume") == std::string::npos &&
              upgradedText.find("foreground_volume") == std::string::npos &&
              upgradedText.find("echo") == std::string::npos &&
              upgradedText.find("sweep_ms") == std::string::npos &&
              upgradedText.find("level") == std::string::npos);
        // A first launch honors edited defaults, and save-as avoids filename collisions.
        auto firstDir = dir / "first-launch";
        PresetLibrary first;
        auto factory = first.entries[1];
        factory.sound.bpm = 143;
        writePreset(factory, (firstDir / "presets/factory/dream.toml").string());
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
        // Combined layers retain their exact selection through editable preset files.
        for (unsigned layers = 0; layers < 8; ++layers) {
            auto preset = lib.entries[4]; preset.sound.mode = modeForLayers(layers);
            writePreset(preset, (dir / "layers.toml").string());
            CHECK(readPreset((dir / "layers.toml").string()).sound.mode == preset.sound.mode);
        }
        // New defaults and stock Dream migration preserve custom Dream and other presets.
        PresetLibrary defaults;
        Settings start;
        defaults.load((dir / "clean").string(), start);
        CHECK(defaults.current(start).id == "dream" && defaults.order().front() == 1);
        CHECK(start.chordProgram == 88 && start.padProgram == 89 && start.harpProgram == 10);
        CHECK(start.mode == PlaybackMode::All && start.chordPattern == sustainedChordPattern);
        for (int preset : {0, 1, 2})
            CHECK(defaults.entries[preset].sound.chordPattern == sustainedChordPattern);
        // Previous preview stock upgrades, but a chosen Manual hit override survives.
        auto previewDir = dir / "preview";
        auto previewDream = defaults.entries[1];
        previewDream.sound.mode = PlaybackMode::Pad;
        previewDream.sound.chordPattern = 0;
        writePreset(previewDream, (previewDir / "presets/factory/dream.toml").string());
        writePreset(previewDream, (previewDir / "working/dream.toml").string());
        PresetLibrary preview;
        Settings previewSettings;
        preview.load(previewDir.string(), previewSettings);
        CHECK(preview.entries[1].sound.mode == PlaybackMode::All);
        CHECK(preview.entries[1].sound.chordPattern == sustainedChordPattern);
        // A modified definition and its working copy are never mistaken for stock.
        previewDream.sound.bpm = 137;
        writePreset(previewDream, (previewDir / "presets/factory/dream.toml").string());
        writePreset(previewDream, (previewDir / "working/dream.toml").string());
        PresetLibrary previewCustom;
        previewCustom.load(previewDir.string(), previewSettings);
        CHECK(previewCustom.entries[1].sound.chordPattern == 0);
        CHECK(previewCustom.entries[1].sound.bpm == 137);
        auto oldDream = defaults.entries[1];
        oldDream.sound = Settings{};
        oldDream.sound.chordProgram = oldDream.sound.padProgram = 89;
        oldDream.sound.harpProgram = 10;
        writePreset(oldDream, (dir / "clean/presets/factory/dream.toml").string());
        writePreset(oldDream, (dir / "clean/working/dream.toml").string());
        defaults.save(start);
        writePreset(oldDream, (dir / "clean/working/dream.toml").string());
        PresetLibrary migrated;
        migrated.load((dir / "clean").string(), start);
        CHECK(start.chordProgram == 88 && start.padProgram == 89);
        oldDream.sound.bpm = 137;
        writePreset(oldDream, (dir / "clean/presets/factory/dream.toml").string());
        writePreset(oldDream, (dir / "clean/working/dream.toml").string());
        PresetLibrary custom;
        custom.load((dir / "clean").string(), start);
        CHECK(start.chordProgram == 89 && start.bpm == 137);
        CHECK(custom.entries[1].sound.chordProgram == 89);
        // Envelopes persist per part and per preset; factory defaults remain independent.
        auto envelopeDir = dir / "envelopes";
        PresetLibrary envelopes;
        Settings envelopeSettings;
        envelopes.load(envelopeDir.string(), envelopeSettings);
        CHECK(envelopeSettings.envelopes == envelopes.entries[1].sound.envelopes);
        envelopeSettings.envelopes[0].values = {20, 300, 65, 700};
        envelopeSettings.envelopes[1].values = {0, 100, 20, 30};
        envelopeSettings.envelopes[2].values = {1000, 2000, 90, 3000};
        auto savedEnvelopes = envelopeSettings.envelopes;
        envelopes.select(envelopeSettings, 0);
        envelopeSettings.envelopes[0].values[0] = 40;
        envelopes.select(envelopeSettings, 1);
        CHECK(envelopeSettings.envelopes == savedEnvelopes);
        envelopes.saveAs(envelopeSettings);
        CHECK(envelopes.current(envelopeSettings).sound.envelopes == savedEnvelopes);
        envelopes.save(envelopeSettings);
        saveSettings(envelopeSettings, (envelopeDir / "settings.conf").string());
        Settings restoredEnvelopes;
        loadSettings(restoredEnvelopes, (envelopeDir / "settings.conf").string());
        CHECK(restoredEnvelopes.envelopes == savedEnvelopes);
        PresetLibrary envelopeReload;
        envelopeReload.load(envelopeDir.string(), restoredEnvelopes);
        CHECK(restoredEnvelopes.envelopes == savedEnvelopes);
        envelopeReload.select(restoredEnvelopes, 1);
        envelopeReload.restore(restoredEnvelopes);
        CHECK(restoredEnvelopes.envelopes == envelopes.entries[1].sound.envelopes);
        envelopeReload.select(restoredEnvelopes, 0);
        CHECK(restoredEnvelopes.envelopes[0].values[0] == 40);
        std::ofstream(dir / "invalid-envelope.toml") << "schema_version=2\nid='bad'\nname='Bad'\nfront_sustain_percent=101\n";
        rejected = false;
        try { readPreset((dir / "invalid-envelope.toml").string()); } catch (...) { rejected = true; }
        CHECK(rejected);
        std::ofstream(dir / "invalid-envelope.toml") << "schema_version=2\nid='bad'\nname='Bad'\npad_attack_ms=-2\n";
        rejected = false;
        try { readPreset((dir / "invalid-envelope.toml").string()); } catch (...) { rejected = true; }
        CHECK(rejected);
        fs::remove_all(dir);
        std::cout << "PASS: TOML validation, migration, remembered edits, globals, "
                     "per-field/preset restore, save-as and read-only mode\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        fs::remove_all(dir);
        return 1;
    }
}
