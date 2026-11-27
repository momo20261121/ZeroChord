#include "settings_editor.hpp"
#include "menu_navigation.hpp"
#include "settings_io.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::cerr << __LINE__ << ": " #x "\n";                                                 \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)
using namespace zc;
struct Capture : Sink {
    std::vector<Command> events;
    void send(const Command &c) override { events.push_back(c); }
};
struct MenuAudit : SettingsEditor {
    using SettingsEditor::SettingsEditor;
    using SettingsEditor::settingValue;
    std::vector<int> visibleRows() const {
        return page() == Page::FrontNotes ? frontRows()
               : page() == Page::Combinations ? combinationRows()
               : page() == Page::Envelope ? std::vector<int>{0, 1, 2, 3, 4} : rows();
    }
    int currentValue() const {
        const auto &s = instrument_.settings;
        if (page() == Page::Envelope) return envelopeValue(s);
        if (page() == Page::FrontNotes)
            return frontValue(s, selected());
        if (page() == Page::Combinations)
            return chordConfigValue(s);
        return settingValueNumber();
    }
    int defaultChoice() const { return defaultValue(); }
    bool adjustable() const {
        return selectedKind() == RowKind::Number;
    }
    bool action() const {
        return page() == Page::Envelope ? selected() == 4
               : page() == Page::FrontNotes ? frontReset()
               : page() == Page::Combinations ? selected() == 4
                                             : selected() == 19 || selected() == 20;
    }
};
void press(SettingsEditor &editor, char key) {
    editor.key(key, true);
    editor.key(key, false);
}
void row(SettingsEditor &e, int field) { zc::test::goToRow(e, field); }
void choose(SettingsEditor &editor, int value) {
    CHECK(editor.pickerOpen());
    const auto groups = editor.soundPicker() ? editor.soundCategories().size() : 1;
    for (size_t group = 0; group < groups; group++) {
        for (size_t n = 0; n < editor.choices().size(); n++) {
            if (editor.choices()[editor.highlighted()].value == value)
                return;
            press(editor, 18);
        }
        if (editor.soundPicker())
            press(editor, '\t'); // Tab: next sound group.
    }
    throw std::runtime_error("Choice missing");
}
int main() {
    Settings defaults;
    for (int column = 0; column < 7; column++)
        for (unsigned mask = 1; mask < 8; mask++)
            CHECK(defaults.quality(mask, column) == qualityFor(mask));
    for (int quality = 0; quality < qualityCount; quality++)
        for (int tonic = 0; tonic < 12; tonic++) {
            Chord chord{48 + tonic, -1, 6, Quality(quality)};
            auto harp = harpNotes(chord);
            for (int n = 1; n < 7; n++)
                CHECK(harp[n] > harp[n - 1]);
            if (chord.quality == Quality::No3)
                for (int pitch : harp)
                    CHECK((pitch - chord.root) % 12 == 0 || (pitch - chord.root) % 12 == 7);
        }
    CHECK((chordNotes({48, -1, 6, Quality::Sus2}) == std::array<int, 4>{48, 50, 55, 60}));
    CHECK((chordNotes({48, -1, 6, Quality::Sus4}) == std::array<int, 4>{48, 53, 55, 60}));
    Capture sink;
    Instrument instrument(sink);
    std::optional<int> outputVolume = 43;
    bool allowVolumeChange = true;
    int volumeWrites = 0;
    MenuAudit editor(instrument, [&] { return outputVolume; }, [&](int value) {
        ++volumeWrites;
        if (!allowVolumeChange) return false;
        outputVolume = value;
        return true;
    });
    press(editor, '\t');
    CHECK(editor.selected() == 2); // Preset is first on Style.
    CHECK((editor.rows() == std::vector<int>{2, 8, SettingsEditor::backingField,
                                             SettingsEditor::frontSoundField, 19, 20}));
    const auto original = instrument.settings;
    press(editor, '\t'); // Tab moves to the next tab; left/right stay for values.
    CHECK(editor.tab() == SettingsEditor::Tab::App && editor.selected() == 18);
    CHECK((editor.rows() == std::vector<int>{18, SettingsEditor::volumeEffectsField, 3, 22,
                                             SettingsEditor::allFrontField,
                                             SettingsEditor::loopField,
                                             SettingsEditor::midiInputField}));
    row(editor, SettingsEditor::volumeEffectsField);
    press(editor, '\n');
    CHECK(editor.page() == SettingsEditor::Page::VolumeEffects &&
          editor.selected() == SettingsEditor::systemVolumeField);
    CHECK((editor.rows() == std::vector<int>{SettingsEditor::systemVolumeField, 17, 4, 5, 6, 26, 27, 28,
                                            SettingsEditor::outputField}));
    sink.events.clear();
    press(editor, '\n');
    CHECK(!editor.pickerOpen() && volumeWrites == 0);
    press(editor, '\b');
    CHECK(outputVolume == 43 && volumeWrites == 0);
    outputVolume = 52; // The system can change the level while the page is open.
    press(editor, 19); // Values step along their 5% grid.
    CHECK(outputVolume == 50 && volumeWrites == 1 && sink.events.empty());
    CHECK(instrument.settings.masterVolume == original.masterVolume);
    allowVolumeChange = false;
    press(editor, 'c');
    CHECK(!editor.pickerOpen() && outputVolume == 50 && volumeWrites == 2);
    outputVolume.reset();
    press(editor, 19);
    CHECK(!editor.pickerOpen() && volumeWrites == 2);
    row(editor, 17);
    press(editor, 'c');
    CHECK(instrument.settings.masterVolume == 100);
    press(editor, 'z');
    CHECK(instrument.settings.masterVolume == 95 && volumeWrites == 2);
    press(editor, '\b');
    CHECK(instrument.settings.masterVolume == defaults.masterVolume);
    for (int field : {4, 5, 6}) {
        row(editor, field);
        press(editor, 'z');
        press(editor, '\b');
    }
    CHECK(instrument.settings.chordVolume == defaults.chordVolume &&
          instrument.settings.harpVolume == defaults.harpVolume &&
          instrument.settings.echo == defaults.echo &&
          instrument.settings.strumMs == defaults.strumMs);
    press(editor, 27);
    CHECK(editor.page() == SettingsEditor::Page::Setup && editor.selected() == SettingsEditor::volumeEffectsField);
    // MIDI input: an app-wide page. Plays and Velocity are lists; Keyboard only reports.
    CHECK(editor.settingValue(SettingsEditor::midiInputField) == "No keyboard");
    row(editor, SettingsEditor::midiInputField);
    press(editor, '\n');
    CHECK(editor.page() == SettingsEditor::Page::MidiInput &&
          editor.selected() == SettingsEditor::midiTargetField);
    CHECK((editor.rows() == std::vector<int>{SettingsEditor::midiDeviceField,
                                            SettingsEditor::midiTargetField,
                                            SettingsEditor::midiVelocityField}));
    sink.events.clear();
    press(editor, '\n');
    CHECK(editor.pickerOpen() && editor.choices().size() == midiTargetCount &&
          editor.choices()[editor.highlighted()].value == MidiFront);
    for (int n = 0; n < MidiBass; n++) press(editor, 18);
    press(editor, '\n');
    CHECK(!editor.pickerOpen() && instrument.settings.midiTarget == MidiBass);
    CHECK(std::none_of(sink.events.begin(), sink.events.end(), // Choosing plays no preview.
                       [](const Command &c) { return c.type == CommandType::On; }));
    instrument.midiDevice = "Keystation 49";
    CHECK(editor.settingValue(SettingsEditor::midiInputField) == "Plays Bass");
    CHECK(editor.settingValue(SettingsEditor::midiDeviceField) == "Keystation 49");
    press(editor, 17);
    CHECK(editor.selected() == SettingsEditor::midiDeviceField);
    press(editor, '\n');
    CHECK(!editor.pickerOpen() && editor.page() == SettingsEditor::Page::MidiInput);
    row(editor, SettingsEditor::midiVelocityField);
    press(editor, '\n');
    press(editor, 18);
    press(editor, '\n');
    CHECK(instrument.settings.midiVelocity == VelocityHard);
    press(editor, '\b'); // Restore the default.
    CHECK(instrument.settings.midiVelocity == VelocityNormal);
    // Style presets keep the app-wide MIDI choice.
    instrument.library.select(instrument.settings, 1);
    CHECK(instrument.settings.midiTarget == MidiBass);
    press(editor, 27);
    CHECK(editor.page() == SettingsEditor::Page::Setup &&
          editor.selected() == SettingsEditor::midiInputField);
    instrument.settings.midiTarget = defaults.midiTarget;
    instrument.library.select(instrument.settings, original.preset);
    instrument.midiDevice.clear();
    // All front: Scale rows, or Free play's Lowest note. Key and Scale are Front notes' own.
    row(editor, SettingsEditor::allFrontField);
    press(editor, '\n');
    CHECK(editor.page() == SettingsEditor::Page::AllFront &&
          editor.selected() == SettingsEditor::allFrontModeField);
    CHECK((editor.rows() == std::vector<int>{SettingsEditor::allFrontModeField,
                                            SettingsEditor::allFrontKeyField,
                                            SettingsEditor::allFrontScaleField,
                                            SettingsEditor::allFrontOctaveField}));
    row(editor, SettingsEditor::allFrontKeyField);
    press(editor, '\n');
    CHECK(editor.pickerOpen() && editor.choices()[editor.highlighted()].value == -1);
    for (int n = 0; n < 3; n++) press(editor, 18); // Follow chord, C, C#, D.
    press(editor, '\n');
    CHECK(!instrument.settings.frontFollow && instrument.settings.frontTonic == 2 &&
          instrument.settings.scale == instrument.settings.chordScale);
    CHECK(editor.settingValue(SettingsEditor::allFrontKeyField) == "D");
    {
        // Front notes shows the same key, and choosing there shows here.
        Settings &s = instrument.settings;
        CHECK(s.frontKeyTonic() == 2);
        s.frontMode = 1;
    }
    row(editor, SettingsEditor::allFrontOctaveField);
    press(editor, 19);
    CHECK(instrument.settings.allFrontOctave == 2);
    CHECK(editor.settingValue(SettingsEditor::allFrontOctaveField) == "2  (Z-M D2, 4-0 D5)");
    press(editor, '\b');
    CHECK(instrument.settings.allFrontOctave == 3);
    row(editor, SettingsEditor::allFrontKeyField);
    press(editor, '\b'); // Backspace follows the chord again: key and scale together.
    CHECK(instrument.settings.frontFollow == 1);
    row(editor, SettingsEditor::allFrontModeField);
    press(editor, '\n');
    press(editor, 18);
    press(editor, '\n');
    CHECK(instrument.settings.allFrontMode == 1);
    CHECK((editor.rows() == std::vector<int>{SettingsEditor::allFrontModeField,
                                            SettingsEditor::allFrontLowestField}));
    CHECK(editor.settingValue(SettingsEditor::allFrontLowestField) == "F3-E5");
    row(editor, SettingsEditor::allFrontLowestField);
    press(editor, 19); // White keys only: F3 to E3.
    CHECK(instrument.settings.allFrontLowest == 52);
    press(editor, 20);
    press(editor, 20);
    CHECK(instrument.settings.allFrontLowest == 55);
    press(editor, 27);
    CHECK(editor.page() == SettingsEditor::Page::Setup &&
          editor.selected() == SettingsEditor::allFrontField &&
          editor.settingValue(SettingsEditor::allFrontField) == "Free play");
    instrument.settings.allFrontMode = defaults.allFrontMode;
    instrument.settings.allFrontLowest = defaults.allFrontLowest;
    instrument.settings.frontMode = defaults.frontMode;
    press(editor, '1'); // Number keys jump to a tab.
    CHECK(editor.tab() == SettingsEditor::Tab::Style && editor.selected() == 2);
    CHECK(samePresetSettings(original, instrument.settings));
    row(editor, 3);
    press(editor, '\n');
    CHECK(editor.page() == SettingsEditor::Page::Combinations);
    CHECK(editor.selected() == 6);
    press(editor, 18); // Octave -> Key -> Scale.
    press(editor, 18);
    // Natural minor moves the III, VI and VII roots; the M/m/7 rows stay as they are.
    press(editor, '\n');
    choose(editor, 1);
    press(editor, '\n');
    CHECK(instrument.settings.chordScale == 1 && instrument.settings.chordRoot(1) % 12 == 3 &&
          instrument.settings.chordRoot(4) % 12 == 8 && instrument.settings.chordRoot(5) % 12 == 10 &&
          instrument.settings.chordRoot(3) % 12 == 7);
    press(editor, '\b');
    CHECK(instrument.settings.chordScale == 0 && instrument.settings.chordRoot(1) % 12 == 4);
    press(editor, 18); // Scale -> Hold.
    press(editor, '\n');
    choose(editor, 0);
    press(editor, 27);
    CHECK(instrument.settings.hold == defaults.hold);
    press(editor, '\n');
    choose(editor, 0);
    press(editor, '\n');
    CHECK(instrument.settings.hold == 0);
    press(editor, '\b');
    CHECK(instrument.settings.hold == defaults.hold);
    press(editor, 18);
    CHECK(editor.scope() == -1 && editor.selected() == 0);
    // Choices open a list with Enter; left/right neither change them nor switch tabs.
    press(editor, 20);
    press(editor, 19);
    CHECK(instrument.settings.combinations[0] == int(Quality::Diminished) && editor.scope() == -1);
    // Tab only moves forward and wraps; number keys jump.
    press(editor, '\t');
    CHECK(editor.scope() == 0);
    for (int n = 0; n < 7; n++) press(editor, '\t');
    CHECK(editor.scope() == -1);
    press(editor, '5');
    CHECK(editor.scope() == 3);
    press(editor, '\t');
    CHECK(editor.scope() == 4);
    press(editor, '1');
    press(editor, '\n');
    choose(editor, int(Quality::No3));
    CHECK(instrument.settings.combinations[0] ==
          int(Quality::Diminished)); // Preview is transactional.
    CHECK(std::any_of(sink.events.begin(), sink.events.end(), [](const Command &c) {
        return c.type == CommandType::On && c.durationMs > 0;
    }));
    press(editor, 27);
    CHECK(!editor.pickerOpen());
    CHECK(instrument.settings.combinations[0] == int(Quality::Diminished));
    press(editor, '\n');
    choose(editor, int(Quality::No3));
    press(editor, '\n');
    for (int col = 0; col < 7; col++)
        CHECK(instrument.settings.quality(3, col) == Quality::No3);
    for (char key = '2'; key <= '8'; key++) {
        press(editor, key);
        CHECK(editor.scope() == key - '2');
    }
    press(editor, '5'); // V override.
    press(editor, '\n');
    choose(editor, int(Quality::Sus4));
    press(editor, '8'); // Scope shortcuts do not switch the target while a picker is open.
    CHECK(editor.scope() == 3);
    press(editor, '\n');
    CHECK(instrument.settings.quality(3, 3) == Quality::Sus4);
    CHECK(instrument.settings.quality(3, 6) == Quality::No3);
    press(editor, '1');
    press(editor, '\n');
    choose(editor, int(Quality::Sus2));
    press(editor, '\n');
    CHECK(instrument.settings.quality(3, 3) == Quality::Sus4);
    CHECK(instrument.settings.quality(3, 6) == Quality::Sus2);
    // Verify the actual performance dispatcher, including release stability, sharp and slash.
    instrument.key('5', true, 1);
    instrument.key('r', true, 2);
    CHECK(chordName(instrument.chord) == "Gsus4");
    instrument.key('r', false, 3);
    CHECK(instrument.chord.quality == Quality::Sus4);
    instrument.panic();
    instrument.key('1', true, 4);
    instrument.key('8', true, 5);
    instrument.key('u', true, 6);
    CHECK(chordName(instrument.chord) == "C#sus2");
    instrument.key('5', true, 7);
    CHECK(chordName(instrument.chord) == "C#sus2/Ab");
    instrument.panic();
    // Test all four customized combinations in every column, plus unchanged single buttons.
    const char *rows[] = {"2345678", "qwertyu", "asdfghj"};
    for (int col = 0; col < 7; col++)
        for (int combo = 0; combo < 4; combo++) {
            instrument.settings.degreeCombinations[col][combo] = int(Quality::No3);
            instrument.panic();
            for (int row = 0; row < 3; row++)
                if (combinationMasks[combo] & (1u << row))
                    instrument.key(rows[row][col], true, 10 + row);
            CHECK(instrument.chord.quality == Quality::No3);
            CHECK(instrument.settings.quality(1, col) == Quality::Major);
            CHECK(instrument.settings.quality(2, col) == Quality::Minor);
            CHECK(instrument.settings.quality(4, col) == Quality::Seventh);
        }
    press(editor, '5');
    press(editor, '\n');
    choose(editor, -1);
    press(editor, '\n');
    CHECK(instrument.settings.degreeCombinations[3][0] == -1);
    CHECK(instrument.settings.quality(3, 3) == Quality::Sus2);
    for (int n = 0; n < 4; n++)
        press(editor, 18);
    press(editor, '\n');
    choose(editor, 1);
    press(editor, '\n'); // Reset V only.
    CHECK(instrument.settings.degreeCombinations[3] == defaults.degreeCombinations[3]);
    CHECK(instrument.settings.degreeCombinations[0][0] == int(Quality::No3));
    // Reset all requires confirmation, includes Key, and preserves unrelated sound settings.
    instrument.settings.tonic = 4;
    instrument.settings.hold = 0;
    press(editor, '1');
    press(editor, '\n');
    choose(editor, 1);
    press(editor, 27);
    CHECK(instrument.settings.combinations[0] == int(Quality::Sus2));
    press(editor, '\n');
    choose(editor, 1);
    press(editor, '\n');
    CHECK(instrument.settings.combinations == defaults.combinations);
    CHECK(instrument.settings.degreeCombinations == defaults.degreeCombinations);
    CHECK(instrument.settings.tonic == 0 && instrument.settings.hold == defaults.hold);
    instrument.settings.tonic = 4;
    CHECK(instrument.settings.combinationMs == 30);
    for (int n = 0; n < 9; n++) // Restore wraps to Octave, then on to combo timing.
        press(editor, 18);
    CHECK(editor.selected() == 5 && editor.scope() == -1);
    sink.events.clear();
    press(editor, '\n');
    choose(editor, Settings::maxCombinationMs);
    CHECK(sink.events.empty() && instrument.settings.combinationMs == 30);
    press(editor, 27);
    press(editor, '\n');
    choose(editor, Settings::maxCombinationMs);
    press(editor, '\n');
    CHECK(instrument.settings.combinationMs == 120);
    press(editor, '\b');
    CHECK(instrument.settings.combinationMs == 30);
    press(editor, 20); // Numbers step with left/right and stop at their ends.
    CHECK(instrument.settings.combinationMs == 35);
    press(editor, 19);
    CHECK(instrument.settings.combinationMs == 30);
    press(editor, '\t');
    CHECK(editor.scope() == 0 && editor.selected() == 0);
    press(editor, '1');
    CHECK(editor.scope() == -1);
    press(editor, 27);
    CHECK(editor.settingsOpen && editor.page() == SettingsEditor::Page::Setup);
    row(editor, 2);
    CHECK(editor.selected() == 2);
    press(editor, '\n');
    choose(editor, 2);
    CHECK(instrument.settings.preset == 0);
    editor.cancelPending(); // Same cleanup used on long Esc or signal exit.
    CHECK(instrument.settings.preset == 0);
    CHECK(sink.events.back().type == CommandType::Configure &&
          sink.events.back().settings.preset == 0);
    press(editor, '\n');
    choose(editor, 0);
    press(editor, '\n');
    CHECK(instrument.settings.preset == 0);
    press(editor, 20); // The preset is a choice: only its list changes it.
    CHECK(instrument.settings.preset == 0);
    press(editor, '\t');
    CHECK(instrument.settings.preset == 0); // Tab switches tabs without editing.
    CHECK(editor.tab() == SettingsEditor::Tab::App && editor.selected() == 3);
    row(editor, 2);
    press(editor, '\n');
    choose(editor, 2);
    press(editor, '\n');
    CHECK(instrument.settings.preset == 2);
    press(editor, 27);
    CHECK(!editor.settingsOpen);
    // Persistence, backwards compatibility, malformed lines, and invalid enum recovery.
    auto dir = std::filesystem::temp_directory_path() /
               ("zerochord-settings-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    auto path = (dir / "settings.conf").string();
    instrument.settings.combinations[3] = int(Quality::Sus4);
    instrument.settings.degreeCombinations[6][0] = int(Quality::No3);
    instrument.settings.eqDb = {6, -3, 2};
    instrument.settings.midiTarget = MidiDrums;
    instrument.settings.midiVelocity = VelocityFixed;
    instrument.settings.allFrontMode = 1;
    instrument.settings.allFrontOctave = 2;
    instrument.settings.allFrontLowest = 48;
    saveSettings(instrument.settings, path);
    Settings loaded;
    loadSettings(loaded, path);
    CHECK(loaded.combinations == instrument.settings.combinations);
    CHECK(loaded.degreeCombinations == instrument.settings.degreeCombinations);
    CHECK(loaded.preset == 2 && loaded.tonic == 4);
    CHECK(loaded.eqDb == instrument.settings.eqDb);
    CHECK(loaded.midiTarget == MidiDrums && loaded.midiVelocity == VelocityFixed);
    CHECK(loaded.allFrontMode == 1 && loaded.allFrontOctave == 2 && loaded.allFrontLowest == 48);
    loaded.midiTarget = 99, loaded.midiVelocity = -4;
    loaded.clamp();
    CHECK(loaded.midiTarget == MidiDrums && loaded.midiVelocity == VelocitySoft);
    loaded.eqDb = {-99, 0, 99}; loaded.clamp();
    CHECK((loaded.eqDb == std::array<int, 3>{-12, 0, 12}));
    {
        std::ofstream out(path);
        out << "tonic 7\npreset 1\nbad data\ncombo3 999\ndegree6combo3 -9\nscale 1\n";
    }
    Settings legacy;
    loadSettings(legacy, path);
    CHECK(legacy.tonic == 7 && legacy.preset == 1 && legacy.scale == 1);
    CHECK((legacy.eqDb == std::array<int, 3>{0, 0, 0}));
    CHECK(legacy.combinations == defaults.combinations &&
          legacy.degreeCombinations == defaults.degreeCombinations);
    {
        // A preset can still use a legacy arp as its default after the player changes away from it.
        Capture captured;
        Instrument player(captured);
        auto custom = player.library.current(player.settings);
        custom.id = "custom-legacy-arp";
        custom.name = "Custom legacy arp";
        custom.factory = false;
        custom.sound.pattern = 6;
        player.library.entries.push_back(custom);
        player.library.select(player.settings, int(player.library.entries.size()) - 1);
        player.settings.pattern = 0;
        SettingsEditor menu(player);
        press(menu, '\t');
        row(menu, 9);
        press(menu, '\n');
        press(menu, '\b');
        CHECK(menu.choices()[menu.highlighted()].value == 6);
        CHECK(player.settings.pattern == 0);
        press(menu, '\n');
        CHECK(player.settings.pattern == 6);
    }
    // O opens Backing on its Layers tab. Mute toggles and mix apply live and keep the chord.
    for (unsigned initial = 0; initial < 8; ++initial) {
        Capture captured; Instrument p(captured); SettingsEditor e(p);
        p.settings.mode = modeForLayers(initial);
        p.settings.combinationMs = 0; p.key('8', true, 10); p.key('8', false, 11);
        auto harmony = p.chord;
        press(e, 'o');
        CHECK(e.backingOpen() && e.page() == SettingsEditor::Page::Backing && e.backingTab() == 0);
        CHECK(e.selected() == SettingsEditor::chordMixField);
        for (auto layer : backingLayerOrder) {
            auto previous = p.settings.enabledLayers();
            press(e, '\n');
            CHECK(p.settings.enabledLayers() == (previous ^ backingLayers(layer)));
            press(e, 'x');
        }
        CHECK(p.settings.enabledLayers() == ((initial | 8u) ^ 15u)); // Drums start unmuted.
        CHECK(p.active && p.chord == harmony);
        CHECK(e.selected() == SettingsEditor::chordMixField);
        const bool chordOn = layerEnabled(p.settings.mode, PlaybackMode::Chord);
        press(e, 'z');
        CHECK(p.settings.mix(PlaybackMode::Chord) == 95 &&
              layerEnabled(p.settings.mode, PlaybackMode::Chord) == chordOn);
        press(e, 'c'); // Raising a muted layer unmutes it.
        CHECK(p.settings.mix(PlaybackMode::Chord) == 100 && layerEnabled(p.settings.mode, PlaybackMode::Chord));
        for (int n = 0; n < 25; n++) press(e, 'z');
        CHECK(p.settings.mix(PlaybackMode::Chord) == 0 && !p.settings.audible(PlaybackMode::Chord));
        press(e, '\n'); press(e, '\n'); // Unmuting a silent layer restores an audible level.
        CHECK(p.settings.audible(PlaybackMode::Chord));
        press(e, '\b');
        const auto &d = p.library.current(p.settings).sound;
        CHECK(p.settings.mix(PlaybackMode::Chord) == d.mix(PlaybackMode::Chord) &&
              layerEnabled(p.settings.mode, PlaybackMode::Chord) == layerEnabled(d.mode, PlaybackMode::Chord));
        p.settings.mix(PlaybackMode::Pad) = 35;
        press(e, 27);
        CHECK(!e.settingsOpen && !e.backingOpen() && p.active && p.chord == harmony);
        press(e, '\t'); // O leaves the settings pages where they were.
        CHECK(e.settingsOpen && e.page() == SettingsEditor::Page::Setup && e.selected() == 2 &&
              e.tab() == SettingsEditor::Tab::Style);
        press(e, 27);
        press(e, 'o'); press(e, '\t'); // Tab moves through the tabs; Esc returns to playing.
        CHECK(e.backingOpen() && e.backingTab() == 1);
        press(e, 27);
        CHECK(!e.backingOpen() && !e.settingsOpen);
        auto file = path + ".layers";
        saveSettings(p.settings, file); Settings loaded; loadSettings(loaded, file);
        CHECK(loaded.mode == p.settings.mode && loaded.layerMix == p.settings.layerMix);
        std::filesystem::remove(file);
    }
    // Audit every page/picker with both desktop arrow events and the physical
    // Cardputer's printed F/X/Z/C arrows. Previews must never commit settings.
    for (const auto nav : {std::array<char, 4>{17, 18, 19, 20},
                           std::array<char, 4>{'f', 'x', 'z', 'c'}}) {
        Capture captured;
        Instrument walker(captured);
        // Start from a loaded preset, whose sounds are explicit rather than placeholders.
        walker.library.restore(walker.settings);
        MenuAudit menu(walker);
        const auto up = nav[0], down = nav[1], left = nav[2], right = nav[3];
        auto state = [&]() {
            saveSettings(walker.settings, path);
            std::ifstream file(path);
            return std::string(std::istreambuf_iterator<char>(file), {});
        };
        auto checkPicker = [&]() {
            const auto before = state();
            const auto saved = walker.settings;
            const auto page = menu.page();
            const auto tab = menu.tab();
            const int scope = menu.scope(), frontKey = menu.frontKey(), row = menu.selected();
            // Left/right change a value row at once; the opposite arrow undoes it.
            if (menu.adjustable()) {
                press(menu, right);
                if (state() == before) {
                    press(menu, left);
                    if (state() != before) press(menu, right);
                } else
                    press(menu, left);
                CHECK(state() == before && menu.selected() == row && menu.page() == page);
                walker.settings = saved;
                walker.configure();
            }
            press(menu, '\n');
            if (page == SettingsEditor::Page::Setup && row == 8) {
                CHECK(menu.tempoOpen());
                press(menu, '1'); press(menu, '3'); press(menu, '7');
                CHECK(menu.tempoText() == "137" && state() == before);
                press(menu, 27);
                CHECK(!menu.tempoOpen() && state() == before);
                press(menu, '\n'); press(menu, 'r'); press(menu, '\n');
                CHECK(walker.settings.bpm == walker.library.current(walker.settings).sound.bpm);
                walker.settings = saved;
                return;
            }
            CHECK(menu.pickerOpen() && !menu.choices().empty());
            const int start = menu.highlighted();
            if (menu.action()) {
                // A confirmation is one question: arrows do nothing, Esc keeps everything.
                press(menu, down); press(menu, right);
                CHECK(menu.highlighted() == start && menu.choices()[start].value == 1);
                press(menu, 27);
                CHECK(!menu.pickerOpen() && menu.settingsOpen && state() == before);
                return;
            }
            const int candidate = menu.choices()[(start + 1) % menu.choices().size()].value;
            press(menu, down);
            CHECK(menu.highlighted() == (start + 1) % int(menu.choices().size()));
            press(menu, up);
            CHECK(menu.highlighted() == start);
            if (menu.soundPicker()) {
                const auto categories = menu.soundCategories();
                const auto category = menu.soundCategory();
                press(menu, '\t');
                CHECK(menu.soundCategory() != category && !menu.choices().empty());
                const int index = int(std::find(categories.begin(), categories.end(), category) -
                                      categories.begin());
                press(menu, char('1' + index));
                CHECK(menu.soundCategory() == category);
            } else {
                press(menu, right);
                CHECK(menu.highlighted() == (start + 1) % int(menu.choices().size()));
                press(menu, left);
                CHECK(menu.highlighted() == start);
            }
            CHECK(menu.page() == page && menu.tab() == tab && menu.scope() == scope &&
                  menu.frontKey() == frontKey && menu.selected() == row);
            CHECK(state() == before);
            if (!(page == SettingsEditor::Page::Setup && row == 2)) {
                press(menu, '\b');
                CHECK(menu.choices()[menu.highlighted()].value == menu.defaultChoice());
            }
            press(menu, 27);
            CHECK(!menu.pickerOpen() && menu.settingsOpen && state() == before);
            press(menu, '\n');
            choose(menu, candidate);
            press(menu, '\n');
            CHECK(!menu.pickerOpen() && menu.currentValue() == candidate);
            walker.settings = saved;
            walker.configure();
        };
        press(menu, '\t');
        for (int tab = 0; tab < 2; tab++) {
            const auto list = menu.visibleRows();
            for (size_t n = 0; n < list.size(); n++) {
                CHECK(menu.selected() == list[n]);
                if (menu.selected() == SettingsEditor::volumeEffectsField) {
                    press(menu, '\n');
                    CHECK(menu.page() == SettingsEditor::Page::VolumeEffects);
                    const auto fields = menu.visibleRows();
                    for (int field : fields) {
                        CHECK(menu.selected() == field);
                        const auto before = state();
                        if (field == SettingsEditor::outputField) { // A list: Auto, Speaker, Headphones.
                            press(menu, '\n');
                            CHECK(menu.pickerOpen() && menu.choices().size() == 3);
                            press(menu, 27);
                            CHECK(!menu.pickerOpen() && state() == before);
                            press(menu, down);
                            continue;
                        }
                        press(menu, '\n');
                        CHECK(!menu.pickerOpen() && state() == before);
                        if (field != SettingsEditor::systemVolumeField) {
                            const int initial = menu.currentValue();
                            press(menu, left);
                            CHECK(menu.currentValue() == initial - (field >= 26 ? 1 : 5));
                            CHECK(menu.selected() == field && state() != before);
                            press(menu, right);
                            CHECK(state() == before);
                        }
                        press(menu, down);
                    }
                    CHECK(menu.selected() == fields.front());
                    press(menu, up);
                    const auto before = state();
                    CHECK(menu.page() == SettingsEditor::Page::VolumeEffects && menu.selected() == fields.back());
                    press(menu, '\t'); // No tabs here: Tab does nothing.
                    CHECK(menu.page() == SettingsEditor::Page::VolumeEffects && state() == before);
                    press(menu, 27);
                    CHECK(menu.page() == SettingsEditor::Page::Setup && menu.selected() == SettingsEditor::volumeEffectsField);
                    press(menu, '\n');
                    CHECK(menu.page() == SettingsEditor::Page::VolumeEffects && menu.selected() == fields.back());
                    press(menu, 27);
                    CHECK(menu.page() == SettingsEditor::Page::Setup && menu.selected() == SettingsEditor::volumeEffectsField);
                } else if (menu.selected() == SettingsEditor::allFrontField) {
                    for (int mode = 0; mode < 2; mode++) {
                        walker.settings.allFrontMode = mode;
                        press(menu, '\n');
                        CHECK(menu.page() == SettingsEditor::Page::AllFront);
                        for (int field : menu.visibleRows()) {
                            CHECK(menu.selected() == field);
                            if (field == SettingsEditor::allFrontOctaveField ||
                                field == SettingsEditor::allFrontLowestField) {
                                const auto before = state();
                                press(menu, '\n');
                                CHECK(!menu.pickerOpen() && state() == before);
                                press(menu, right);
                                CHECK(state() != before);
                                press(menu, left);
                                CHECK(state() == before);
                            } else checkPicker();
                            press(menu, down);
                        }
                        press(menu, 27);
                        CHECK(menu.page() == SettingsEditor::Page::Setup &&
                              menu.selected() == SettingsEditor::allFrontField);
                    }
                    walker.settings.allFrontMode = 0;
                } else if (menu.selected() == SettingsEditor::loopField) {
                    // With no loop recorded, Loop mix and Delete do nothing; the rest behave as
                    // their kind says.
                    press(menu, '\n');
                    CHECK(menu.page() == SettingsEditor::Page::Loop &&
                          menu.selected() == SettingsEditor::loopStyleField);
                    press(menu, up);
                    for (int field : menu.visibleRows()) {
                        CHECK(menu.selected() == field);
                        const auto before = state();
                        if (field == SettingsEditor::loopStyleField) checkPicker();
                        else if (field == SettingsEditor::loopBarsField ||
                                 field == SettingsEditor::loopCountField ||
                                 field == SettingsEditor::loopShortField ||
                                 field == SettingsEditor::loopSnapField) {
                            press(menu, '\n');
                            CHECK(!menu.pickerOpen() && state() == before);
                            press(menu, left);
                            CHECK(state() != before);
                            press(menu, right);
                            CHECK(state() == before);
                        } else {
                            for (char key : {char('\n'), left, right, char('\b')}) press(menu, key);
                            CHECK(!menu.pickerOpen() && !menu.loopQuestion() &&
                                  menu.page() == SettingsEditor::Page::Loop && state() == before);
                        }
                        press(menu, down);
                    }
                    press(menu, 27);
                    CHECK(menu.page() == SettingsEditor::Page::Setup &&
                          menu.selected() == SettingsEditor::loopField);
                } else if (menu.selected() == SettingsEditor::midiInputField) {
                    press(menu, '\n');
                    CHECK(menu.page() == SettingsEditor::Page::MidiInput);
                    press(menu, up); // It opens on Plays; Keyboard is above.
                    for (int field : menu.visibleRows()) {
                        CHECK(menu.selected() == field);
                        if (field == SettingsEditor::midiDeviceField) {
                            const auto before = state();
                            for (char key : {char('\n'), left, right, char('\b')}) press(menu, key);
                            CHECK(!menu.pickerOpen() && state() == before);
                        } else checkPicker();
                        press(menu, down);
                    }
                    press(menu, 27);
                    CHECK(menu.page() == SettingsEditor::Page::Setup &&
                          menu.selected() == SettingsEditor::midiInputField);
                } else if (menu.selected() == SettingsEditor::backingField ||
                           menu.selected() == SettingsEditor::frontSoundField) {
                    const int opener = menu.selected();
                    const bool backing = opener == SettingsEditor::backingField;
                    press(menu, '\n');
                    CHECK(menu.page() == (backing ? SettingsEditor::Page::Backing
                                                  : SettingsEditor::Page::FrontSound));
                    for (int tab = 0; tab < (backing ? SettingsEditor::backingTabCount : 1); tab++) {
                        if (backing) {
                            press(menu, char('1' + tab));
                            CHECK(menu.backingTab() == tab);
                        }
                        const auto fields = menu.visibleRows();
                        for (int field : fields) {
                            CHECK(menu.selected() == field);
                            if ((field >= SettingsEditor::chordMixField &&
                                 field < SettingsEditor::chordMixField + 3) ||
                                field == SettingsEditor::drumMixField ||
                                field == SettingsEditor::bassMixField) {
                                const auto before = state();
                                const auto saved = walker.settings;
                                const auto layer = field == SettingsEditor::drumMixField
                                                       ? PlaybackMode::Drums
                                                   : field == SettingsEditor::bassMixField
                                                       ? PlaybackMode::Bass
                                                       : backingLayerOrder[field - SettingsEditor::chordMixField];
                                press(menu, '\n');
                                CHECK(state() != before);
                                press(menu, '\n');
                                CHECK(state() == before);
                                press(menu, left);
                                CHECK(walker.settings.mix(layer) == saved.mix(layer) - 5);
                                press(menu, right);
                                CHECK(walker.settings.mix(layer) == saved.mix(layer) &&
                                      walker.settings.enabled(layer));
                                walker.settings = saved;
                                walker.configure();
                            } else if ((field >= 23 && field <= 25) || field == SettingsEditor::arpEnvelopeField ||
                                       field == SettingsEditor::bassEnvelopeField) {
                                const auto parent = menu.page();
                                const int part = field == SettingsEditor::arpEnvelopeField    ? 3
                                                 : field == SettingsEditor::bassEnvelopeField ? 4
                                                                                              : field - 23;
                                press(menu, '\n');
                                CHECK(menu.page() == SettingsEditor::Page::Envelope && menu.envelopePart() == part);
                                for (int control = 0; control < 5; control++) {
                                    CHECK(menu.selected() == control);
                                    checkPicker();
                                    press(menu, down);
                                }
                                press(menu, '\t');
                                CHECK(menu.page() == SettingsEditor::Page::Envelope && menu.envelopePart() == part);
                                press(menu, 27);
                                CHECK(menu.page() == parent && menu.selected() == field);
                            } else
                                checkPicker();
                            press(menu, down);
                        }
                        CHECK(menu.selected() == fields.front());
                    }
                    if (backing) {
                        press(menu, '\t'); // From Drums, the next tab wraps to Layers.
                        CHECK(menu.backingTab() == 0);
                    }
                    press(menu, 27);
                    CHECK(menu.page() == SettingsEditor::Page::Setup && menu.selected() == opener);
                } else if (menu.selected() != 3 && menu.selected() != 22)
                    checkPicker();
                press(menu, down);
            }
            CHECK(menu.selected() == list[0]);
            press(menu, up);
            CHECK(menu.selected() == list.back());
            press(menu, down);
            press(menu, '\t');
        }
        // Shortcuts is a read-only list: up/down scroll it and change nothing else.
        CHECK(menu.tab() == SettingsEditor::Tab::Shortcuts && menu.rows().empty());
        const auto beforeShortcuts = state();
        const int shortcutCount = int(SettingsEditor::shortcuts(walker.settings).size());
        CHECK(menu.shortcutRow() == 0 && shortcutCount > 20);
        press(menu, down);
        CHECK(menu.shortcutRow() == 1);
        press(menu, up);
        press(menu, up); // Wraps to the last shortcut.
        CHECK(menu.shortcutRow() == shortcutCount - 1);
        for (char key : {left, right, char('\n'), char('\b'), 'r', 'o', 'p'}) {
            press(menu, key);
            CHECK(menu.tab() == SettingsEditor::Tab::Shortcuts && menu.selected() == -1);
            CHECK(!menu.pickerOpen() && !menu.tempoOpen() && !menu.backingOpen());
            CHECK(state() == beforeShortcuts);
        }
        press(menu, '\t');
        CHECK(menu.tab() == SettingsEditor::Tab::About && menu.rows().empty());
        const auto beforeAbout = state();
        for (char key : {up, down, left, right, char('\n'), char('\b'), 'r', 'o', 'p'}) {
            press(menu, key);
            CHECK(menu.tab() == SettingsEditor::Tab::About && menu.selected() == -1);
            CHECK(!menu.pickerOpen() && !menu.tempoOpen() && !menu.backingOpen());
            CHECK(state() == beforeAbout);
        }
        press(menu, '\t'); // Tab wraps from the last tab to the first.
        CHECK(menu.tab() == SettingsEditor::Tab::Style && menu.selected() == 2);
        press(menu, '3');
        CHECK(menu.tab() == SettingsEditor::Tab::Shortcuts);
        press(menu, '4');
        CHECK(menu.tab() == SettingsEditor::Tab::About);
        press(menu, 27); CHECK(!menu.settingsOpen);
        press(menu, '\t'); CHECK(menu.settingsOpen && menu.tab() == SettingsEditor::Tab::About);
        press(menu, '2');
        CHECK(menu.tab() == SettingsEditor::Tab::App && menu.selected() == 18);
        row(menu, 22);
        for (int mode = 0; mode < 2; mode++) {
            walker.settings.frontMode = mode;
            press(menu, '\n');
            press(menu, '1');
            for (int key = -1; key < 7; key++) {
                CHECK(menu.frontKey() == key);
                const auto list = menu.visibleRows();
                for (int field : list) {
                    CHECK(menu.selected() == field);
                    checkPicker();
                    press(menu, down);
                }
                CHECK(menu.selected() == list.front());
                press(menu, up);
                CHECK(menu.selected() == list.back());
                press(menu, down);
                press(menu, '\t');
            }
            CHECK(menu.frontKey() == -1);
            press(menu, 27);
            CHECK(menu.page() == SettingsEditor::Page::Setup && menu.selected() == 22);
        }
        row(menu, 3);
        press(menu, '\n');
        for (int scope = -1; scope < 7; scope++) {
            CHECK(menu.scope() == scope);
            const auto list = menu.visibleRows();
            for (int field : list) {
                CHECK(menu.selected() == field);
                checkPicker();
                press(menu, down);
            }
            CHECK(menu.selected() == list.front());
            press(menu, '\t');
        }
        CHECK(menu.scope() == -1);
        press(menu, '8');
        CHECK(menu.scope() == 6);
        press(menu, '1');
        CHECK(menu.scope() == -1);
        press(menu, 27);
        CHECK(menu.page() == SettingsEditor::Page::Setup && menu.selected() == 3);
        row(menu, 8);
        const auto before = state();
        press(menu, '\n');
        press(menu, down);
        press(menu, '\t'); // Typing a tempo ignores Tab; Esc cancels it.
        CHECK(menu.tempoOpen() && state() == before);
        press(menu, 27);
        CHECK(menu.settingsOpen && !menu.tempoOpen() && state() == before && menu.selected() == 8);
    }
    {
        Capture captured; Instrument player(captured); SettingsEditor envelopes(player);
        player.library.entries[0].sound.envelopes[0].values[0] = 37;
        player.settings.envelopes[0].values = {200, 300, 65, 500};
        player.settings.envelopes[1].values[3] = 700;
        player.settings.echo = 25;
        press(envelopes, '\t'); row(envelopes, 23); press(envelopes, '\n');
        press(envelopes, '\b');
        CHECK(player.settings.envelopes[0].values[0] == 37);
        press(envelopes, '\n'); choose(envelopes, -1); press(envelopes, 27);
        CHECK(player.settings.envelopes[0].values[0] == 37);
        press(envelopes, '\n'); choose(envelopes, -1); press(envelopes, '\n');
        CHECK(player.settings.envelopes[0].values[0] == -1);
        for (int n = 0; n < 4; n++) press(envelopes, 18);
        press(envelopes, '\n'); choose(envelopes, 1); press(envelopes, '\n');
        CHECK(player.settings.envelopes[0] == player.library.entries[0].sound.envelopes[0]);
        CHECK(player.settings.envelopes[1].values[3] == 700 && player.settings.echo == 25);
    }
    {
        Capture captured; Instrument player(captured); SettingsEditor config(player);
        press(config, '\t'); row(config, 3); press(config, '\n');
        auto set = [&](int value) { press(config, '\n'); choose(config, value); press(config, '\n'); };
        CHECK(config.selected() == 6);
        set(4); CHECK(player.settings.chordRoot(0) == 62);
        press(config, '2'); set(2); CHECK(player.settings.chordRoot(0) == 38);
        press(config, '1'); set(5);
        CHECK(player.settings.chordRoot(0) == 38 && player.settings.chordRoot(1) == 76);
        press(config, '2'); press(config, '\b');
        CHECK(player.settings.degreeChordOctave[0] == -2 && player.settings.chordRoot(0) == 74);
        press(config, 18); CHECK(config.selected() == 8);
        set(6); CHECK(player.settings.chordRoot(0) == 78);
        press(config, '1'); CHECK(config.selected() == 7);
        set(2); CHECK(player.settings.tonic == 2);
        CHECK(player.settings.chordRoot(0) == 78 && player.settings.chordRoot(1) == 78);
        press(config, '2'); press(config, '\b');
        CHECK(player.settings.degreeChordRoot[0] == -1 && player.settings.chordRoot(0) == 76);
        set(1);
        saveSettings(player.settings, path); Settings restored; loadSettings(restored, path);
        CHECK(restored.chordOctave == 5 && restored.degreeChordOctave == player.settings.degreeChordOctave &&
              restored.degreeChordRoot == player.settings.degreeChordRoot);
        player.library.select(player.settings, 2); player.library.restore(player.settings);
        CHECK(player.settings.chordOctave == 5 && player.settings.degreeChordRoot[0] == 1 && player.settings.tonic == 2);
        player.settings.echo = 42;
        press(config, 17); CHECK(config.selected() == 6); press(config, '\b');
        CHECK(player.settings.degreeChordOctave[0] == -2 && player.settings.chordOctave == 5);
        press(config, 17); CHECK(config.selected() == 4); set(1);
        CHECK(player.settings.degreeChordRoot[0] == -1 && player.settings.chordOctave == 5 && player.settings.tonic == 2);
        press(config, '1'); set(1);
        CHECK(player.settings.chordOctave == 3 && player.settings.tonic == 0 && player.settings.echo == 42);
        press(config, 27); row(config, 22); press(config, '\n');
        set(1); // Front mode -> Scale notes.
        // Scale notes follow the chord key and scale until either front Key or Scale is set.
        press(config, 18); CHECK(config.selected() == 5);
        CHECK(player.settings.frontFollow && player.settings.frontKeyTonic() == player.settings.tonic);
        set(2);
        CHECK(!player.settings.frontFollow && player.settings.frontTonic == 2 &&
              player.settings.frontKeyScale() == 0 && player.settings.chordRoot(1) == 52);
        CHECK(frontNotes({48, -1, 6, Quality::Major}, player.settings)[0] == 62);
        press(config, '\b'); CHECK(player.settings.frontFollow);
        press(config, 18); CHECK(config.selected() == 4); set(1);
        CHECK(player.settings.scale == 1 && !player.settings.frontFollow &&
              player.settings.frontTonic == player.settings.tonic && player.settings.chordRoot(1) == 52);
        CHECK(frontNotes({48, -1, 6, Quality::Major}, player.settings)[2] == 63);
        press(config, '\b'); CHECK(player.settings.frontFollow && player.settings.frontKeyScale() == 0);
        player.settings.chordScale = 1; // Following picks up the chord's natural minor.
        CHECK(frontNotes({48, -1, 6, Quality::Major}, player.settings)[2] == 63);
    }
    // 1.2 settings files: Arp copies the Chord sound and envelope; Key and Scale keep sounding the same.
    for (int scale = 0; scale < 2; scale++) {
        auto file = path + ".legacy";
        std::ofstream(file) << "frontMappingVersion 2\ntonic 2\nscale " << scale
                            << "\nchordProgram 24\nenvelope13 300\nchordSound pack/strings\n";
        Settings loaded;
        loadSettings(loaded, file);
        CHECK(loaded.arpProgram == 24 && loaded.arpSound == "pack/strings" &&
              loaded.envelopes[ArpEnvelope].values[3] == 300);
        CHECK(loaded.chordScale == 0 && loaded.frontKeyTonic() == 2 && loaded.frontKeyScale() == scale &&
              loaded.frontFollow == (scale == 0));
        saveSettings(loaded, file);
        Settings again;
        loadSettings(again, file);
        CHECK(again.frontFollow == loaded.frontFollow && again.arpProgram == 24 &&
              again.frontKeyScale() == scale);
        std::filesystem::remove(file);
    }
    // Holding an arrow repeats after a delay, stops at the ends, and auditions once on release.
    {
        Capture captured;
        Instrument player(captured);
        SettingsEditor e(player);
        press(e, '\t');
        row(e, 4); // Backing volume, 80%.
        e.tick(1000);
        e.key('z', true);
        CHECK(player.settings.chordVolume == 75 && !captured.events.empty());
        captured.events.clear();
        e.tick(1390);
        CHECK(player.settings.chordVolume == 75);
        e.tick(1400);
        CHECK(player.settings.chordVolume == 70);
        for (uint64_t now = 1400; now <= 4000; now += 10) e.tick(now);
        CHECK(player.settings.chordVolume == 0 &&
              std::none_of(captured.events.begin(), captured.events.end(),
                           [](const Command &c) { return c.type == CommandType::On; }));
        e.key('z', false);
        CHECK(std::any_of(captured.events.begin(), captured.events.end(),
                          [](const Command &c) { return c.type == CommandType::On; }));
        e.tick(5000); // Released keys do not repeat.
        CHECK(player.settings.chordVolume == 0);
        e.key('x', true);
        for (uint64_t now = 5000; now <= 8000; now += 10) e.tick(now);
        e.key('x', false);
        CHECK(e.selected() == SettingsEditor::outputField); // Rows stop at the last one.
        e.key('f', true);
        e.key('f', false);
        CHECK(e.selected() == 28);
        e.key(19, true); // Left/right on a tabbed page never repeat.
        e.key(19, false);
        CHECK(player.settings.eqDb[2] == -1);
    }
    // A preview ends as soon as the player leaves its option, including back to playing.
    {
        Capture captured;
        Instrument player(captured);
        player.library.restore(player.settings);
        SettingsEditor e(player);
        press(e, 'o'); // Quick Backing from playing, nothing sounding yet.
        press(e, '2');
        press(e, 'x'); press(e, 'x');
        press(e, '\n'); // Chord envelope page.
        CHECK(e.page() == SettingsEditor::Page::Envelope);
        press(e, 20);
        CHECK(player.auditioning());
        press(e, '\t'); // No tabs here: the preview keeps playing.
        CHECK(player.auditioning());
        press(e, 18);
        CHECK(!player.auditioning()); // Moving to another row ends it.
        press(e, 20);
        CHECK(player.auditioning());
        press(e, 27);
        CHECK(!player.auditioning() && e.page() == SettingsEditor::Page::Backing);
        press(e, 'x'); press(e, '\n'); // Back to Envelope, then the playing screen.
        press(e, 20);
        CHECK(player.auditioning());
        press(e, 27); press(e, 27);
        CHECK(!e.settingsOpen && !player.auditioning());
        press(e, '\t'); // Front envelope: a previewed note stops when leaving the page.
        row(e, 23);
        press(e, '\n');
        press(e, 20);
        CHECK(player.auditioning());
        press(e, 27);
        CHECK(!player.auditioning());
        press(e, '\n'); press(e, '\n'); // A list preview stops when the list closes.
        CHECK(e.pickerOpen() && player.auditioning());
        press(e, 27);
        CHECK(!e.pickerOpen() && !player.auditioning());
    }
    // P and menu Tempo share a transactional numeric editor with no audio restart.
    {
        Capture captured;
        Instrument player(captured);
        SettingsEditor tempo(player);
        press(tempo, 'p');
        CHECK(tempo.tempoOpen() && !tempo.settingsOpen);
        press(tempo, '1'); press(tempo, '3'); press(tempo, '7'); press(tempo, '9');
        CHECK(tempo.tempoText() == "137" && player.settings.bpm == 100);
        press(tempo, '\n');
        CHECK(!tempo.tempoOpen() && player.settings.bpm == 137 && captured.events.empty());
        for (const auto text : {"", "39", "241"}) {
            press(tempo, 'p'); press(tempo, '\b');
            for (char c : std::string(text)) press(tempo, c);
            press(tempo, '\n');
            CHECK(tempo.tempoOpen() && !tempo.tempoError().empty() && player.settings.bpm == 137);
            press(tempo, 27);
        }
        press(tempo, 'p'); press(tempo, 'r');
        CHECK(tempo.tempoText() == "70");
        press(tempo, '\b'); press(tempo, '8'); press(tempo, '0'); press(tempo, '\n');
        CHECK(player.settings.bpm == 80 && captured.events.empty());
        // Swing: Down to Swing; Enter or left/right cycle Off, 8ths, 16ths. Down to Amount;
        // left/right (or Z/C) change it live in 5% steps. Esc keeps them; R loads the preset's.
        press(tempo, 'p'); press(tempo, 18);
        CHECK(tempo.tempoRow() == 1);
        press(tempo, '\n');
        CHECK(tempo.tempoOpen() && player.settings.swingMode == SwingEighths);
        press(tempo, 20);
        CHECK(player.settings.swingMode == SwingSixteenths);
        press(tempo, 20); press(tempo, 19);
        CHECK(player.settings.swingMode == SwingSixteenths);
        press(tempo, 18); press(tempo, 18);
        CHECK(tempo.tempoRow() == 2);
        press(tempo, 20); press(tempo, 20); press(tempo, 'c'); press(tempo, 'z');
        CHECK(player.settings.swingAmount == 60 && tempo.tempoText() == "80");
        press(tempo, '7'); // Typing returns to the tempo.
        CHECK(tempo.tempoRow() == 0 && tempo.tempoText() == "7");
        press(tempo, 27);
        CHECK(player.settings.swingMode == SwingSixteenths && player.settings.swingAmount == 60 &&
              player.settings.bpm == 80);
        press(tempo, 'p'); press(tempo, 18); press(tempo, 18);
        for (int n = 0; n < 20; n++) press(tempo, 20);
        CHECK(player.settings.swingAmount == 100);
        press(tempo, 'r');
        const auto &stock = player.library.current(player.settings).sound;
        CHECK(player.settings.swingMode == stock.swingMode && stock.swingMode == SwingOff &&
              player.settings.swingAmount == stock.swingAmount && stock.swingAmount == 50);
        press(tempo, 27);
        press(tempo, 'p'); press(tempo, '\t'); // Tab is ignored while typing a tempo.
        CHECK(tempo.tempoOpen() && !tempo.settingsOpen);
        press(tempo, 27);
        CHECK(!tempo.tempoOpen() && !tempo.settingsOpen && player.settings.bpm == 80);
    }
    std::filesystem::remove_all(dir);
    // Drums tab: Kit lists the built-in kits and applies live; Pattern lists every groove.
    {
        Capture captured; Instrument player(captured); SettingsEditor menu(player);
        press(menu, '\t');
        row(menu, SettingsEditor::drumKitField);
        CHECK(menu.page() == SettingsEditor::Page::Backing && menu.backingTab() == 5);
        const int kit = player.settings.drumKit;
        press(menu, '\n');
        CHECK(menu.pickerOpen() && !menu.soundPicker() && menu.choices().size() == 8);
        choose(menu, 25);
        CHECK(player.settings.drumKit == kit); // Highlighting previews without applying.
        press(menu, '\n');
        CHECK(player.settings.drumKit == 25 && player.settings.drumSound.empty());
        press(menu, '\b');
        CHECK(player.settings.drumKit == player.library.current(player.settings).sound.drumKit);
        // Without the FluidR3 pack, choosing the same built-in kit keeps the pack preference.
        player.settings.drumSound = "fluid-drums/jazz";
        player.settings.drumKit = 32;
        const auto preferred = player.settings.drumSound;
        press(menu, '\n'); choose(menu, player.settings.drumKit); press(menu, '\n');
        CHECK(player.settings.drumSound == preferred && !preferred.empty());
        press(menu, 18);
        CHECK(menu.selected() == SettingsEditor::drumPatternField);
        press(menu, '\n');
        CHECK(int(menu.choices().size()) == drumPatternCount);
        choose(menu, 5); press(menu, '\n');
        CHECK(player.settings.drumPattern == 5);
        auto file = path + ".drums";
        saveSettings(player.settings, file); Settings loaded; loadSettings(loaded, file);
        CHECK(loaded.drumKit == player.settings.drumKit && loaded.drumPattern == 5 &&
              loaded.mix(PlaybackMode::Drums) == player.settings.mix(PlaybackMode::Drums));
        // Chord Scale offers only seven-note scales; front Scale adds pentatonics and Blues.
        press(menu, 27);
        row(menu, 3); press(menu, '\n');
        for (int n = 0; n < 20 && menu.selected() != 10; n++) press(menu, 18);
        press(menu, '\n');
        CHECK(int(menu.choices().size()) == chordScaleCount);
        choose(menu, 4); press(menu, '\n');
        CHECK(player.settings.chordScale == 4 && player.settings.frontKeyScale() == 4);
    }
    {
        // Tap tempo: P on the Tempo page. From the second tap the tempo is the average of up
        // to the last four gaps and applies at once; a pause over 2 s starts over.
        Capture captured; Instrument player(captured); SettingsEditor menu(player);
        player.settings.bpm = 70;
        auto tapAt = [&](uint64_t at) { menu.tick(at); press(menu, 'p'); };
        tapAt(1000); // Opens the Tempo page.
        CHECK(menu.tempoOpen());
        tapAt(1100);
        CHECK(player.settings.bpm == 70); // One tap sets nothing yet.
        tapAt(1600);
        CHECK(player.settings.bpm == 120 && menu.tempoText() == "120");
        for (uint64_t at : {2100, 2600, 3100, 3500, 3900, 4300, 4700}) tapAt(at);
        CHECK(player.settings.bpm == 150); // The last four gaps of 400 ms.
        tapAt(7000); // After a pause: the first tap of a new count.
        CHECK(player.settings.bpm == 150);
        tapAt(7250);
        CHECK(player.settings.bpm == 240); // Clamped to the range.
        press(menu, 27); // Already applied: Esc keeps it.
        CHECK(!menu.tempoOpen() && player.settings.bpm == 240);
    }
    std::cout << "PASS: defaults, no3/sus voicings, shared/degree resolution, scope shortcuts, "
                 "preview/save/cancel, resets, performance and persistence\n";
}
