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
void press(SettingsEditor &editor, char key) {
    editor.key(key, true);
    editor.key(key, false);
}
void choose(SettingsEditor &editor, int value) {
    CHECK(editor.pickerOpen());
    for (size_t n = 0; n < editor.choices().size(); n++) {
        if (editor.choices()[editor.highlighted()].value == value)
            return;
        press(editor, 18);
    }
    CHECK(false);
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
    for (int n = 0; n < 3; n++)
        press(editor, 18);
    press(editor, '\n');
    CHECK(editor.page() == SettingsEditor::Page::Combinations);
    CHECK(editor.scope() == -1);
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
    // Reset all requires confirmation and preserves unrelated sound/key settings.
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
    CHECK(instrument.settings.tonic == 4);
    press(editor, 27);
    CHECK(editor.settingsOpen && editor.page() == SettingsEditor::Page::Setup);
    press(editor, 17);
    CHECK(editor.selected() == 2);
    press(editor, '\n');
    choose(editor, 2);
    CHECK(instrument.settings.preset == 0);
    editor.cancelPending(); // Same cleanup used on long Esc or signal exit.
    CHECK(instrument.settings.preset == 0);
    CHECK(sink.events.back().type == CommandType::Configure &&
          sink.events.back().settings.preset == 0);
    press(editor, '\n');
    choose(editor, 1);
    press(editor, '\n');
    CHECK(instrument.settings.preset == 1);
    press(editor, 20);
    CHECK(instrument.settings.preset == 2); // Direct right adjustment remains.
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
    CHECK(loaded.preset == 2 && loaded.tonic == 0);
    {
        std::ofstream out(path);
        out << "tonic 7\npreset 1\nbad data\ncombo3 999\ndegree6combo3 -9\nscale 1\n";
    }
    Settings legacy;
    loadSettings(legacy, path);
    CHECK(legacy.tonic == 7 && legacy.preset == 1 && legacy.scale == 1);
    CHECK(legacy.combinations == defaults.combinations &&
          legacy.degreeCombinations == defaults.degreeCombinations);
    std::filesystem::remove_all(dir);
    std::cout << "PASS: defaults, no3/sus voicings, shared/degree resolution, scope shortcuts, "
                 "preview/save/cancel, resets, performance and persistence\n";
}
