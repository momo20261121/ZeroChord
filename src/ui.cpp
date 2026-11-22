#include "ui.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <cstring>
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
           const lv_font_t *font = &lv_font_montserrat_10,
           lv_text_align_t align = LV_TEXT_ALIGN_LEFT) {
    auto *o = lv_label_create(lv_screen_active());
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(o, align, 0);
    lv_label_set_long_mode(o, LV_LABEL_LONG_CLIP);
    lv_label_set_text(o, value.c_str());
}
// Every settings page uses the same dimensions, colors and centered tab labels.
void tabs(const std::vector<std::string> &names, int active) {
    const int count = int(names.size());
    for (int n = 0; n < count; n++) {
        int x = 6 + n * 308 / count;
        int width = (n + 1) * 308 / count - n * 308 / count - 2;
        box(x, 26, width, 18, n == active ? gold : panel);
        label(x + 2, 30, width - 4, names[n], n == active ? bg : muted,
              &lv_font_montserrat_10, LV_TEXT_ALIGN_CENTER);
    }
}
} // namespace
void UI::drawAbout() {
    label(8, 5, 304, "ZeroChord / Setup", gold, &lv_font_montserrat_14);
    tabs({"Style", "App", "About"}, int(tab_));
    label(10, 55, 188, "ZeroChord", text, &lv_font_montserrat_14);
    label(10, 77, 188, "Version " ZEROCHORD_VERSION, gold, &lv_font_montserrat_12);
    label(10, 97, 188, "Pocket chord instrument", muted);
    label(10, 120, 190, "github.com/", cyan);
    label(10, 134, 194, "momo20261121/ZeroChord", cyan);
    constexpr char url[] = "https://github.com/momo20261121/ZeroChord";
    auto *qr = lv_qrcode_create(lv_screen_active());
    lv_obj_set_pos(qr, 208, 48);
    lv_qrcode_set_size(qr, 106);
    // Keep high contrast and the scanner's quiet zone in both themes.
    lv_qrcode_set_dark_color(qr, lv_color_hex(0x000000));
    lv_qrcode_set_light_color(qr, lv_color_hex(0xffffff));
    lv_qrcode_set_quiet_zone(qr, true);
    if (lv_qrcode_update(qr, url, std::strlen(url)) != LV_RESULT_OK)
        label(210, 80, 100, "Scan unavailable", muted);
    label(8, 159, 306, LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " tab   Esc back   Hold Esc: home", muted);
}
void UI::drawEnvelope() {
    const auto &s = instrument_.settings;
    const auto &d = instrument_.library.current(s).sound;
    label(8, 5, 304, "Envelope", gold, &lv_font_montserrat_14);
    tabs({"Front", "Chord", "Pad"}, envelopePart_);
    for (int field = 0; field < 5; field++) {
        int y = 53 + field * 18;
        if (field == envelopeField_) box(6, y - 3, 308, 18, panel);
        bool edited = field < 4 && s.envelopes[envelopePart_].values[field] != d.envelopes[envelopePart_].values[field];
        label(12, y, 177, std::string(field == envelopeField_ ? "> " : "  ") + envelopeControlName(field) + (edited ? " *" : ""),
              field == envelopeField_ ? gold : muted);
        label(192, y, 120, field == 4 ? "Restore >" : envelopeValueName(field, s.envelopes[envelopePart_].values[field]));
    }
    label(8, 146, 306, "Backspace: preset default   * edited", muted);
    label(8, 159, 306, LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " part   " LV_SYMBOL_UP LV_SYMBOL_DOWN " row   Enter edit   Esc back", muted);
}
void UI::drawBacking() {
    label(8, 5, 304, "Backing layers", gold, &lv_font_montserrat_14);
    label(8, 26, 304, instrument_.library.current(instrument_.settings).name + " / choose any combination", muted);
    static const char *names[]{"Chord", "Pad", "Arp", "Apply"};
    static const char *details[]{"Simultaneous chord notes", "Sustained harmony", "Patterned chord notes", ""};
    for (int n = 0; n < 4; ++n) {
        int y = 49 + n * 20;
        if (n == backingRow_) box(6, y - 3, 308, 19, panel);
        std::string mark = n == 3 ? "> " : (pendingLayers_ & (1u << n)) ? "[x] " : "[ ] ";
        label(12, y, 100, mark + names[n], n == backingRow_ ? gold : text);
        label(112, y, 198, details[n], muted);
    }
    label(8, 134, 304, pendingLayers_ ? modeName(modeForLayers(pendingLayers_)) : "None selected: Front only", cyan);
    label(8, 148, 304, LV_SYMBOL_UP LV_SYMBOL_DOWN " row   Enter toggle / Apply   Esc cancel", muted);
    label(8, 160, 304, "Backspace: preset default", muted);
}
void UI::drawTempo() {
    label(8, 5, 304, "Tempo", gold, &lv_font_montserrat_14);
    label(8, 26, 304, instrument_.library.current(instrument_.settings).name + " / 40-240 BPM", muted);
    box(28, 50, 264, 52, panel);
    label(36, 67, 248, (tempoInput_.empty() ? "_" : tempoInput_) + " BPM",
          gold, &lv_font_montserrat_14, LV_TEXT_ALIGN_CENTER);
    label(8, 113, 304, tempoError_.empty() ? "Type a tempo; Enter saves, Esc cancels" : tempoError_,
          tempoError_.empty() ? muted : gold);
    label(8, 139, 304, "Backspace delete   R preset default", muted);
    label(8, 156, 304, "Enter save   Esc cancel", muted);
}
void UI::drawCombinations() {
    const auto &s = instrument_.settings;
    label(8, 5, 210, "Chord config", gold, &lv_font_montserrat_14);
    label(222, 8, 92, scopeName(), muted);
    std::vector<std::string> names{"1 All"};
    for (int n = 0; n < 7; n++)
        names.push_back(std::to_string(n + 2) + " " + degreeName(n));
    tabs(names, scope_ + 1);
    const auto rows = combinationRows();
    int row = int(std::find(rows.begin(), rows.end(), combination_) - rows.begin());
    int start = std::clamp(row - 2, 0, std::max(0, int(rows.size()) - 5));
    for (int index = start; index < int(rows.size()) && index < start + 5; index++) {
        int n = rows[index], y = 52 + (index - start) * 18;
        bool selected = combination_ == n;
        if (selected) box(6, y - 3, 308, 18, panel);
        if (n < 4) {
            for (int button = 0; button < 3; button++)
                box(12, y - 1 + button * 5, 7, 4, combinationMasks[n] & (1u << button) ? gold : 0x43505e);
            auto quality = scope_ < 0 ? Quality(s.combinations[n]) : s.quality(combinationMasks[n], scope_);
            label(26, y, 142, combinationName(n), selected ? gold : text);
            label(173, y, 48, qualityName(quality));
            label(224, y, 88, scope_ < 0 ? "Shared" : s.degreeCombinations[scope_][n] < 0 ? "Inherited" : "Custom", muted);
        } else {
            label(12, y, 177, combinationName(n), selected ? gold : muted);
            if (n == 6) {
                int value = chordOctaveValue(s);
                label(192, y, 120, value == -2 ? "All (" + std::to_string(s.chordOctave) + ")" : std::to_string(value));
            } else if (n == 7)
                label(192, y, 120, noteName(s.tonic));
            else if (n == 8)
                label(192, y, 120, s.degreeChordRoot[scope_] < 0 ? "Key (" + noteName(s.chordRoot(scope_)) + ")" : noteName(s.degreeChordRoot[scope_]));
            else if (n == 5)
                label(192, y, 120, s.combinationMs ? std::to_string(s.combinationMs) + " ms" : "Off");
            else label(192, y, 120, "Restore >");
        }
    }
    label(8, 143, 306, "Backspace: default   Esc back", muted);
    label(8, 158, 307, LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " / 1-8 tab   " LV_SYMBOL_UP LV_SYMBOL_DOWN " row   Enter edit", muted);
}
void UI::drawFrontNotes() {
    const auto &s = instrument_.settings;
    label(8, 5, 220, "Front notes", gold, &lv_font_montserrat_14);
    label(230, 8, 84, "App-wide", muted);
    const char *keys = "ZXCVBNM";
    std::vector<std::string> names{"All"};
    for (int n = 0; n < 7; n++) names.push_back(std::string(1, keys[n]));
    tabs(names, frontKey_ + 1);
    const auto rows = frontRows();
    Settings defaults;
    defaults.frontMode = s.frontMode;
    for (int row = 0; row < int(rows.size()); row++) {
        int n = rows[row];
        int y = 54 + row * 18;
        if (n == frontField_)
            box(6, y - 3, 308, 18, panel);
        bool modified =
            !((frontKey_ < 0 && n == 3) || (frontKey_ >= 0 && n == 2)) &&
            frontValue(s, n) != frontValue(frontKey_ < 0 && n == 0 ? Settings{} : defaults, n);
        label(12, y, 176,
              std::string(n == frontField_ ? "> " : "  ") + frontSettingName(n) +
                  (modified ? " *" : ""),
              n == frontField_ ? gold : muted);
        label(190, y, 122, frontValueName(s, n), text);
    }
    Chord c{s.chordRoot(6), -1, 6, Quality::Major};
    int pitch = frontNotes(c, s)[std::max(frontKey_, 0)];
    std::string hint = frontKey_ < 0 ? "Each mode remembers its K/L action and time"
                                     : std::string(1, keys[frontKey_]) + " = " + noteName(pitch) +
                                           std::to_string(pitch / 12 - 1) + "   / " +
                                           (s.frontMode ? "selected scale" : chordName(c)) +
                                           "   Bksp: default";
    label(8, 142, 306, hint, cyan);
    label(8, 158, 306, LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " tab   " LV_SYMBOL_UP LV_SYMBOL_DOWN " row   Enter edit", muted);
}
void UI::drawPicker() {
    std::string title = page_ == Page::Setup
                            ? settingName(selection_)
                            : (reset_ ? "Restore chord config?" : "Choose chord / " + scopeName());
    if (page_ == Page::Envelope)
        title = std::string(envelopePartName(envelopePart_)) + " / " + envelopeControlName(envelopeField_);
    if (page_ == Page::FrontNotes)
        title = (frontKey_ < 0 ? "All" : std::string("ZXCVBNM").substr(frontKey_, 1)) +
                std::string(" / ") + frontSettingName(frontField_);
    if (page_ == Page::Combinations && combination_ >= 5)
        title = std::string(combinationName(combination_)) + " / " + scopeName();
    label(8, 5, 260, title, gold, &lv_font_montserrat_14);
    label(273, 8, 41, std::to_string(choice_ + 1) + "/" + std::to_string(choices_.size()), muted);
    if (soundPicker()) {
        auto categories = soundCategories();
        tabs(categories, int(std::find(categories.begin(), categories.end(), soundCategory_) -
                             categories.begin()));
    } else {
        label(8, 26, 306,
              page_ == Page::Combinations
                  ? (reset_ ? (scope_ < 0 ? "All degrees and overrides" : scopeName())
                            : combinationName(combination_))
                  : "Choose a value; Enter saves, Esc cancels",
              muted);
    }
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
        else if (soundPicker() ? choices_[n].value == settingValueNumber() : n == savedChoice_)
            label(261, y + 1, 50, "Saved", muted);
    }
    std::string description = choices_[choice_].description;
    if (page_ == Page::Combinations && combination_ < 4) {
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
    label(8, 158, 306,
          soundPicker() ? LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " group  " LV_SYMBOL_UP LV_SYMBOL_DOWN " sound  Enter save  Esc back"
                        : LV_SYMBOL_UP LV_SYMBOL_DOWN " choose   Enter save   Esc cancel",
          muted);
}
void UI::draw() {
    const auto &i = instrument_;
    const auto &s = i.settings;
    std::ostringstream signature;
    signature << settingsOpen << ',' << selection_ << ',' << s.tonic << ',' << s.scale << ','
              << s.preset << ',' << s.chordVolume << ',' << s.harpVolume << ',' << s.echo << ','
              << s.strumMs << ',' << s.bpm << ',' << s.pattern << ',' << i.active << ','
              << chordName(i.chord) << ',' << s.hold << ',' << int(s.mode) << ',' << i.harpFlash
              << ',' << s.combinationMs << ',' << i.semitoneShift();
    signature << ',' << backing_ << ',' << pendingLayers_ << ',' << backingRow_;
    signature << ',' << tempo_ << ',' << tempoInput_ << ',' << tempoError_ << ',' << int(page_) << ',' << combination_ << ',' << scope_ << ',' << picker_ << ','
              << reset_ << ',' << choice_ << ',' << int(tab_);
    for (auto value : s.combinations)
        signature << ',' << value;
    for (const auto &row : s.degreeCombinations)
        for (auto value : row)
            signature << ',' << value;
    signature << ',' << s.padProgram << ',' << s.chordProgram << ',' << s.harpProgram << ','
              << s.chordPattern << ',' << s.padMotion << ',' << s.masterVolume << ',' << s.theme
              << ',' << status_ << ',' << instrument_.library.current(s).name << ','
              << soundCategory_ << ',' << frontKey_ << ',' << frontField_ << ',' << s.padSound
              << ',' << s.chordSound << ',' << s.frontSound << ',' << s.frontMode << ','
              << s.frontAction[0] << ',' << s.frontAction[1] << ',' << s.scaleStrumMs << ','
              << i.frontOctaveShift[0] << ',' << i.frontOctaveShift[1];
    for (int n = 0; n < 7; n++)
        signature << ',' << s.frontSource[n] << ',' << s.frontOffset[n] << ',' << s.frontOctave[n]
                  << ',' << s.frontDegree[n] << ',' << s.scaleOctave[n];
    signature << ',' << s.chordOctave;
    for (int gain : s.eqDb) signature << ',' << gain;
    for (int octave : s.degreeChordOctave) signature << ',' << octave;
    for (int root : s.degreeChordRoot) signature << ',' << root;
    signature << ',' << envelopePart_ << ',' << envelopeField_;
    for (const auto &envelope : s.envelopes)
        for (int value : envelope.values) signature << ',' << value;
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
    if (backing_) {
        drawBacking();
        return;
    }
    if (tempo_) {
        drawTempo();
        return;
    }
    if (settingsOpen && picker_) {
        drawPicker();
        return;
    }
    if (settingsOpen && page_ == Page::Envelope) {
        drawEnvelope();
        return;
    }
    if (settingsOpen && page_ == Page::FrontNotes) {
        drawFrontNotes();
        return;
    }
    if (settingsOpen && page_ == Page::Combinations) {
        drawCombinations();
        return;
    }
    if (settingsOpen && tab_ == Tab::About) {
        drawAbout();
        return;
    }
    label(8, 5, 174, settingsOpen ? "ZeroChord / Setup" :
              (i.semitoneShift() ? "ZeroChord  +1" : "ZeroChord"), gold,
          &lv_font_montserrat_14);
    label(188, 7, 124,
          noteName(s.tonic) + (s.frontMode ? (s.scale ? " min" : " maj") : "") + "  " +
              instrument_.library.current(s).name + (instrument_.library.modified(s) ? "*" : ""),
          muted);
    if (settingsOpen) {
        tabs({"Style", "App", "About"}, int(tab_));
        int row = rowIndex();
        int start = std::clamp(row - 2, 0, std::max(0, int(rows().size()) - 5));
        for (int n = start; n < int(rows().size()) && n < start + 5; n++) {
            int field = rows()[n];
            int y = 51 + (n - start) * 18;
            bool selected = field == selection_;
            if (selected)
                box(6, y - 3, 308, 18, panel);
            label(12, y, 193,
                  std::string(selected ? "> " : "  ") + settingName(field) +
                      (fieldModified(field) ? " *" : ""),
                  selected ? gold : muted, &lv_font_montserrat_12);
            label(207, y + 1, 105, settingValue(field), selected ? text : muted);
        }
        label(8, 146, 306, LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " tab   " LV_SYMBOL_UP LV_SYMBOL_DOWN " row   Enter edit   Esc back", muted);
        label(8, 159, 306,
              status_.empty() ? (tab_ == Tab::App && !soundLibrary().warning.empty()
                                     ? "Some sound packs skipped; see startup log"
                                     : "Backspace: default   * edited   Hold Esc: home")
                              : status_,
              muted);
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
                    (row == 0 ? noteName(s.chordRoot(col) + i.semitoneShift()) : (row == 1 ? "m" : "7")),
                selected ? gold : text);
        }
    }
    std::string backingHint;
    unsigned layers = backingLayers(s.mode);
    if (layers & 1) backingHint += "C";
    if (layers & 2) backingHint += "P";
    if (layers & 4) backingHint += "A";
    if (backingHint.empty()) backingHint = "Front";
    else if (layers == 1 || layers == 2 || layers == 4) backingHint = modeName(s.mode);
    label(8, 90, 210,
          std::string("I ") + (s.hold ? "hold" : "off") + "  O " + backingHint + "  P " +
              std::to_string(s.bpm), muted);
    label(220, 90, 94, s.frontAction[s.frontMode] ? "K/L oct -/+" : "K/L strum", cyan);
    auto pitches = i.playingFrontNotes();
    bool frontReady = i.active || s.frontMode == 1;
    const char *harp = "ZXCVBNM";
    for (int n = 0; n < 7; n++) {
        int x = 8 + n * 44;
        bool flash = i.harpFlash == n;
        box(x, 107, 40, 31, flash ? cyan : panel);
        label(x + 2, 110, 36,
              frontReady ? noteName(pitches[n]) + std::to_string(pitches[n] / 12 - 1) : "-",
              flash ? bg : cyan, &lv_font_montserrat_12, LV_TEXT_ALIGN_CENTER);
        label(x + 2, 125, 36, std::string(1, harp[n]), flash ? bg : muted,
              &lv_font_montserrat_10, LV_TEXT_ALIGN_CENTER);
    }
    label(8, 142, 306, "9/0 preset   Hold 1: +1 semitone", text);
    label(8, 156, 306, "Tab setup   Space stop   Hold Esc: home", muted);
}
} // namespace zc
