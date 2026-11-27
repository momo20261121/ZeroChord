#include "ui.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <array>
#include <cctype>
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

lv_obj_t *keycap(int x, int y, int width, const std::string &key, const lv_font_t *font,
                 uint32_t face, uint32_t ink, int height = 13);

// Tabs drawn on the current page; the footer then shows Tab and 1-N.
int pageTabs = 0;

// Numbered tabs: 1-8 jump to a tab and Tab moves to the next one (hinted in the footer).
void tabs(const std::vector<std::string> &names, int active, bool symbols = false) {
    const int count = int(names.size());
    const int width = tabsWidth;
    pageTabs = count;
    for (int n = 0; n < count; n++) {
        int x = tabsX + n * width / count;
        int w = tabsX + (n + 1) * width / count - x - 2;
        const bool selected = n == active;
        box(x, 26, w, 18, selected ? colors.accent : colors.panel);
        const int inset = n < 8 ? 10 : 2;
        if (n < 8) label(x + 2, 29, 7, std::to_string(n + 1), selected ? colors.bg : colors.signal);
        const bool symbol = symbols && n != 0; // Degree and key names; "All" stays text.
        const auto *font = symbol ? mono : small;
        // A long name in a narrow tab starts after its number rather than under it.
        const int room = w - inset - 1;
        const int start = textWidth(names[n], font) > room ? 9 : inset;
        lv_label_set_long_mode(label(x + start, symbol ? 31 : 29, w - start - 1, names[n],
                                     selected ? colors.bg : symbol ? colors.keyText : colors.text,
                                     font, LV_TEXT_ALIGN_CENTER),
                               LV_LABEL_LONG_CLIP);
    }
}

const Hint editedHint{"*", "Edited"};
constexpr int detailY = 137;
constexpr int hintY = 154;
constexpr int visibleRows = 5;
constexpr int rowHeight = 17;
const std::vector<std::string> setupTabNames{"Style", "App", "About"};
constexpr char frontKeys[] = "ZXCVBNM";

std::string keyLabel(const Settings &s) {
    auto name = [](int tonic, int scale) { return noteName(tonic) + " " + scaleInfo(scale).abbr; };
    auto key = name(s.tonic, s.chordScale);
    if (s.frontMode && !s.frontFollow && (s.frontTonic != s.tonic || s.scale != s.chordScale))
        key += " / " + name(s.frontTonic, s.scale); // Front scale notes in their own key.
    return key;
}

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

lv_obj_t *keycap(int x, int y, int width, const std::string &key, const lv_font_t *font,
                 uint32_t face, uint32_t ink, int height) {
    auto *o = label(x, y, width, key, ink, font, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_height(o, height);
    lv_obj_set_style_pad_top(o, (height - font->line_height) / 2, 0);
    lv_obj_set_style_radius(o, 3, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(face), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    return o;
}

// In-place updates touch an object only when a value changes, so LVGL redraws just that area.
void setText(lv_obj_t *o, const std::string &value) {
    if (value != lv_label_get_text(o)) lv_label_set_text(o, value.c_str());
}
void setInk(lv_obj_t *o, uint32_t color) {
    if (!lv_color_eq(lv_obj_get_style_text_color(o, LV_PART_MAIN), lv_color_hex(color)))
        lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
}
void setFace(lv_obj_t *o, uint32_t color) {
    if (!lv_color_eq(lv_obj_get_style_bg_color(o, LV_PART_MAIN), lv_color_hex(color)))
        lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
}
void setFont(lv_obj_t *o, const lv_font_t *font) {
    if (lv_obj_get_style_text_font(o, LV_PART_MAIN) != font) lv_obj_set_style_text_font(o, font, 0);
}
void setShown(lv_obj_t *o, bool shown) {
    if (lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN) == shown) {
        if (shown) lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    }
}

void hintLine(int y, const std::vector<Hint> &hints, const lv_font_t *font = body,
              bool caps = true, int gap = 8, int right = 314) {
    int x = 8;
    for (const auto &hint : hints) {
        if (!hint.key) continue;
        const auto *keyFont = caps && std::strlen(hint.key) == 1 ? mono : small;
        const int keyWidth = textWidth(hint.key, keyFont) + (caps ? 8 : 0);
        std::string action = hint.action;
        if (x + keyWidth + 4 + textWidth(action, font) > right && !hint.shortAction.empty())
            action = hint.shortAction;
        const int actionWidth = action.empty() ? 0 : textWidth(action, font);
        if (x + keyWidth + 4 + actionWidth > right) break; // Never clip a hint mid-word.
        if (caps) keycap(x, y, keyWidth, hint.key, keyFont, colors.keyFace, colors.keyText);
        else label(x, y, keyWidth, hint.key, colors.signal, keyFont);
        x += keyWidth + (action.empty() ? 3 : 4);
        if (action.empty()) continue;
        label(x, y + (caps ? (font == body ? -1 : 1) : 0), actionWidth, action, colors.muted, font);
        x += actionWidth + gap;
    }
}

// The bottom of every menu: what the highlighted row does, then any non-obvious keys.
void footer(const std::string &detail, const std::string &status,
            std::initializer_list<Hint> hints = {}) {
    if (!status.empty()) label(8, detailY, 306, status, colors.signal);
    else if (!detail.empty()) label(8, detailY, 306, detail, colors.muted);
    static const char *jumps[]{"1", "1-2", "1-3", "1-4", "1-5", "1-6", "1-7", "1-8"};
    std::vector<Hint> line;
    if (pageTabs > 1) {
        line.push_back({"Tab", ""});
        line.push_back({jumps[std::min(pageTabs, 8) - 1], "Switch tab"});
    }
    line.insert(line.end(), hints);
    pageTabs = 0;
    hintLine(hintY, line, small);
}

void rowFocus(int y, int height) {
    box(6, y - 1, 308, height, colors.panel);
    box(8, y + 2, 2, height - 6, colors.accent);
}

using RowKind = SettingsEditor::RowKind;
// Number rows show the arrows that change them; rows that Enter opens end in ">".
void fieldRow(int y, int height, const std::string &name, const std::string &value, bool selected,
              RowKind kind = RowKind::Plain) {
    if (selected) rowFocus(y, height);
    label(16, y, 159, name, selected ? colors.accent : colors.text, body);
    const bool arrows = selected && kind == RowKind::Number;
    const std::string shown = arrows ? LV_SYMBOL_LEFT "  " + value + "  " LV_SYMBOL_RIGHT : value;
    auto *valueLabel = label(180, y, kind == RowKind::Opens ? 122 : 132, shown,
                             selected ? colors.text : colors.muted, body);
    lv_obj_set_height(valueLabel, body->line_height); // One line: long values end in dots.
    lv_label_set_long_mode(valueLabel, LV_LABEL_LONG_DOT);
    if (kind == RowKind::Opens) label(304, y, 10, ">", selected ? colors.accent : colors.muted, body);
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
Hint UI::restoreHint() const {
    if (!canRestoreField()) return {};
    if (picker_) return {"Backspace", "Highlight the default"};
    return appPage() ? Hint{"Backspace", "Restore this option to its default", "Restore default"}
                     : Hint{"Backspace", "Restore this option to its preset default",
                            "Restore preset default"};
}
void UI::menuFooter(bool edited) {
    footer(rowDescription(), status_, {restoreHint(), edited ? editedHint : Hint{}});
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
}
bool UI::drawSettingRows(int top, int limit, int spacing) {
    const auto &fields = rows();
    const int first = firstVisibleRow(rowIndex(), int(fields.size()), limit);
    bool edited = false;
    for (int field : fields) edited = edited || fieldModified(field);
    for (int n = first; n < int(fields.size()) && n < first + limit; n++) {
        int field = fields[n];
        int y = top + (n - first) * spacing;
        const bool selected = field == selection_;
        fieldRow(y, spacing, rowName(field) + (fieldModified(field) ? " *" : ""), settingValue(field),
                 selected, rowKind(field));
    }
    scrollIndicator(top - 1, first, int(fields.size()), limit, spacing);
    return edited;
}
void UI::drawVolumeEffects() {
    label(8, 5, 222, settingName(volumeEffectsField), colors.accent, titleFont);
    label(230, 8, 84, "All presets", colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    menuFooter(drawSettingRows(21, 8, 14));
}
void UI::drawMidiInput() {
    label(8, 5, 222, settingName(midiInputField), colors.accent, titleFont);
    label(230, 8, 84, "All presets", colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    menuFooter(drawSettingRows(26, 3, 18));
}
void UI::drawAllFrontPage() {
    label(8, 5, 222, settingName(allFrontField), colors.accent, titleFont);
    label(230, 8, 84, "All presets", colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    const bool edited = drawSettingRows(26, 4, 18);
    // How to get there: the page sets the mode up, but it is opened from playing.
    label(8, 106, 304, "Hold Space while playing to open or close All front", colors.accent);
    menuFooter(edited);
}
void UI::drawEnvelope() {
    const auto &s = instrument_.settings;
    const auto &d = instrument_.library.current(s).sound;
    label(8, 5, 304, std::string(envelopePartName(envelopePart_)) + " envelope", colors.accent, titleFont);
    label(8, 26, 304, instrument_.library.current(s).name + " / " +
                          settingValue(envelopePart_ == FrontEnvelope ? 14
                                       : envelopePart_ == PadEnvelope ? 21
                                       : envelopePart_ == ArpEnvelope ? arpSoundField
                                       : envelopePart_ == BassEnvelope ? bassSoundField : 13),
          colors.muted);
    bool anyEdited = false;
    for (int field = 0; field < 5; field++) {
        int y = 46 + field * rowHeight;
        bool edited = field < 4 && s.envelopes[envelopePart_].values[field] != d.envelopes[envelopePart_].values[field];
        anyEdited = anyEdited || edited;
        fieldRow(y, rowHeight, std::string(envelopeControlName(field)) + (edited ? " *" : ""),
                 field == 4 ? "" : envelopeValueName(s, envelopePart_, field),
                 field == envelopeField_, rowKind(field));
    }
    menuFooter(anyEdited);
}
void UI::drawBacking() {
    const auto &s = instrument_.settings;
    label(8, 5, 200, "Backing layers", colors.accent, titleFont);
    label(200, 8, 114, instrument_.library.current(s).name, colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    tabs({"Layers", "Chord", "Arp", "Pad", "Bass", "Drums"}, backingTab_);
    if (backingTab_ != 0) {
        menuFooter(drawSettingRows(46, visibleRows, rowHeight));
        return;
    }
    bool edited = false;
    for (int n = 0; n < 5; ++n) {
        const auto layer = backingLayerOrder[n];
        const int field = layer == PlaybackMode::Drums ? drumMixField
                          : layer == PlaybackMode::Bass ? bassMixField
                                                        : chordMixField + n;
        const int y = 46 + n * 17;
        const bool selected = field == selection_;
        const bool on = s.enabled(layer);
        edited = edited || fieldModified(field);
        if (selected) rowFocus(y, rowHeight);
        keycap(16, y + 1, 14, on ? LV_SYMBOL_OK : "", small, colors.keyFace, colors.keyText);
        label(36, y, 120, std::string(modeName(layer)) + (fieldModified(field) ? " *" : ""),
              selected ? colors.accent : on ? colors.text : colors.muted, body);
        const std::string level = std::to_string(s.mix(layer)) + "%";
        label(170, y, 80, selected ? LV_SYMBOL_LEFT "  " + level + "  " LV_SYMBOL_RIGHT : level,
              selected ? colors.text : colors.muted, body, LV_TEXT_ALIGN_CENTER);
        if (!on) label(254, y + 2, 56, "Muted", colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    }
    menuFooter(edited);
}
void UI::drawFrontSound() {
    label(8, 5, 222, "Front sound", colors.accent, titleFont);
    label(200, 8, 114, instrument_.library.current(instrument_.settings).name, colors.muted, small,
          LV_TEXT_ALIGN_RIGHT);
    label(8, 26, 304, "Z-M notes and K/L strums", colors.muted);
    menuFooter(drawSettingRows(46, visibleRows, rowHeight));
}
void UI::drawTempo() {
    const auto &s = instrument_.settings;
    const auto &preset = instrument_.library.current(s).sound;
    label(8, 5, 304, "Tempo", colors.accent, titleFont);
    label(8, 26, 304, instrument_.library.current(s).name + " / 40-240 BPM", colors.muted, body);
    // Tempo is typed and applied with Enter; Swing and Swing amount apply as they change.
    const bool tempoRow = tempoRow_ == 0;
    box(28, 44, 264, 32, tempoRow ? colors.panel : colors.bg);
    if (tempoRow) box(30, 48, 2, 24, colors.accent);
    label(36, 49, 248, (tempoInput_.empty() ? "_" : tempoInput_) + " BPM",
          tempoRow ? colors.accent : colors.text, titleFont, LV_TEXT_ALIGN_CENTER);
    auto row = [&](int index, int y, const std::string &name, bool edited, const std::string &value,
                   bool active) {
        const bool selected = tempoRow_ == index;
        if (selected) rowFocus(y, rowHeight);
        label(16, y, 150, name + (edited ? " *" : ""), selected ? colors.accent : colors.text, body);
        label(170, y, 140, selected ? LV_SYMBOL_LEFT "  " + value + "  " LV_SYMBOL_RIGHT : value,
              active ? colors.text : colors.muted, body, LV_TEXT_ALIGN_CENTER);
    };
    row(1, 84, "Swing", s.swingMode != preset.swingMode, swingModeName(s.swingMode), true);
    row(2, 104, "Swing amount", s.swingAmount != preset.swingAmount,
        std::to_string(s.swingAmount) + "%", s.swingMode != SwingOff);
    static const char *details[]{"Type a tempo, then press Enter",
                                 "Enter or left/right: Off, 8ths, 16ths",
                                 "0% straight, 100% triplet feel"};
    footer(details[tempoRow_], tempoError_,
           {{"Backspace", tempoReplace_ ? "Clear" : "Delete digit", "Clear"},
            {"R", "Load preset tempo", "Preset"}});
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
            label(250, y + 2, 52, scope_ < 0 ? "Shared" : s.degreeCombinations[scope_][n] < 0 ? "Inherited" : "Custom",
                  colors.muted, small, LV_TEXT_ALIGN_RIGHT);
            label(304, y, 10, ">", selected ? colors.accent : colors.muted, body);
        } else {
            std::string value;
            if (n == 6) {
                int octave = chordOctaveValue(s);
                value = octave == -2 ? "All (" + std::to_string(s.chordOctave) + ")" : std::to_string(octave);
            } else if (n == 7)
                value = noteName(s.tonic);
            else if (n == chordScaleField)
                value = s.chordScale ? "Natural minor" : "Major";
            else if (n == 8)
                value = s.degreeChordRoot[scope_] < 0 ? "Key (" + noteName(s.chordRoot(scope_)) + ")" :
                                                       noteName(s.degreeChordRoot[scope_]);
            else if (n == 5)
                value = s.combinationMs ? std::to_string(s.combinationMs) + " ms" : "Off";
            else if (n == chordHoldField)
                value = s.hold ? "On" : "Off";
            fieldRow(y, rowHeight, combinationName(n), value, selected, rowKind(n));
        }
    }
    scrollIndicator(45, start, int(rows.size()));
    menuFooter(false);
}
void UI::drawFrontNotes() {
    const auto &s = instrument_.settings;
    label(8, 5, 220, "Front notes", colors.accent, titleFont);
    label(230, 8, 84, "All presets", colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    std::vector<std::string> names{"All"};
    for (int n = 0; n < 7; n++) names.push_back(std::string(1, frontKeys[n]));
    tabs(names, frontKey_ + 1, true);
    const auto rows = frontRows();
    Settings defaults;
    defaults.frontMode = s.frontMode;
    bool anyEdited = false;
    const int start = firstVisibleRow(int(std::find(rows.begin(), rows.end(), frontField_) - rows.begin()),
                                      int(rows.size()));
    for (int row = start; row < int(rows.size()) && row < start + visibleRows; row++) {
        int n = rows[row];
        int y = 46 + (row - start) * rowHeight;
        bool modified =
            !((frontKey_ < 0 && n == 3) || (frontKey_ >= 0 && n == 2)) &&
            frontValue(s, n) != frontValue(frontKey_ < 0 && n == 0 ? Settings{} : defaults, n);
        anyEdited = anyEdited || modified;
        fieldRow(y, rowHeight, std::string(frontSettingName(n)) + (modified ? " *" : ""),
                 frontValueName(s, n), n == frontField_, rowKind(n));
    }
    scrollIndicator(45, start, int(rows.size()));
    std::string detail = rowDescription();
    if (frontKey_ >= 0 && !frontReset()) {
        Chord chord{s.chordRoot(6), -1, 6, Quality::Major};
        int pitch = frontNotes(chord, s)[frontKey_];
        detail = std::string("Example: ") + frontKeys[frontKey_] + " plays " + noteName(pitch) +
                 std::to_string(pitch / 12 - 1) + " with " +
                 (s.frontMode ? "the selected scale" : chordName(chord) + " major");
    }
    footer(detail, status_, {restoreHint(), anyEdited ? editedHint : Hint{}});
}
void UI::drawPicker() {
    const auto action = confirmation();
    const bool confirming = action != Confirmation::None;
    std::string title, subtitle;
    switch (page_) {
    case Page::Setup:
    case Page::Backing:
    case Page::FrontSound:
    case Page::VolumeEffects:
    case Page::MidiInput:
    case Page::AllFront:
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
    if (confirming) {
        // One question: Enter does it, Esc keeps everything as it is.
        label(8, 5, 306, title, colors.accent, titleFont);
        if (!subtitle.empty()) label(8, 26, 306, subtitle, colors.muted);
        box(20, 52, 280, 50, colors.panel);
        label(30, 60, 260, choices_[1].name, colors.text, body, LV_TEXT_ALIGN_CENTER);
        label(30, 80, 260, choices_[1].description, colors.muted, small, LV_TEXT_ALIGN_CENTER);
        const char *verb = action == Confirmation::Create ? "Create" : "Restore";
        footer("", "", {{"Enter", verb}, {"Esc", "Keep"}});
        return;
    }
    label(8, 5, 260, title, colors.accent, titleFont);
    if (!confirming)
        label(273, 8, 41, std::to_string(choice_ + 1) + "/" + std::to_string(choices_.size()),
              colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    const bool grouped = soundPicker();
    const bool canRestore = canRestoreField();
    const int defaultChoice = canRestore ? defaultValue() : 0;
    const int currentChoice = confirming ? 0 : fieldValue();
    if (grouped) {
        auto categories = soundCategories();
        const int active = int(std::find(categories.begin(), categories.end(), soundCategory_) -
                               categories.begin());
        for (auto &name : categories)
            if (name == "Plucked") name = "Pluck"; // Fits a narrow tab.
        tabs(categories, active);
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
        label(16, y, nameWidth, choice.name,
              selected ? colors.accent : isDefault ? colors.signal : colors.text, body);
        if (isCurrent || isDefault) {
            const char *badge = isCurrent && isDefault ? "Current / Default" : isCurrent ? "Current" : "Default";
            label(221, y + 2, 90, badge, isDefault ? colors.signal : colors.muted, small,
                  LV_TEXT_ALIGN_RIGHT);
        }
    }
    scrollIndicator(45, start, int(choices_.size()));
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
    footer(description, "", {restoreHint()});
}
struct UI::PerformanceView {
    lv_obj_t *title, *chordBox, *chordName, *degree, *chordWord, *selectHint;
    int degreeWidth = -1;
    std::array<lv_obj_t *, 7> degrees, roots, frontBoxes, frontNotes, frontKeys;
    std::array<std::array<lv_obj_t *, 3>, 7> chordKeys;
    lv_obj_t *midi = nullptr; // "MIDI Front" while a keyboard is connected; lights on notes.
    // All front: one box, note and keycap per key, in allFrontKeys order.
    bool allFront = false;
    std::array<lv_obj_t *, allFrontKeyCount> keyBoxes{}, keyNotes{}, keyCaps{};
};

void UI::drawAllFront() {
    const auto &i = instrument_;
    const auto &s = i.settings;
    auto view = std::make_shared<PerformanceView>();
    auto &v = *view;
    v.allFront = true;
    v.title = label(8, 5, 108, "", colors.accent, titleFont);
    const bool free = s.allFrontMode == 1;
    std::string range;
    if (free) {
        const auto notes = i.playingAllFrontNotes();
        range = noteName(notes[21]) + std::to_string(notes[21] / 12 - 1) + "-" +
                noteName(notes[13]) + std::to_string(notes[13] / 12 - 1);
    }
    const std::string value = free ? range : keyLabel(s);
    const char *name = free ? "Range:" : "Key:";
    const auto &preset = i.library.current(s).name;
    constexpr int statusLeft = 116, statusRight = 312, betweenFields = 4;
    const int keyWidth = statusFieldWidth(name, value);
    const int presetWidth = std::min(statusFieldWidth("Preset:", preset, i.library.modified(s)),
                                     statusRight - statusLeft - keyWidth - betweenFields);
    const int presetX = statusRight - presetWidth;
    statusField(presetX - betweenFields - keyWidth, keyWidth, name, value);
    statusField(presetX, presetWidth, "Preset:", preset, i.library.modified(s));
    // The keyboard's aligned 7x4 block. In Free play the sharp rows are darker, with gaps blank.
    const auto notes = i.playingAllFrontNotes();
    for (int n = 0; n < allFrontKeyCount; n++) {
        const int row = n / 7, col = n % 7;
        const int x = 8 + col * 44, y = 22 + row * 32;
        const bool sharp = free && (row == 0 || row == 2);
        const char cap = char(std::toupper(static_cast<unsigned char>(allFrontKeys[n])));
        if (notes[size_t(n)] < 0) {
            v.keyCaps[size_t(n)] = keycap(x + 11, y + 16, 18, std::string(1, cap), mono,
                                          colors.bg, colors.panel, 12);
            continue;
        }
        v.keyBoxes[size_t(n)] = box(x, y, 40, 30, sharp ? colors.panel : colors.frontPanel);
        v.keyNotes[size_t(n)] = label(x + 2, y, 36, "", colors.text, body, LV_TEXT_ALIGN_CENTER);
        v.keyCaps[size_t(n)] = keycap(x + 11, y + 16, 18, std::string(1, cap), mono,
                                      colors.keyFace, colors.keyText, 12);
    }
    int hintsRight = 314;
    if (!i.midiDevice.empty()) {
        const std::string badge = std::string("MIDI ") + midiTargetName(s.midiTarget);
        const int width = textWidth(badge, small) + 10;
        v.midi = keycap(312 - width, 153, width, badge, small, colors.panel, colors.muted);
        hintsRight = 312 - width - 6;
    }
    std::vector<Hint> hints{{"Q/W", "Octave"}};
    if (!free) hints.push_back({"1", "Sharpen"});
    hints.push_back({"Tab", "Setup"});
    hints.push_back({"Space", "Hold: chords", "Hold"});
    hintLine(153, hints, body, true, 6, hintsRight);
    performance_ = view;
    updatePerformance();
}

void UI::drawPerformance() {
    const auto &i = instrument_;
    const auto &s = i.settings;
    if (i.allFront) {
        drawAllFront();
        return;
    }
    auto view = std::make_shared<PerformanceView>();
    auto &v = *view;
    // Objects that follow playing start blank; updatePerformance() fills them in.
    v.title = label(8, 5, 174, "", colors.accent, titleFont);
    const auto key = keyLabel(s);
    const auto &preset = i.library.current(s).name;
    const bool presetEdited = i.library.modified(s);
    constexpr int statusLeft = 116, statusRight = 312, betweenFields = 4;
    const int keyWidth = statusFieldWidth("Key:", key);
    const int presetWidth = std::min(statusFieldWidth("Preset:", preset, presetEdited),
                                     statusRight - statusLeft - keyWidth - betweenFields);
    const int presetX = statusRight - presetWidth;
    statusField(presetX - betweenFields - keyWidth, keyWidth, "Key:", key);
    statusField(presetX, presetWidth, "Preset:", preset, presetEdited);

    v.chordBox = box(8, 33, 74, 55, colors.panel);
    v.chordName = label(13, 42, 65, "", colors.text, titleFont);
    v.degree = label(13, 67, 0, "", colors.muted, mono);
    v.chordWord = label(17, 66, 52, "chord", colors.muted);
    v.selectHint = label(13, 66, 65, "Select chord", colors.muted);

    constexpr int gridX = 103, gridWidth = 28, gridStep = 30, gridY = 45, gridRowStep = 16;
    constexpr const char *chordKeys[] = {"2345678", "QWERTYU", "ASDFGHJ"};
    constexpr const char *qualities[] = {"M", "m", "7"};
    for (int row = 0; row < 3; row++)
        label(86, gridY + row * gridRowStep, 13, qualities[row], colors.muted, body, LV_TEXT_ALIGN_CENTER);
    for (int col = 0; col < 7; col++) {
        const int x = gridX + col * gridStep;
        v.degrees[col] = label(x, 23, gridWidth, degreeName(col), colors.muted, mono, LV_TEXT_ALIGN_CENTER);
        v.roots[col] = label(x, 31, gridWidth, "", colors.text, body, LV_TEXT_ALIGN_CENTER);
        for (int row = 0; row < 3; row++)
            v.chordKeys[col][row] = keycap(x, gridY + row * gridRowStep, gridWidth,
                                           std::string(1, chordKeys[row][col]), body, colors.panel,
                                           colors.keyText, 15);
    }

    int hintsRight = 314;
    if (!i.midiDevice.empty()) {
        const std::string badge = std::string("MIDI ") + midiTargetName(s.midiTarget);
        const int width = textWidth(badge, small) + 10;
        v.midi = keycap(312 - width, 93, width, badge, small, colors.panel, colors.muted);
        hintsRight = 312 - width - 6;
    }
    hintLine(93, {{"I", s.hold ? "Hold on" : "Hold off"}, {"O", backingLabel(s)},
                  {"P", std::to_string(s.bpm) + " BPM" + (s.swingMode ? " swing" : "")}},
             body, true, 8, hintsRight);
    for (int n = 0; n < 7; n++) {
        const int x = 8 + n * 44;
        v.frontBoxes[n] = box(x, 108, 40, 30, colors.frontPanel);
        v.frontNotes[n] = label(x + 2, 108, 36, "", colors.text, body, LV_TEXT_ALIGN_CENTER);
        v.frontKeys[n] = keycap(x + 11, 124, 18, std::string(1, frontKeys[n]), mono,
                                colors.keyFace, colors.keyText, 12);
    }
    hintLine(141, {{"K/L", s.frontAction[s.frontMode] ? "Octave -/+" : "Strum"},
                   {"9/0", "Preset"}, {"1", "Hold to sharpen"}});
    hintLine(155, {{"Tab", "Setup"}, {"Space", "Stop"}, {"Esc", "Hold to exit"}});
    performance_ = view;
    updatePerformance();
}

void UI::updatePerformance() {
    auto &v = *performance_;
    const auto &i = instrument_;
    const auto &s = i.settings;
    const int transpose = i.semitoneShift();
    if (v.midi) {
        setFace(v.midi, i.midiLit() ? colors.signal : colors.panel);
        setInk(v.midi, i.midiLit() ? colors.bg : colors.muted);
    }
    if (v.allFront) {
        setText(v.title, s.allFrontMode == 0 && transpose ? "All front  +1" : "All front");
        const auto notes = i.playingAllFrontNotes();
        for (int n = 0; n < allFrontKeyCount; n++) {
            if (!v.keyBoxes[size_t(n)]) continue;
            const bool held = i.allFrontHeld(n);
            const bool sharp = s.allFrontMode == 1 && (n / 7 == 0 || n / 7 == 2);
            const int pitch = notes[size_t(n)];
            setFace(v.keyBoxes[size_t(n)], held ? colors.signal : sharp ? colors.panel : colors.frontPanel);
            setText(v.keyNotes[size_t(n)], pitch < 0 ? "-" : noteName(pitch) + std::to_string(pitch / 12 - 1));
            setInk(v.keyNotes[size_t(n)], held ? colors.bg : colors.text);
            setFace(v.keyCaps[size_t(n)], held ? colors.bg : colors.keyFace);
            setInk(v.keyCaps[size_t(n)], held ? colors.signal : colors.keyText);
        }
        return;
    }
    setText(v.title, transpose ? "ZeroChord  +1" : "ZeroChord");

    setFace(v.chordBox, i.active ? colors.activeChord : colors.panel);
    const std::string currentChord = i.active ? chordName(i.chord) : "Ready";
    setText(v.chordName, currentChord);
    setInk(v.chordName, i.active ? colors.accent : colors.text);
    setFont(v.chordName, textWidth(currentChord, titleFont) <= 65 ? titleFont : body);
    if (i.active) {
        const std::string degree = degreeName(i.chord.column);
        const int width = textWidth(degree, mono);
        setText(v.degree, degree);
        if (width != v.degreeWidth) {
            v.degreeWidth = width;
            lv_obj_set_width(v.degree, width);
            lv_obj_set_x(v.chordWord, 17 + width);
            lv_obj_set_width(v.chordWord, 52 - width);
        }
    }
    setShown(v.degree, i.active);
    setShown(v.chordWord, i.active);
    setShown(v.selectHint, !i.active);

    for (int col = 0; col < 7; col++) {
        const bool selected = i.active && col == i.chord.column;
        setInk(v.degrees[col], selected ? colors.accent : colors.muted);
        setText(v.roots[col], noteName(s.chordRoot(col) + transpose));
        setInk(v.roots[col], selected ? colors.accent : colors.text);
        for (auto *keycap : v.chordKeys[col]) {
            setFace(keycap, selected ? colors.activeChord : colors.panel);
            setInk(keycap, selected ? colors.accent : colors.keyText);
        }
    }

    const auto pitches = i.playingFrontNotes();
    const bool frontReady = i.active || s.frontMode == 1;
    for (int n = 0; n < 7; n++) {
        const bool flash = i.harpFlash == n || i.frontHeld(n);
        setFace(v.frontBoxes[n], flash ? colors.signal : colors.frontPanel);
        setText(v.frontNotes[n],
                frontReady ? noteName(pitches[n]) + std::to_string(pitches[n] / 12 - 1) : "-");
        setInk(v.frontNotes[n], flash ? colors.bg : colors.text);
        setFace(v.frontKeys[n], flash ? colors.bg : colors.keyFace);
        setInk(v.frontKeys[n], flash ? colors.signal : colors.keyText);
    }
}

void UI::draw() {
    const auto &i = instrument_;
    const auto &s = i.settings;
    std::ostringstream signature;
    signature << settingsOpen << ',' << selection_ << ',' << s.tonic << ',' << s.scale << ','
              << s.preset << ',' << s.chordVolume << ',' << s.harpVolume << ',' << s.echo << ','
              << s.strumMs << ',' << s.bpm << ',' << s.pattern << ',' << s.hold << ','
              << int(s.mode) << ',' << s.combinationMs;
    // What playing changes; the playing screen follows it in place.
    std::ostringstream playing;
    playing << i.active << ',' << chordName(i.chord) << ',' << i.chord.column << ','
            << i.harpFlash << ',' << i.semitoneShift() << ',' << i.frontOctaveShift[0] << ','
            << i.frontOctaveShift[1];
    for (int n = 0; n < 7; n++) playing << ',' << i.frontHeld(n);
    playing << ',' << i.midiLit();
    for (int n = 0; n < allFrontKeyCount; n++) playing << ',' << i.allFrontHeld(n);
    // All front redraws when it opens or its range moves (status line and silent keys).
    signature << ',' << i.allFront << ',' << i.allFrontShift << ',' << s.allFrontMode << ','
              << s.allFrontOctave << ',' << s.allFrontLowest;
    // The keyboard and its target change the playing screen; the last note only this page.
    signature << ',' << i.midiDevice << ',' << s.midiTarget << ',' << s.midiVelocity << ','
              << midiRow_;
    if (settingsOpen && page_ == Page::MidiInput)
        signature << ',' << i.midiLastNote << ',' << i.midiLastVelocity;
    signature << ',' << backingTab_ << ',' << fromPlay_ << ',' << s.arpProgram << ',' << s.arpSound
              << ',' << s.chordScale << ',' << s.frontFollow << ',' << s.frontTonic << ','
              << int(envelopeParent_);
    for (int level : s.layerMix) signature << ',' << level;
    signature << ',' << s.drums << ',' << s.drumKit << ',' << s.drumPattern << ',' << s.drumSound
              << ',' << s.scale << ',' << s.bass << ',' << s.bassProgram << ',' << s.bassPattern
              << ',' << s.bassSound << ',' << s.swingMode << ',' << s.swingAmount << ','
              << s.chordVoicing << ',' << s.padVoicing << ',' << s.output << ',' << instrument_.jack
              << ',' << tempoRow_;
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
              << s.frontAction[0] << ',' << s.frontAction[1] << ',' << s.scaleStrumMs;
    for (int n = 0; n < 7; n++)
        signature << ',' << s.frontSource[n] << ',' << s.frontOffset[n] << ',' << s.frontOctave[n]
                  << ',' << s.frontDegree[n] << ',' << s.scaleOctave[n];
    signature << ',' << s.chordOctave << ',' << systemVolume_.value_or(-1);
    for (int gain : s.eqDb) signature << ',' << gain;
    for (int octave : s.degreeChordOctave) signature << ',' << octave;
    for (int root : s.degreeChordRoot) signature << ',' << root;
    signature << ',' << int(envelopePart_) << ',' << envelopeField_;
    for (const auto &envelope : s.envelopes)
        for (int value : envelope.values) signature << ',' << value;
    auto layout = signature.str(), played = playing.str();
    if (layout == previous_ && played == previousPlaying_)
        return;
    previousPlaying_ = played;
    if (layout == previous_ && performance_ && !tempo_ && !settingsOpen) {
        updatePerformance();
        return;
    }
    previous_ = std::move(layout);
    performance_.reset(); // Its widgets go with the screen.
    colors = s.theme ? light : dark;
    lv_obj_clean(lv_screen_active());
    lv_obj_remove_flag(lv_screen_active(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(colors.bg), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    if (tempo_) {
        drawTempo();
        return;
    }
    if (settingsOpen && picker_) {
        drawPicker();
        return;
    }
    if (settingsOpen && page_ == Page::Backing) {
        drawBacking();
        return;
    }
    if (settingsOpen && page_ == Page::FrontSound) {
        drawFrontSound();
        return;
    }
    if (settingsOpen && page_ == Page::VolumeEffects) {
        drawVolumeEffects();
        return;
    }
    if (settingsOpen && page_ == Page::MidiInput) {
        drawMidiInput();
        return;
    }
    if (settingsOpen && page_ == Page::AllFront) {
        drawAllFrontPage();
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
          keyLabel(s) + "  " + i.library.current(s).name + (i.library.modified(s) ? "*" : ""),
          colors.muted, small, LV_TEXT_ALIGN_RIGHT);
    tabs(setupTabNames, int(tab_));
    const int scopeX = tabX(int(tab_), int(setupTabNames.size())) + 2;
    label(scopeX, 46, 312 - scopeX, tab_ == Tab::App ? "Applies to all presets" : "Settings for this preset",
          colors.muted);
    const bool edited = drawSettingRows(59, visibleRows, 15);
    const bool skipped = tab_ == Tab::App && !soundLibrary().warning.empty() && status_.empty();
    footer(skipped ? "Some sound packs skipped; see startup log" : rowDescription(), status_,
           {restoreHint(), edited ? editedHint : Hint{}});
}
} // namespace zc
