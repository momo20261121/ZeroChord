#include "ui.hpp"
#include <algorithm>
#include <lvgl.h>
#include <sstream>
namespace zc {
namespace {
uint32_t bg = 0x111923, panel = 0x202d3a, text = 0xe9edf0, muted = 0x90a1b2, gold = 0xffce75,
         cyan = 0x73d9d0;
lv_obj_t *box(int x, int y, int w, int h, uint32_t color) {
    auto *o = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, 4, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}
void label(int x, int y, int w, const std::string &value, uint32_t color = text,
           const lv_font_t *font = &lv_font_montserrat_10) {
    auto *o = lv_label_create(lv_screen_active());
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_label_set_long_mode(o, LV_LABEL_LONG_CLIP);
    lv_label_set_text(o, value.c_str());
}
} // namespace
void UI::drawCombinations() {
    const auto &s = instrument_.settings;
    label(8, 5, 210, "Chord combinations", gold, &lv_font_montserrat_14);
    label(222, 8, 92, scopeName(), muted);
    for (int n = 0; n < 8; n++) {
        int x = n == 0 ? 6 : 60 + (n - 1) * 36;
        int w = n == 0 ? 51 : 34;
        bool active = scope_ == n - 1;
        box(x, 27, w, 17, active ? gold : panel);
        label(x + 3, 30, w - 4, std::to_string(n + 1) + " " + (n == 0 ? "All" : degreeName(n - 1)),
              active ? bg : muted);
    }
    label(10, 49, 150, "COMBINATION", muted);
    label(172, 49, 140, "CHORD / SOURCE", muted);
    for (int n = 0; n < 4; n++) {
        int y = 64 + n * 18;
        bool selected = combination_ == n;
        if (selected)
            box(6, y - 3, 308, 18, panel);
        for (int row = 0; row < 3; row++)
            box(12, y - 1 + row * 5, 7, 4, combinationMasks[n] & (1u << row) ? gold : 0x43505e);
        label(26, y, 142, combinationName(n), selected ? gold : text);
        auto quality = s.quality(combinationMasks[n], scope_ < 0 ? 6 : scope_);
        // The shared view must ignore degree I's override.
        if (scope_ < 0)
            quality = Quality(s.combinations[n]);
        label(173, y, 48, qualityName(quality), text);
        label(224, y, 88,
              scope_ < 0 ? "Shared"
                         : (s.degreeCombinations[scope_][n] < 0 ? "Inherited" : "Custom"),
              scope_ >= 0 && s.degreeCombinations[scope_][n] >= 0 ? cyan : muted);
    }
    if (combination_ == 4)
        box(6, 134, 308, 19, panel);
    label(12, 138, 300, scope_ < 0 ? "Restore preset mappings..." : "Restore degree defaults...",
          combination_ == 4 ? gold : muted);
    label(8, 158, 307, "1-8 scope   Arrows move/change   Enter edit", muted);
}
void UI::drawPicker() {
    std::string title = page_ == Page::Setup
                            ? settingName(selection_)
                            : (reset_ ? "Reset mappings?" : "Choose chord / " + scopeName());
    label(8, 5, 260, title, gold, &lv_font_montserrat_14);
    label(273, 8, 41, std::to_string(choice_ + 1) + "/" + std::to_string(choices_.size()), muted);
    label(8, 26, 306,
          page_ == Page::Combinations
              ? (reset_ ? (scope_ < 0 ? "All degrees and overrides" : scopeName())
                        : combinationName(combination_))
              : "Choose a value; Enter saves, Esc cancels",
          muted);
    int start = std::clamp(choice_ - 2, 0, std::max(0, int(choices_.size()) - 5));
    for (int n = start; n < int(choices_.size()) && n < start + 5; n++) {
        int y = 47 + (n - start) * 18;
        bool selected = n == choice_;
        if (selected)
            box(6, y - 3, 308, 18, panel);
        label(12, y, 228, std::string(selected ? "> " : "  ") + choices_[n].name,
              selected ? gold : text, &lv_font_montserrat_12);
        if (!reset_ && choices_[n].value == defaultValue() &&
            !(page_ == Page::Setup && selection_ == 2))
            label(261, y + 1, 50, "Default", cyan);
        else if (n == savedChoice_)
            label(261, y + 1, 50, "Saved", muted);
    }
    std::string description = choices_[choice_].description;
    if (page_ == Page::Combinations && !reset_) {
        auto chord = previewChord();
        auto notes = chordNotes(chord);
        description = chordName(chord) + ": " + noteName(notes[0]) + " " + noteName(notes[1]);
        if (chord.quality != Quality::No3)
            description += " " + noteName(notes[2]);
        if (chord.quality == Quality::Seventh || chord.quality == Quality::Major7 ||
            chord.quality == Quality::Minor7)
            description += " " + noteName(notes[3]);
        description += "  |  " + choices_[choice_].description;
    }
    label(8, 138, 306, description, cyan);
    label(8, 158, 306, "Arrows choose   Enter save   Esc cancel", muted);
}
void UI::draw() {
    const auto &i = instrument_;
    const auto &s = i.settings;
    std::ostringstream signature;
    signature << settingsOpen << ',' << selection_ << ',' << s.tonic << ',' << s.scale << ','
              << s.preset << ',' << s.chordVolume << ',' << s.harpVolume << ',' << s.echo << ','
              << s.strumMs << ',' << s.bpm << ',' << s.pattern << ',' << i.active << ','
              << chordName(i.chord) << ',' << s.hold << ',' << int(s.mode) << ',' << i.harpFlash;
    signature << ',' << int(page_) << ',' << combination_ << ',' << scope_ << ',' << picker_ << ','
              << reset_ << ',' << choice_;
    for (auto value : s.combinations)
        signature << ',' << value;
    for (const auto &row : s.degreeCombinations)
        for (auto value : row)
            signature << ',' << value;
    signature << ',' << s.chordProgram << ',' << s.harpProgram << ',' << s.chordPattern << ','
              << s.padMotion << ',' << s.presetLevel << ',' << s.masterVolume << ',' << s.theme
              << ',' << status_ << ',' << instrument_.library.current(s).name;
    if (signature.str() == previous_)
        return;
    previous_ = signature.str();
    if (s.theme) {
        bg = 0xf3f5f7;
        panel = 0xe2e8ee;
        text = 0x17232e;
        muted = 0x52687b;
        gold = 0x805000;
        cyan = 0x006c72;
    } else {
        bg = 0x111923;
        panel = 0x202d3a;
        text = 0xe9edf0;
        muted = 0x90a1b2;
        gold = 0xffce75;
        cyan = 0x73d9d0;
    }
    lv_obj_clean(lv_screen_active());
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    if (settingsOpen && picker_) {
        drawPicker();
        return;
    }
    if (settingsOpen && page_ == Page::Combinations) {
        drawCombinations();
        return;
    }
    label(8, 5, 174, settingsOpen ? "ZeroChord / Setup" : "ZeroChord", gold,
          &lv_font_montserrat_14);
    label(188, 7, 124,
          noteName(s.tonic) + (s.scale ? " minor" : " major") + "  " +
              instrument_.library.current(s).name + (instrument_.library.modified(s) ? "*" : ""),
          muted);
    if (settingsOpen) {
        int start = (selection_ / 5) * 5;
        for (int n = start; n < settingCount && n < start + 5; n++) {
            int y = 29 + (n - start) * 22;
            bool selected = n == selection_;
            if (selected)
                box(6, y - 3, 308, 21, panel);
            label(12, y, 193,
                  std::string(selected ? "> " : "  ") + settingName(n) +
                      (fieldModified(n) ? " *" : ""),
                  selected ? gold : muted, &lv_font_montserrat_12);
            label(207, y + 1, 105, settingValue(n), selected ? text : muted);
        }
        label(8, 146, 306,
              selection_ == 17 || selection_ == 18
                  ? "App-wide setting   Enter choices   Esc back"
                  : "Arrows move/change   Enter choices   Esc back",
              muted);
        label(8, 159, 306,
              status_.empty() ? "Backspace: default   * edited   Hold Esc: home" : status_, muted);
        return;
    }
    box(8, 29, 84, 48, panel);
    label(15, 35, 74, i.active ? chordName(i.chord) : "Ready", i.active ? gold : text,
          &lv_font_montserrat_14);
    label(15, 58, 74,
          i.active ? std::string(degreeName(i.chord.column)) + " chord" : "Select chord", muted);
    static const char *rows[] = {"2345678", "QWERTYU", "ASDFGHJ"};
    for (int col = 0; col < 7; col++) {
        int x = 100 + col * 30;
        bool selected = i.active && col == i.chord.column;
        label(x + 5, 25, 28, degreeName(col), selected ? gold : muted);
        for (int row = 0; row < 3; row++) {
            int y = 39 + row * 17;
            box(x, y, 28, 15, selected ? (s.theme ? 0xf2ddb2 : 0x3e3a30) : panel);
            label(
                x + 3, y + 2, 25,
                std::string(1, rows[row][col]) + " " +
                    (row == 0 ? noteName(rootFor(s.tonic, s.scale, col)) : (row == 1 ? "m" : "7")),
                selected ? gold : text);
        }
    }
    label(8, 90, 305,
          std::string("I hold ") + (s.hold ? "ON" : "off") + "   O " + modeName(s.mode) + "   P " +
              std::to_string(s.bpm) + " BPM",
          muted);
    auto pitches = harpNotes(i.chord);
    const char *harp = "ZXCVBNM";
    for (int n = 0; n < 7; n++) {
        int x = 8 + n * 44;
        bool flash = i.harpFlash == n;
        box(x, 107, 40, 29, flash ? cyan : panel);
        label(x + 5, 110, 30,
              std::string(1, harp[n]) + " " + (i.active ? noteName(pitches[n]) : "-"),
              flash ? bg : cyan);
        label(x + 5, 123, 30, i.active ? std::to_string(pitches[n] / 12 - 1) : "",
              flash ? bg : muted);
    }
    label(8, 142, 306, "K down / L up   9/0 preset   1 sharp", text);
    label(8, 156, 306, "Tab setup   Space stop   Hold Esc: home", muted);
}
} // namespace zc
