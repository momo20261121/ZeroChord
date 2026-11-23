#include "ui.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <lvgl.h>
#include <sstream>
namespace zc {
namespace {
struct Palette {
    uint32_t bg, panel, text, muted, accent, signal, keyFace, keyText, activeChord, frontPanel;
};
constexpr Palette dark{0x20192c, 0x30263f, 0xf9f0e8, 0xc1b3c8, 0xffc18d, 0x84e0bf,
                       0x2d4a49, 0xc3f5df, 0x50333f, 0x29373f};
constexpr Palette light{0xfff5e6, 0xefe2e8, 0x30213c, 0x5f4d65, 0x882f61, 0x0a6352,
                        0xd2e9da, 0x174d43, 0xf7d3d2, 0xe8e9da};
Palette colors = dark;
constexpr auto *small = &lv_font_montserrat_10;
constexpr auto *body = &lv_font_montserrat_12;
constexpr auto *titleFont = &lv_font_montserrat_14;
constexpr auto *mono = &lv_font_unscii_8;

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
lv_obj_t *label(int x, int y, int w, const std::string &value, uint32_t color = colors.text,
                const lv_font_t *font = small, lv_text_align_t align = LV_TEXT_ALIGN_LEFT) {
    auto *o = lv_label_create(lv_screen_active());
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(o, align, 0);
    lv_label_set_long_mode(o, LV_LABEL_LONG_CLIP);
    lv_label_set_text(o, value.c_str());
    return o;
}

int textWidth(const std::string &value, const lv_font_t *font) {
    lv_point_t size{};
    lv_text_get_size(&size, value.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}

constexpr int tabsX = 6;
constexpr int tabsWidth = 308;

int tabX(int index, int count) {
    return tabsX + index * tabsWidth / count;
}

void tabs(const std::vector<std::string> &names, int active, bool numbered = false) {
    const int count = int(names.size());
    for (int n = 0; n < count; n++) {
        int x = tabX(n, count);
        int width = tabX(n + 1, count) - x - 2;
        const bool selected = n == active;
        const bool key = names[n].size() == 1;
        const bool fixedWidth = key || (numbered && n != 0);
        box(x, 26, width, 18, selected ? colors.accent : colors.panel);
        if (numbered)
            label(x + 2, 29, 7, std::to_string(n + 1), selected ? colors.bg : colors.signal);
        int inset = numbered ? 10 : 2;
        label(x + inset, fixedWidth ? 31 : 29, width - inset - 1, names[n],
              selected ? colors.bg : key && !numbered ? colors.keyText : colors.text,
              fixedWidth ? mono : small, LV_TEXT_ALIGN_CENTER);
    }
}

struct Hint {
    const char *key;
    std::string action;
};
constexpr char horizontalKeys[] = LV_SYMBOL_LEFT LV_SYMBOL_RIGHT;
constexpr char verticalKeys[] = LV_SYMBOL_UP LV_SYMBOL_DOWN;
const Hint tabHint{horizontalKeys, "Tab"};
const Hint rowHint{verticalKeys, "Row"};
const Hint backHint{"Esc", "Back"};
const Hint cancelHint{"Esc", "Cancel"};
const Hint defaultHint{"Backspace", "Reset field"};
const Hint editedHint{"*", "Edited"};
constexpr int menuDetailY = 132;
constexpr int visibleRows = 5;
constexpr int rowHeight = 18;
const std::vector<std::string> setupTabNames{"Style", "App", "About"};
constexpr char frontKeys[] = "ZXCVBNM";

int firstVisibleRow(int selected, int count, int limit = visibleRows) {
    return std::clamp(selected - limit / 2, 0, std::max(0, count - limit));
}

void scrollIndicator(int top, int first, int count, int limit = visibleRows, int spacing = rowHeight) {
    if (count <= limit) return;
    const int height = limit * spacing;
    const int thumb = height * limit / count;
    const int offset = (height - thumb) * first / (count - limit);
    box(316, top, 2, height, colors.panel);
    box(316, top + offset, 2, thumb, colors.muted);
}

void keycap(int x, int y, int width, const std::string &key, const lv_font_t *font,
            uint32_t face = colors.keyFace, uint32_t ink = colors.keyText, int height = 13) {
    auto *o = label(x, y, width, key, ink, font, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_height(o, height);
    lv_obj_set_style_pad_top(o, (height - font->line_height) / 2, 0);
    lv_obj_set_style_radius(o, 3, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(face), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
}

void hintLine(int y, std::initializer_list<Hint> hints, const lv_font_t *font = body,
              bool caps = true, int gap = 8) {
    int x = 8;
    for (const auto &hint : hints) {
        if (!hint.key) continue;
        const auto *keyFont = caps && std::strlen(hint.key) == 1 ? mono : small;
        const int keyWidth = textWidth(hint.key, keyFont) + (caps ? 8 : 0);
        if (caps) keycap(x, y, keyWidth, hint.key, keyFont);
        else label(x, y, keyWidth, hint.key, colors.signal, keyFont);
        x += keyWidth + 4;
        const int actionWidth = textWidth(hint.action, font);
        label(x, y + (caps ? (font == body ? -1 : 1) : 0), actionWidth, hint.action, colors.muted, font);
        x += actionWidth + gap;
    }
}

void menuHints(std::initializer_list<Hint> primary, std::initializer_list<Hint> secondary = {}) {
    hintLine(143, secondary, small, false);
    hintLine(155, primary);
}

void rowFocus(int y, int height) {
    box(6, y - 1, 308, height, colors.panel);
    box(8, y + 2, 2, height - 6, colors.accent);
}

void fieldRow(int y, int height, const std::string &name, const std::string &value, bool selected) {
    if (selected) rowFocus(y, height);
    label(16, y, 159, name, selected ? colors.accent : colors.text, body);
    lv_label_set_long_mode(label(180, y, 132, value, selected ? colors.text : colors.muted, body),
                           LV_LABEL_LONG_DOT);
}

constexpr int statusFieldInset = 6;
constexpr int statusFieldGap = 4;
constexpr int statusEditedWidth = 8;

int statusFieldWidth(const char *name, const std::string &value, bool edited = false) {
    return 2 * statusFieldInset + textWidth(name, small) + statusFieldGap + textWidth(value, body) +
           (edited ? statusEditedWidth : 0);
}

void statusField(int x, int width, const char *name, const std::string &value, bool edited = false) {
    box(x, 4, width, 17, colors.panel);
    const int nameWidth = textWidth(name, small);
    label(x + statusFieldInset, 7, nameWidth, name, colors.muted);
    const int valueOffset = statusFieldInset + nameWidth + statusFieldGap;
    const int valueWidth = width - valueOffset - statusFieldInset - (edited ? statusEditedWidth : 0);
    auto *valueLabel = label(x + valueOffset, 5, valueWidth, value, colors.text, body);
    lv_obj_set_height(valueLabel, body->line_height);
    lv_label_set_long_mode(valueLabel, LV_LABEL_LONG_DOT);
    if (edited)
        label(x + width - statusFieldInset - statusEditedWidth + 2, 6, statusEditedWidth, "*", colors.accent);
}
} // namespace
const char *UI::enterAction() const {
    if (backing_)
        return backingRow_ == backingApplyRow ? "Apply" : "Toggle";
    if (tempo_) return "Apply";
    const auto action = confirmation();
    if (picker_) {
        if (action == Confirmation::None) return "Apply";
        if (choices_[choice_].value == 0) return "Keep";
        return action == Confirmation::Create ? "Create" : "Restore";
    }
    if (page_ == Page::Setup && setupDestination() != SetupDestination::Picker) return "Open";
    return action == Confirmation::None ? "Change" : "Review";
}
void UI::drawAbout() {
    label(8, 5, 304, "ZeroChord / Setup", colors.accent, titleFont);
    tabs(setupTabNames, int(tab_));
    label(10, 52, 188, "ZeroChord", colors.text, titleFont);
    label(10, 71, 188, "Version " ZEROCHORD_VERSION, colors.accent, body);
    label(10, 88, 188, "Pocket chord instrument", colors.muted);
    label(10, 107, 190, "User guide and details", colors.muted);
    label(10, 119, 190, "github.com/", colors.signal);
    label(10, 130, 194, "momo20261121/ZeroChord", colors.signal);
    constexpr char url[] = "https://github.com/momo20261121/ZeroChord";
    auto *qr = lv_qrcode_create(lv_screen_active());
    lv_obj_set_pos(qr, 208, 48);
    lv_qrcode_set_size(qr, 106);
    // Keep high contrast and the scanner's quiet zone in both themes.
    lv_qrcode_set_dark_color(qr, lv_color_hex(0x000000));
    lv_qrcode_set_light_color(qr, lv_color_hex(0xffffff));
    lv_qrcode_set_quiet_zone(qr, true);
    if (lv_qrcode_update(qr, url, std::strlen(url)) != LV_RESULT_OK)
        label(210, 80, 100, "Scan unavailable", colors.muted);
    menuHints({tabHint, backHint});
}
void UI::drawSettingRows(int top, int limit, int spacing) {
    const auto &fields = rows();
    const int first = firstVisibleRow(rowIndex(), int(fields.size()), limit);
    for (int n = first; n < int(fields.size()) && n < first + limit; n++) {
        int field = fields[n];
        int y = top + (n - first) * spacing;
        fieldRow(y, spacing, std::string(settingName(field)) + (fieldModified(field) ? " *" : ""),
                 settingValue(field), field == selection_);
    }
    scrollIndicator(top - 1, first, int(fields.size()), limit, spacing);
}
void UI::drawVolumeEffects() {
    label(8, 5, 222, settingName(volumeEffectsField), colors.accent, titleFont);
    label(230, 8, 84, "All presets", colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    drawSettingRows(24, 7, 15);
    if (!status_.empty()) label(8, menuDetailY, 306, status_, colors.signal);
    menuHints({rowHint, {"Enter", enterAction()}, backHint}, {defaultHint, editedHint});
}
void UI::drawEnvelope() {
    const auto &s = instrument_.settings;
    const auto &d = instrument_.library.current(s).sound;
    label(8, 5, 304, "Envelope", colors.accent, titleFont);
    tabs({envelopePartName(0), envelopePartName(1), envelopePartName(2)}, envelopePart_);
    for (int field = 0; field < 5; field++) {
        int y = 46 + field * rowHeight;
        bool edited = field < 4 && s.envelopes[envelopePart_].values[field] != d.envelopes[envelopePart_].values[field];
        fieldRow(y, rowHeight, std::string(envelopeControlName(field)) + (edited ? " *" : ""),
                 field == 4 ? "Restore >" : envelopeValueName(field, s.envelopes[envelopePart_].values[field]),
                 field == envelopeField_);
    }
    if (!status_.empty()) label(8, menuDetailY, 306, status_, colors.signal);
    menuHints({{horizontalKeys, "Part"}, rowHint, {"Enter", enterAction()}, backHint},
              {{canRestoreField() ? "Backspace" : nullptr, "Reset to preset default"}, editedHint});
}
void UI::drawBacking() {
    label(8, 5, 304, "Backing layers", colors.accent, titleFont);
    label(8, 26, 304, instrument_.library.current(instrument_.settings).name + " / Choose any combination", colors.muted);
    for (int n = 0; n <= backingApplyRow; ++n) {
        int y = 46 + n * 20;
        bool selected = n == backingRow_;
        if (selected) rowFocus(y, rowHeight);
        if (n == backingApplyRow) {
            label(16, y, 100, "Apply", selected ? colors.accent : colors.text, body);
            break;
        }
        const auto &row = backingLayerRows[n];
        bool checked = pendingLayers_ & backingLayers(row.mode);
        keycap(12, y + 1, 14, checked ? LV_SYMBOL_OK : "", small, colors.keyFace);
        label(32, y, 76, modeName(row.mode), selected ? colors.accent : colors.text, body);
        label(112, y, 198, row.description, colors.muted, body);
    }
    label(8, 124, 304, "Will play: " + backingLabel(modeForLayers(pendingLayers_)), colors.signal, body);
    menuHints({rowHint, {"Enter", enterAction()}, cancelHint}, {{"Backspace / R", "Load preset layers"}});
}
void UI::drawTempo() {
    label(8, 5, 304, "Tempo", colors.accent, titleFont);
    label(8, 26, 304, instrument_.library.current(instrument_.settings).name + " / 40-240 BPM", colors.muted, body);
    box(28, 50, 264, 52, colors.panel);
    label(36, 67, 248, (tempoInput_.empty() ? "_" : tempoInput_) + " BPM",
          colors.accent, titleFont, LV_TEXT_ALIGN_CENTER);
    if (!tempoError_.empty()) label(8, 113, 304, tempoError_, colors.accent, body);
    menuHints({{"0-9", "Type BPM"}, {"Enter", enterAction()}, cancelHint},
              {{"Backspace", tempoReplace_ ? "Clear" : "Delete digit"}, {"R", "Load preset tempo"}});
}
void UI::drawCombinations() {
    const auto &s = instrument_.settings;
    label(8, 5, 210, "Chord config", colors.accent, titleFont);
    label(222, 8, 92, scopeName(), colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    std::vector<std::string> names{"All"};
    for (int n = 0; n < 7; n++)
        names.push_back(degreeName(n));
    tabs(names, scope_ + 1, true);
    const auto rows = combinationRows();
    int row = int(std::find(rows.begin(), rows.end(), combination_) - rows.begin());
    int start = firstVisibleRow(row, int(rows.size()));
    for (int index = start; index < int(rows.size()) && index < start + visibleRows; index++) {
        int n = rows[index], y = 46 + (index - start) * rowHeight;
        bool selected = combination_ == n;
        if (n < 4) {
            if (selected) rowFocus(y, rowHeight);
            for (int button = 0; button < 3; button++)
                box(16, y + 1 + button * 4, 7, 3,
                    combinationMasks[n] & (1u << button) ? colors.signal : colors.keyFace);
            auto quality = scope_ < 0 ? Quality(s.combinations[n]) : s.quality(combinationMasks[n], scope_);
            label(30, y, 145, combinationName(n), selected ? colors.accent : colors.text, body);
            label(180, y, 75, qualityName(quality), selected ? colors.text : colors.muted, body);
            label(254, y + 2, 58, scope_ < 0 ? "Shared" : s.degreeCombinations[scope_][n] < 0 ? "Inherited" : "Custom",
                  colors.muted, small, LV_TEXT_ALIGN_RIGHT);
        } else {
            std::string value;
            if (n == 6) {
                int octave = chordOctaveValue(s);
                value = octave == -2 ? "All (" + std::to_string(s.chordOctave) + ")" : std::to_string(octave);
            } else if (n == 7)
                value = noteName(s.tonic);
            else if (n == 8)
                value = s.degreeChordRoot[scope_] < 0 ? "Key (" + noteName(s.chordRoot(scope_)) + ")" :
                                                       noteName(s.degreeChordRoot[scope_]);
            else if (n == 5)
                value = s.combinationMs ? std::to_string(s.combinationMs) + " ms" : "Off";
            else value = "Restore >";
            fieldRow(y, rowHeight, combinationName(n), value, selected);
        }
    }
    scrollIndicator(45, start, int(rows.size()));
    if (!status_.empty()) label(8, menuDetailY, 306, status_, colors.signal);
    menuHints({{LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " / 1-8", "Tab"}, rowHint, {"Enter", enterAction()}, backHint},
              {canRestoreField() ? defaultHint : Hint{}});
}
void UI::drawFrontNotes() {
    const auto &s = instrument_.settings;
    label(8, 5, 220, "Front notes", colors.accent, titleFont);
    label(230, 8, 84, "All presets", colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    std::vector<std::string> names{"All"};
    for (int n = 0; n < 7; n++) names.push_back(std::string(1, frontKeys[n]));
    tabs(names, frontKey_ + 1);
    const auto rows = frontRows();
    Settings defaults;
    defaults.frontMode = s.frontMode;
    for (int row = 0; row < int(rows.size()); row++) {
        int n = rows[row];
        int y = 46 + row * rowHeight;
        bool modified =
            !((frontKey_ < 0 && n == 3) || (frontKey_ >= 0 && n == 2)) &&
            frontValue(s, n) != frontValue(frontKey_ < 0 && n == 0 ? Settings{} : defaults, n);
        fieldRow(y, rowHeight, std::string(frontSettingName(n)) + (modified ? " *" : ""),
                 frontValueName(s, n), n == frontField_);
    }
    std::string detail = "Each mode remembers its K/L action and time";
    if (frontKey_ >= 0) {
        Chord chord{s.chordRoot(6), -1, 6, Quality::Major};
        int pitch = frontNotes(chord, s)[frontKey_];
        detail = std::string("Example: ") + frontKeys[frontKey_] + " plays " + noteName(pitch) +
                 std::to_string(pitch / 12 - 1) + " with " +
                 (s.frontMode ? "the selected scale" : chordName(chord) + " major");
    }
    label(8, menuDetailY, 306, status_.empty() ? detail : status_, status_.empty() ? colors.muted : colors.signal);
    menuHints({tabHint, rowHint, {"Enter", enterAction()}, backHint},
              {canRestoreField() ? defaultHint : Hint{}, editedHint});
}
void UI::drawPicker() {
    const auto action = confirmation();
    const bool confirming = action != Confirmation::None;
    std::string title, subtitle;
    switch (page_) {
    case Page::Setup:
    case Page::VolumeEffects:
        title = settingName(selection_);
        if (confirming) {
            title = action == Confirmation::Create ? "Create preset copy?" : "Restore preset?";
            subtitle = instrument_.library.current(instrument_.settings).name;
        }
        break;
    case Page::Envelope:
        title = envelopePartName(envelopePart_);
        title += confirming ? " / Restore envelope?"
                            : std::string(" / ") + envelopeControlName(envelopeField_);
        break;
    case Page::FrontNotes:
        if (confirming) {
            title = frontKey_ < 0 ? "Restore all front notes?" : "Restore this key?";
            if (frontKey_ >= 0) subtitle = std::string("Key ") + frontKeys[frontKey_];
        } else {
            title = (frontKey_ < 0 ? "All" : std::string(1, frontKeys[frontKey_])) +
                    std::string(" / ") + frontSettingName(frontField_);
        }
        break;
    case Page::Combinations:
        if (confirming) {
            title = "Restore chord config?";
            subtitle = scopeName();
        } else if (combination_ < 4) {
            title = "Chord quality / " + scopeName();
            subtitle = combinationName(combination_);
        } else {
            title = std::string(combinationName(combination_)) + " / " + scopeName();
        }
        break;
    }
    label(8, 5, confirming ? 306 : 260, title, colors.accent, titleFont);
    if (!confirming)
        label(273, 8, 41, std::to_string(choice_ + 1) + "/" + std::to_string(choices_.size()),
              colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    const bool grouped = soundPicker();
    const bool canRestore = canRestoreField();
    const int defaultChoice = canRestore ? defaultValue() : 0;
    const int currentChoice = confirming ? 0 : fieldValue();
    bool multipleGroups = false;
    if (grouped) {
        auto categories = soundCategories();
        multipleGroups = categories.size() > 1;
        tabs(categories, int(std::find(categories.begin(), categories.end(), soundCategory_) -
                             categories.begin()));
    } else if (!subtitle.empty()) {
        label(8, 26, 306, subtitle, colors.muted);
    }
    int start = firstVisibleRow(choice_, int(choices_.size()));
    for (int n = start; n < int(choices_.size()) && n < start + visibleRows; n++) {
        int y = 46 + (n - start) * rowHeight;
        bool selected = n == choice_;
        if (selected) rowFocus(y, rowHeight);
        const auto &choice = choices_[n];
        bool isCurrent = !confirming && choice.value == currentChoice;
        bool isDefault = canRestore && choice.value == defaultChoice;
        int nameWidth = isCurrent && isDefault ? 203 : (isCurrent || isDefault ? 244 : 294);
        label(16, y, nameWidth, choice.name, selected ? colors.accent : colors.text, body);
        if (isCurrent || isDefault) {
            const char *badge = isCurrent && isDefault ? "Current / Default" : isCurrent ? "Current" : "Default";
            label(221, y + 2, 90, badge, isDefault ? colors.signal : colors.muted, small,
                  LV_TEXT_ALIGN_RIGHT);
        }
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
    if (!description.empty()) label(8, menuDetailY, 306, description, colors.muted);
    menuHints({{multipleGroups ? horizontalKeys : nullptr, "Group"},
               {choices_.size() > 1 ? verticalKeys : nullptr, grouped ? "Sound" : "Choose"},
               {"Enter", enterAction()}, cancelHint},
              {{canRestore ? "Backspace" : nullptr, "Select default"}});
}
void UI::drawPerformance() {
    const auto &i = instrument_;
    const auto &s = i.settings;
    const int transpose = i.semitoneShift();
    label(8, 5, 174, transpose ? "ZeroChord  +1" : "ZeroChord", colors.accent, titleFont);
    const auto key = noteName(s.tonic) + (s.frontMode ? (s.scale ? " min" : " maj") : "");
    const auto &preset = i.library.current(s).name;
    const bool presetEdited = i.library.modified(s);
    constexpr int statusLeft = 116, statusRight = 312, betweenFields = 4;
    const int keyWidth = statusFieldWidth("Key:", key);
    const int presetWidth = std::min(statusFieldWidth("Preset:", preset, presetEdited),
                                     statusRight - statusLeft - keyWidth - betweenFields);
    const int presetX = statusRight - presetWidth;
    statusField(presetX - betweenFields - keyWidth, keyWidth, "Key:", key);
    statusField(presetX, presetWidth, "Preset:", preset, presetEdited);

    box(8, 33, 74, 55, i.active ? colors.activeChord : colors.panel);
    const std::string currentChord = i.active ? chordName(i.chord) : "Ready";
    label(13, 42, 65, currentChord, i.active ? colors.accent : colors.text,
          textWidth(currentChord, titleFont) <= 65 ? titleFont : body);
    if (i.active) {
        const std::string degree = degreeName(i.chord.column);
        const int width = textWidth(degree, mono);
        label(13, 67, width, degree, colors.muted, mono);
        label(17 + width, 66, 52 - width, "chord", colors.muted);
    } else {
        label(13, 66, 65, "Select chord", colors.muted);
    }

    constexpr int gridX = 103, gridWidth = 28, gridStep = 30, gridY = 45, gridRowStep = 16;
    constexpr const char *chordKeys[] = {"2345678", "QWERTYU", "ASDFGHJ"};
    constexpr const char *qualities[] = {"M", "m", "7"};
    for (int row = 0; row < 3; row++)
        label(86, gridY + row * gridRowStep, 13, qualities[row], colors.muted, body, LV_TEXT_ALIGN_CENTER);
    for (int col = 0; col < 7; col++) {
        const int x = gridX + col * gridStep;
        const bool selected = i.active && col == i.chord.column;
        label(x, 23, gridWidth, degreeName(col), selected ? colors.accent : colors.muted, mono, LV_TEXT_ALIGN_CENTER);
        label(x, 31, gridWidth, noteName(s.chordRoot(col) + transpose), selected ? colors.accent : colors.text,
              body, LV_TEXT_ALIGN_CENTER);
        for (int row = 0; row < 3; row++)
            keycap(x, gridY + row * gridRowStep, gridWidth, std::string(1, chordKeys[row][col]), body,
                   selected ? colors.activeChord : colors.panel, selected ? colors.accent : colors.keyText, 15);
    }

    hintLine(93, {{"I", s.hold ? "Hold on" : "Hold off"}, {"O", backingLabel(s.mode)},
                  {"P", std::to_string(s.bpm) + " BPM"}});
    const auto pitches = i.playingFrontNotes();
    const bool frontReady = i.active || s.frontMode == 1;
    for (int n = 0; n < 7; n++) {
        const int x = 8 + n * 44;
        const bool flash = i.harpFlash == n;
        box(x, 108, 40, 30, flash ? colors.signal : colors.frontPanel);
        label(x + 2, 108, 36,
              frontReady ? noteName(pitches[n]) + std::to_string(pitches[n] / 12 - 1) : "-",
              flash ? colors.bg : colors.text, body, LV_TEXT_ALIGN_CENTER);
        keycap(x + 11, 124, 18, std::string(1, frontKeys[n]), mono,
               flash ? colors.bg : colors.keyFace, flash ? colors.signal : colors.keyText, 12);
    }
    hintLine(141, {{"K/L", s.frontAction[s.frontMode] ? "Octave -/+" : "Strum"},
                   {"9/0", "Preset"}, {"1", "Hold to sharpen"}});
    hintLine(155, {{"Tab", "Setup"}, {"Space", "Stop"}, {"Esc", "Hold to exit"}});
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
    signature << ',' << tempo_ << ',' << tempoReplace_ << ',' << tempoInput_ << ',' << tempoError_
              << ',' << int(page_) << ',' << combination_ << ',' << scope_ << ',' << picker_
              << ',' << choice_ << ',' << int(tab_);
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
    colors = s.theme ? light : dark;
    lv_obj_clean(lv_screen_active());
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(colors.bg), 0);
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
    if (settingsOpen && page_ == Page::VolumeEffects) {
        drawVolumeEffects();
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
    if (!settingsOpen) {
        drawPerformance();
        return;
    }
    label(8, 5, 174, "ZeroChord / Setup", colors.accent, titleFont);
    label(184, 7, 130,
          noteName(s.tonic) + (s.frontMode ? (s.scale ? " min" : " maj") : "") + "  " +
              i.library.current(s).name + (i.library.modified(s) ? "*" : ""),
          colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    tabs(setupTabNames, int(tab_));
    const int scopeX = tabX(int(tab_), int(setupTabNames.size())) + 2;
    label(scopeX, 46, 312 - scopeX, tab_ == Tab::App ? "Applies to all presets" : "Settings for this preset",
          colors.muted);
    drawSettingRows(59, visibleRows, 15);
    if (!status_.empty())
        label(8, menuDetailY, 306, status_, colors.signal);
    else if (tab_ == Tab::App && !soundLibrary().warning.empty())
        label(8, menuDetailY, 306, "Some sound packs skipped; see startup log", colors.muted);
    menuHints({tabHint, rowHint, {"Enter", enterAction()}, backHint},
              {canRestoreField() ? defaultHint : Hint{}, editedHint});
}
} // namespace zc
