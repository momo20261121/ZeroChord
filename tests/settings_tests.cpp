#include "settings_editor.hpp"
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
void row(SettingsEditor &editor, int field) {
    if ((editor.tab() == SettingsEditor::Tab::App) != SettingsEditor::appField(field))
        press(editor, 20);
    for (int n = 0; editor.selected() != field && n < SettingsEditor::settingCount; n++)
        press(editor, 18);
    CHECK(editor.selected() == field);
}
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
            press(editor, 20);
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
    SettingsEditor editor(instrument);
    press(editor, '\t');
    CHECK(editor.selected() == 2); // Preset is first on Style.
    CHECK((editor.rows() == std::vector<int>{2, 8, 10, 9, 11, 12, 14, 23, 13, 24, 21, 25, 19, 20}));
    const auto original = instrument.settings;
    press(editor, 20);
    CHECK(editor.tab() == SettingsEditor::Tab::App && editor.selected() == 18);
    CHECK((editor.rows() == std::vector<int>{18, 17, 4, 5, 6, 15, 22, 3}));
    for (int field : {4, 5, 6}) {
        row(editor, field);
        press(editor, '\n');
        choose(editor, 30);
        press(editor, '\n');
        press(editor, '\b');
    }
    CHECK(instrument.settings.chordVolume == defaults.chordVolume &&
          instrument.settings.harpVolume == defaults.harpVolume &&
          instrument.settings.echo == defaults.echo &&
          instrument.settings.strumMs == defaults.strumMs);
    press(editor, 19);
    CHECK(editor.tab() == SettingsEditor::Tab::Style && editor.selected() == 2);
    CHECK(samePresetSettings(original, instrument.settings));
    row(editor, 3);
    press(editor, '\n');
    CHECK(editor.page() == SettingsEditor::Page::Combinations);
    CHECK(editor.selected() == 6);
    press(editor, 18); // Octave, Key/Root, combinations.
    press(editor, 18);
    CHECK(editor.scope() == -1);
    press(editor, 19);
    CHECK(editor.scope() == 6);
    press(editor, 20);
    CHECK(editor.scope() == -1);
    press(editor, 20);
    CHECK(editor.scope() == 0);
    press(editor, '5');
    CHECK(editor.scope() == 3);
    press(editor, 19);
    CHECK(editor.scope() == 2);
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
    CHECK(instrument.settings.tonic == 0);
    instrument.settings.tonic = 4;
    CHECK(instrument.settings.combinationMs == 30);
    for (int n = 0; n < 7; n++)
        press(editor, 18);
    CHECK(editor.selected() == 5 && editor.scope() == -1);
    sink.events.clear();
    press(editor, '\n');
    choose(editor, 45);
    CHECK(sink.events.empty() && instrument.settings.combinationMs == 30);
    press(editor, 27);
    press(editor, '\n');
    choose(editor, 45);
    press(editor, '\n');
    CHECK(instrument.settings.combinationMs == 45);
    press(editor, '\b');
    CHECK(instrument.settings.combinationMs == 30);
    press(editor, 20);
    CHECK(editor.scope() == 0 && editor.selected() == 0);
    press(editor, '1');
    CHECK(editor.scope() == -1);
    press(editor, 27);
    CHECK(editor.settingsOpen && editor.page() == SettingsEditor::Page::Setup);
    row(editor, 2);
    CHECK(editor.selected() == 2);
    press(editor, '\n');
    choose(editor, 2);
    CHECK(instrument.settings.preset == 1);
    editor.cancelPending(); // Same cleanup used on long Esc or signal exit.
    CHECK(instrument.settings.preset == 1);
    CHECK(sink.events.back().type == CommandType::Configure &&
          sink.events.back().settings.preset == 1);
    press(editor, '\n');
    choose(editor, 1);
    press(editor, '\n');
    CHECK(instrument.settings.preset == 1);
    press(editor, 20);
    CHECK(instrument.settings.preset == 1); // Right switches tabs without editing.
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
    saveSettings(instrument.settings, path);
    Settings loaded;
    loadSettings(loaded, path);
    CHECK(loaded.combinations == instrument.settings.combinations);
    CHECK(loaded.degreeCombinations == instrument.settings.degreeCombinations);
    CHECK(loaded.preset == 2 && loaded.tonic == 4);
    {
        std::ofstream out(path);
        out << "tonic 7\npreset 1\nbad data\ncombo3 999\ndegree6combo3 -9\nscale 1\n";
    }
    Settings legacy;
    loadSettings(legacy, path);
    CHECK(legacy.tonic == 7 && legacy.preset == 1 && legacy.scale == 1);
    CHECK(legacy.combinations == defaults.combinations &&
          legacy.degreeCombinations == defaults.degreeCombinations);
    // Audit every page/picker with both desktop arrow events and the physical
    // Cardputer's printed F/X/Z/C arrows. Previews must never commit settings.
    for (const auto nav : {std::array<char, 4>{17, 18, 19, 20},
                           std::array<char, 4>{'f', 'x', 'z', 'c'}}) {
        Capture captured;
        Instrument walker(captured);
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
            const int candidate = menu.choices()[(start + 1) % menu.choices().size()].value;
            press(menu, down);
            CHECK(menu.highlighted() == (start + 1) % int(menu.choices().size()));
            press(menu, up);
            CHECK(menu.highlighted() == start);
            if (menu.soundPicker()) {
                const auto category = menu.soundCategory();
                press(menu, right);
                CHECK(menu.soundCategory() != category && !menu.choices().empty());
                press(menu, left);
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
            if (!menu.action() && !(page == SettingsEditor::Page::Setup && row == 2)) {
                press(menu, '\b');
                CHECK(menu.choices()[menu.highlighted()].value == menu.defaultChoice());
            }
            press(menu, 27);
            CHECK(!menu.pickerOpen() && menu.settingsOpen && state() == before);
            if (!menu.action()) {
                press(menu, '\n');
                choose(menu, candidate);
                press(menu, '\n');
                CHECK(!menu.pickerOpen() && menu.currentValue() == candidate);
                walker.settings = saved;
                walker.configure();
            }
        };
        press(menu, '\t');
        for (int tab = 0; tab < 2; tab++) {
            const auto list = menu.visibleRows();
            for (size_t n = 0; n < list.size(); n++) {
                CHECK(menu.selected() == list[n]);
                if (menu.selected() >= 23) {
                    const int envelopeRow = menu.selected();
                    press(menu, '\n');
                    CHECK(menu.page() == SettingsEditor::Page::Envelope &&
                          menu.envelopePart() == envelopeRow - 23);
                    for (int field = 0; field < 5; field++) {
                        CHECK(menu.selected() == field);
                        checkPicker();
                        press(menu, down);
                    }
                    press(menu, left); press(menu, right);
                    CHECK(menu.envelopePart() == envelopeRow - 23);
                    press(menu, 27);
                    CHECK(menu.page() == SettingsEditor::Page::Setup && menu.selected() == envelopeRow);
                } else if (menu.selected() != 3 && menu.selected() != 22)
                    checkPicker();
                press(menu, down);
            }
            CHECK(menu.selected() == list[0]);
            press(menu, up);
            CHECK(menu.selected() == list.back());
            press(menu, down);
            press(menu, right);
        }
        row(menu, 22);
        for (int mode = 0; mode < 2; mode++) {
            walker.settings.frontMode = mode;
            press(menu, '\n');
            while (menu.frontKey() != -1)
                press(menu, right);
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
                press(menu, right);
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
            press(menu, right);
        }
        CHECK(menu.scope() == -1);
        press(menu, left);
        CHECK(menu.scope() == 6);
        press(menu, '1');
        CHECK(menu.scope() == -1);
        press(menu, 27);
        CHECK(menu.page() == SettingsEditor::Page::Setup && menu.selected() == 3);
        row(menu, 8);
        const auto before = state();
        press(menu, '\n');
        press(menu, down);
        press(menu, '\t');
        CHECK(!menu.settingsOpen && !menu.pickerOpen() && !menu.tempoOpen() && state() == before);
        press(menu, '\t');
        CHECK(menu.settingsOpen && menu.selected() == 8);
    }
    {
        Capture captured; Instrument player(captured); SettingsEditor envelopes(player);
        player.library.entries[1].sound.envelopes[0].values[0] = 37;
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
        CHECK(player.settings.envelopes[0] == player.library.entries[1].sound.envelopes[0]);
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
        press(config, 18); CHECK(config.selected() == 4); set(1);
        CHECK(player.settings.scale == 1 && player.settings.chordRoot(1) == 52);
        CHECK(frontNotes({48, -1, 6, Quality::Major}, player.settings)[2] == 63);
        press(config, '\b'); CHECK(player.settings.scale == 0);
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
        CHECK(tempo.tempoText() == "100");
        press(tempo, '\b'); press(tempo, '8'); press(tempo, '0'); press(tempo, '\n');
        CHECK(player.settings.bpm == 80 && captured.events.empty());
        press(tempo, 'p'); press(tempo, '\t');
        CHECK(!tempo.tempoOpen() && tempo.settingsOpen);
    }
    std::filesystem::remove_all(dir);
    std::cout << "PASS: defaults, no3/sus voicings, shared/degree resolution, scope shortcuts, "
                 "preview/save/cancel, resets, performance and persistence\n";
}
