#pragma once
// Shared test navigation: reach any settings row, including rows on the Backing and
// Front sound pages, using the same keys a player presses.
#include "settings_editor.hpp"
#include <stdexcept>

namespace zc::test {
inline void tap(SettingsEditor &e, char key) {
    e.key(key, true);
    e.key(key, false);
}
// Backing tab holding a field, or -1 when the field is not on the Backing page.
inline int backingTabFor(int field) {
    if ((field >= SettingsEditor::chordMixField && field < SettingsEditor::chordMixField + 3) ||
        field == SettingsEditor::drumMixField || field == SettingsEditor::bassMixField)
        return 0;
    if (field == 11 || field == 13 || field == 24) return 1;
    if (field == 9 || field == SettingsEditor::arpSoundField || field == SettingsEditor::arpEnvelopeField)
        return 2;
    if (field == 12 || field == 21 || field == 25) return 3;
    if (field == SettingsEditor::bassPatternField || field == SettingsEditor::bassSoundField ||
        field == SettingsEditor::bassEnvelopeField)
        return 4;
    if (field == SettingsEditor::drumKitField || field == SettingsEditor::drumPatternField) return 5;
    return -1;
}
inline void selectRow(SettingsEditor &e, int field) {
    for (int n = 0; e.selected() != field && n < SettingsEditor::settingCount; n++) tap(e, 18);
    if (e.selected() != field) throw std::runtime_error("Row missing");
}
inline void goToRow(SettingsEditor &e, int field) {
    const int tab = backingTabFor(field);
    const bool front = field == 14 || field == 23;
    const bool volume = field == SettingsEditor::systemVolumeField || field == 17 ||
                        (field >= 4 && field <= 6) || (field >= 26 && field <= 28);
    using Page = SettingsEditor::Page;
    const Page page = tab >= 0 ? Page::Backing
                      : front  ? Page::FrontSound
                      : volume ? Page::VolumeEffects : Page::Setup;
    if (e.page() != page && e.page() != Page::Setup) tap(e, 27);
    if (page != Page::Setup && e.page() != page) {
        goToRow(e, tab >= 0  ? SettingsEditor::backingField
                   : front ? SettingsEditor::frontSoundField : SettingsEditor::volumeEffectsField);
        tap(e, '\n');
    }
    if (tab >= 0) tap(e, char('1' + tab));
    if (page == Page::Setup) {
        const auto target = SettingsEditor::appField(field) ? SettingsEditor::Tab::App
                                                            : SettingsEditor::Tab::Style;
        if (e.tab() != target) tap(e, char('1' + int(target))); // Number keys jump to a tab.
    }
    selectRow(e, field);
}
} // namespace zc::test
