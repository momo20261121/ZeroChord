#include "settings_editor.hpp"
#include "patterns.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <iterator>
#include <tuple>
namespace zc {
namespace {
const std::vector<int> volumeEffectsRows{SettingsEditor::systemVolumeField, 17, 4, 5, 6, 26, 27, 28,
                                        SettingsEditor::outputField};
const char *scaleName(int scale) { return scaleInfo(scale).name; }
// Built-in General MIDI kits (drum bank 128).
const std::vector<std::pair<int, const char *>> drumKits{
    {0, "Standard"}, {8, "Room"},  {16, "Power"}, {24, "Electronic"},
    {25, "TR-808"},  {32, "Jazz"}, {40, "Brush"}, {48, "Orchestra"}};
std::string drumKitName(const Settings &s) {
    if (auto *sound = soundLibrary().find(s.drumSound); sound && !sound->bank.empty())
        return sound->name; // A missing pack plays the same built-in kit.
    for (const auto &[kit, name] : drumKits)
        if (kit == s.drumKit) return name;
    return "Kit " + std::to_string(s.drumKit + 1);
}
std::string milliseconds(int value) { return std::to_string(value) + " ms"; }
constexpr uint64_t repeatDelayMs = 400, repeatMs = 80, fastRepeatAfterMs = 1500, fastRepeatMs = 40;
} // namespace
const char *SettingsEditor::envelopePartName(int part) {
    static const char *names[]{"Front", "Chord", "Pad", "Arp", "Bass"};
    return names[part];
}
const char *SettingsEditor::envelopeControlName(int field) {
    static const char *names[]{"Attack", "Decay", "Sustain", "Release", "Restore envelope..."};
    return names[field];
}
int SettingsEditor::envelopeField(int part) {
    static constexpr int fields[]{23, 24, 25, arpEnvelopeField, bassEnvelopeField};
    return fields[part];
}
int SettingsEditor::envelopePartFor(int field) {
    return field == arpEnvelopeField    ? ArpEnvelope
           : field == bassEnvelopeField ? BassEnvelope
                                        : field - 23;
}
std::pair<int, std::string> SettingsEditor::soundSource(const Settings &s, int field) const {
    const auto &preset = presets[std::clamp(s.preset, 0, 3)];
    if (field == 14)
        return {s.harpProgram < 0 ? preset.harpProgram : s.harpProgram, s.frontSound};
    if (field == 21)
        return {s.padProgram < 0 ? preset.chordProgram : s.padProgram, s.padSound};
    if (field == arpSoundField && s.arpProgram >= 0) return {s.arpProgram, s.arpSound};
    if (field == bassSoundField) return {s.bassProgram, s.bassSound};
    return {s.chordProgram < 0 ? preset.chordProgram : s.chordProgram, s.chordSound};
}
std::optional<std::array<int, 4>> SettingsEditor::soundEnvelope(const Settings &s, int part) const {
    static constexpr int fields[]{14, 13, 21, arpSoundField, bassSoundField};
    auto [program, id] = soundSource(s, fields[part]);
    auto values = soundLibrary().envelope(program, id);
    if (values && (part == ChordEnvelope || part == ArpEnvelope)) {
        // The renderer bounds backing attacks and tails unless the envelope is set.
        const bool sustained = part == ChordEnvelope && s.chordPattern == sustainedChordPattern;
        (*values)[0] = std::min((*values)[0], 10);
        (*values)[3] = std::min((*values)[3], sustained ? 250 : 200);
    }
    return values;
}
std::string SettingsEditor::envelopeValueName(const Settings &s, int part, int field) const {
    int value = s.envelopes[part].values[field];
    if (value < 0) {
        auto own = soundEnvelope(s, part);
        if (!own) return "Sound's own";
        value = (*own)[field];
    }
    return field == 2 ? std::to_string(value) + "%" : milliseconds(value);
}
std::string SettingsEditor::envelopeSummary(const Settings &s, int part) const {
    std::string text;
    auto own = soundEnvelope(s, part);
    for (int field = 0; field < 4; field++) {
        int value = s.envelopes[part].values[field];
        if (value < 0 && own) value = (*own)[field];
        if (field) text += " / ";
        text += value < 0 ? "-" : std::to_string(value) + (field == 2 ? "%" : "");
    }
    return text;
}
int SettingsEditor::chordOctaveValue(const Settings &s) const {
    return scope_ < 0 ? s.chordOctave : s.degreeChordOctave[scope_];
}
int SettingsEditor::chordConfigValue(const Settings &s) const {
    if (combination_ == 6) return chordOctaveValue(s);
    if (combination_ == 7) return s.tonic;
    if (combination_ == 8) return s.degreeChordRoot[scope_];
    if (combination_ == chordHoldField) return s.hold;
    if (combination_ == chordScaleField) return s.chordScale;
    if (combination_ == 5) return s.combinationMs;
    if (combination_ == 4) return 0;
    return scope_ < 0 ? s.combinations[combination_] : s.degreeCombinations[scope_][combination_];
}
int SettingsEditor::envelopeValue(const Settings &s) const {
    return envelopeField_ == 4 ? 0 : s.envelopes[envelopePart_].values[envelopeField_];
}
bool SettingsEditor::appField(int index) {
    return index == 0 || index == 1 || (index >= 3 && index <= 7) || index == 17 ||
           index == 18 || index == 22 || (index >= 26 && index <= systemVolumeField) ||
           index == outputField;
}
bool SettingsEditor::appPage() const {
    return page_ == Page::Combinations || page_ == Page::FrontNotes ||
           page_ == Page::VolumeEffects || (page_ == Page::Setup && tab_ == Tab::App);
}
const std::vector<int> &SettingsEditor::rows() const {
    static const std::vector<int> style{2, 8, backingField, frontSoundField, 19, 20};
    static const std::vector<int> app{18, volumeEffectsField, 3, 22};
    static const std::vector<int> about;
    static const std::vector<int> backing[backingTabCount]{
        {chordMixField, chordMixField + 1, chordMixField + 2, bassMixField, drumMixField},
        {11, 13, 24, chordVoicingField},
        {9, arpSoundField, arpEnvelopeField},
        {12, 21, 25, padVoicingField},
        {bassPatternField, bassSoundField, bassEnvelopeField},
        {drumKitField, drumPatternField}};
    static const std::vector<int> frontSound{14, 23, frontMixField};
    if (page_ == Page::VolumeEffects) return volumeEffectsRows;
    if (page_ == Page::Backing) return backing[backingTab_];
    if (page_ == Page::FrontSound) return frontSound;
    return tab_ == Tab::About ? about : tab_ == Tab::Style ? style : app;
}
int SettingsEditor::rowIndex() const {
    const auto &list = rows();
    return int(std::find(list.begin(), list.end(), selection_) - list.begin());
}
const char *SettingsEditor::settingName(int index) {
    static const char *names[] = {"Key",
                                  "Scale",
                                  "Preset",
                                  "Chord config",
                                  "Backing volume",
                                  "Front volume",
                                  "Echo",
                                  "Strum time",
                                  "Tempo",
                                  "Arp pattern",
                                  "Backing layers",
                                  "Chord pattern",
                                  "Pad motion",
                                  "Chord sound",
                                  "Front sound",
                                  "" /* Hold moved to Chord config. */,
                                  "" /* Retired field 16; preserve internal field IDs. */,
                                  "App Vol",
                                  "Theme",
                                  "Save as new preset",
                                  "Restore preset...",
                                  "Pad sound",
                                  "Front notes", "Front envelope", "Chord envelope", "Pad envelope",
                                  "EQ Low", "EQ Mid", "EQ High", "Volume & effects", "System Vol",
                                  "Arp sound", "Arp envelope", "Chord mix", "Arp mix", "Pad mix",
                                  "Front sound", "Drum mix", "Drum kit", "Drum pattern",
                                  "Bass mix", "Bass sound", "Bass pattern", "Bass envelope",
                                  "Output", "Chord voicing", "Pad voicing", "Front level"};
    return names[index];
}
std::string SettingsEditor::rowName(int index) const {
    if (page_ == Page::Backing || page_ == Page::FrontSound) {
        if (mixField(index)) return modeName(mixLayer(index));
        if (soundField(index)) return "Sound";
        if (envelopeRow(index)) return "Envelope";
        if (index == 12) return "Motion";
        if (index == 9 || index == 11 || index == drumPatternField || index == bassPatternField)
            return "Pattern";
        if (index == drumKitField) return "Kit";
        if (voicingField(index)) return "Voicing";
        if (index == frontMixField) return "Level";
    }
    return settingName(index);
}
std::string SettingsEditor::rowDescription() const {
    switch (page_) {
    case Page::Envelope: {
        static const char *text[]{"Time to reach full volume", "Time to fall to the sustain level",
                                  "Level while the note is held", "Fade after the note ends",
                                  "Reset this envelope to the preset"};
        return text[envelopeField_];
    }
    case Page::FrontNotes: {
        static const char *all[]{"Chord notes or the notes of a scale",
                                 "Strum, or shift the front octave",
                                 "Time from the first to the last note",
                                 "Reset both modes, every key and K/L",
                                 "Scale of the front notes",
                                 "Tonic of the scale notes"};
        static const char *key[]{"Pitch this key plays", "Octave this key plays",
                                 "Reset this key in this mode"};
        return frontKey_ < 0 ? all[frontField_] : key[frontField_];
    }
    case Page::Combinations: {
        if (combination_ < 4) return "Quality when these buttons are pressed together";
        static const char *text[]{"", "", "", "",
                                  "Reset this tab's chord settings",
                                  "Window for pressing chord buttons together",
                                  "Octave of the chord roots",
                                  "Tonic of the chord grid",
                                  "Fixed root for this degree",
                                  "Keep the chord after releasing its buttons",
                                  "Seven-note scale of the chord roots"};
        return text[combination_];
    }
    default: break;
    }
    switch (selection_) {
    case 2: return "Switch to another saved style";
    case 8: return "Speed and swing of patterns";
    case backingField: return "Layers, mix, patterns, sounds and envelopes";
    case frontSoundField: return "Sound and envelope of Z-M and strums";
    case 19: return "Copy these Style settings to a new preset";
    case 20: return "Undo every Style edit to this preset";
    case 18: return "Dark or light appearance";
    case volumeEffectsField: return "Volumes, echo and EQ";
    case 3: return "Key, scale, octave, Hold and chord qualities";
    case 22: return "What Z-M play and what K/L do";
    case systemVolumeField: return "Device output for every app";
    case 17: return "Overall ZeroChord level";
    case 4: return "Chord, Arp and Pad together";
    case 5: return "Z-M notes and strums";
    case 6: return "Repeats in time with the tempo";
    case outputField: return "Speaker adds bass harmonics; Auto follows the jack";
    case chordVoicingField: case padVoicingField: return "How the chord notes are spread";
    case frontMixField: return "Level of Z-M and strums in this style";
    case 26: return "Bass of the whole mix";
    case 27: return "Middle of the whole mix";
    case 28: return "Treble of the whole mix";
    case chordMixField: return "Chord notes together. Enter mutes or unmutes.";
    case chordMixField + 1: return "Chord tones one at a time. Enter mutes or unmutes.";
    case chordMixField + 2: return "Sustained harmony. Enter mutes or unmutes.";
    case drumMixField: return "Drum groove on the beat. Enter mutes or unmutes.";
    case bassMixField: return "Bass line under the chord. Enter mutes or unmutes.";
    case bassPatternField: return "Rhythm and notes of the bass line";
    case drumKitField: return "Drum set for the groove";
    case drumPatternField: return "Rhythm the drums play";
    case 11: return "How the chord notes are struck";
    case 9: return "Order and rhythm of the chord tones";
    case 12: return "Volume movement of the pad";
    case 13: case 14: case 21: case arpSoundField: case bassSoundField:
        return "Instrument for this part";
    case 23: case 24: case 25: case arpEnvelopeField: case bassEnvelopeField:
        return "Attack, decay, sustain and release";
    }
    return "";
}
std::string SettingsEditor::backingLabel(unsigned layers) {
    // Preset files keep their existing mode names. Display layers in menu order.
    std::string label;
    for (auto layer : backingLayerOrder) {
        if (!(layers & backingLayers(layer))) continue;
        if (!label.empty()) label += " + ";
        label += modeName(layer);
    }
    if (label.empty()) return modeName(PlaybackMode::FrontOnly);
    if ((layers & allLayers) == allLayers) return "All layers";
    return label;
}
std::string SettingsEditor::backingLabel(const Settings &s) {
    return backingLabel(s.audibleLayers());
}
const char *SettingsEditor::combinationName(int index) {
    static const char *names[] = {"Top + middle", "Top + bottom", "Middle + bottom", "All three",
                                  "Restore defaults...", "Chord combo timing", "Octave", "Key",
                                  "Root", "Hold", "Scale"};
    return names[index];
}
std::vector<int> SettingsEditor::combinationRows() const {
    return scope_ < 0 ? std::vector<int>{6, 7, chordScaleField, chordHoldField, 0, 1, 2, 3, 5, 4}
                      : std::vector<int>{6, 8, 0, 1, 2, 3, 4};
}
std::string SettingsEditor::scopeName() const {
    return scope_ < 0 ? "All degrees"
                      : std::string(degreeName(scope_)) + " (" +
                            noteName(instrument_.settings.chordRoot(scope_)) +
                            ")";
}
bool SettingsEditor::soundPicker() const {
    return picker_ && (page_ == Page::Backing || page_ == Page::FrontSound) && soundField(selection_);
}
std::vector<std::string> SettingsEditor::soundCategories() const {
    auto categories = soundLibrary().categories();
    const auto &s = instrument_.settings;
    const auto &d = instrument_.library.current(s).sound;
    if (!soundLibrary().choice(valueFor(s, selection_)) ||
        !soundLibrary().choice(valueFor(d, selection_)))
        categories.push_back("Saved");
    return categories;
}
const char *SettingsEditor::frontSettingName(int field) const {
    static const char *all[]{"Front mode", "K/L action", "Strum time", "Restore all front notes...",
                             "Scale", "Key"};
    static const char *key[]{"Note", "Octave", "Restore this key..."};
    return frontKey_ < 0 ? all[field] : key[field];
}
std::vector<int> SettingsEditor::frontRows() const {
    if (frontKey_ >= 0)
        return {0, 1, 2};
    const auto &s = instrument_.settings;
    std::vector<int> rows{0};
    if (s.frontMode) {
        rows.push_back(frontKeyField);
        rows.push_back(4);
    }
    rows.push_back(1);
    if (s.frontAction[s.frontMode] == 0) rows.push_back(2);
    rows.push_back(3);
    return rows;
}
bool SettingsEditor::frontReset() const {
    return frontKey_ < 0 ? frontField_ == 3 : frontField_ == 2;
}
int SettingsEditor::frontValue(const Settings &s, int field) const {
    if (frontKey_ < 0) {
        const int values[]{s.frontMode,
                           s.frontAction[s.frontMode],
                           s.frontMode ? s.scaleStrumMs : s.strumMs,
                           0,
                           s.frontFollow ? -1 : s.scale,
                           s.frontFollow ? -1 : s.frontTonic};
        return values[field];
    }
    if (field == 0)
        return s.frontMode ? s.frontDegree[frontKey_]
                           : (s.frontSource[frontKey_] == 4 ? 16 + s.frontOffset[frontKey_]
                                                            : s.frontSource[frontKey_]);
    return field == 1 ? (s.frontMode ? s.scaleOctave[frontKey_] : s.frontOctave[frontKey_]) : 0;
}
void SettingsEditor::assignFront(Settings &s, int field, int value) {
    if (frontKey_ < 0) {
        if (field == 0)
            s.frontMode = value;
        if (field == 1)
            s.frontAction[s.frontMode] = value;
        if (field == 2)
            (s.frontMode ? s.scaleStrumMs : s.strumMs) = value;
        if (field == 4 || field == frontKeyField) {
            // Key and Scale follow the chord together; choosing either keeps the other.
            const int tonic = s.frontKeyTonic(), scale = s.frontKeyScale();
            s.frontFollow = value < 0;
            s.frontTonic = field == frontKeyField && value >= 0 ? value : tonic;
            s.scale = field == 4 && value >= 0 ? value : scale;
        }
    } else if (field == 0) {
        if (s.frontMode)
            s.frontDegree[frontKey_] = value;
        else {
            s.frontSource[frontKey_] = value >= 16 ? 4 : value;
            s.frontOffset[frontKey_] = value >= 16 ? value - 16 : 0;
        }
    } else if (field == 1) {
        (s.frontMode ? s.scaleOctave[frontKey_] : s.frontOctave[frontKey_]) = value;
    }
}
std::string SettingsEditor::frontValueName(const Settings &s, int field) const {
    int value = frontValue(s, field);
    if ((frontKey_ < 0 && field == 3) || (frontKey_ >= 0 && field == 2))
        return "";
    if (frontKey_ < 0) {
        if (field == 0)
            return value ? "Scale notes" : "Chord notes";
        if (field == 1)
            return value ? "Octave shift" : "Strum";
        if (field == 4)
            return value < 0 ? std::string("Chord (") + scaleName(s.chordScale) + ")" : scaleName(value);
        if (field == frontKeyField)
            return value < 0 ? "Chord (" + noteName(s.tonic) + ")" : noteName(value);
        return milliseconds(value);
    }
    if (field == 1) {
        if (value != -2) return std::to_string(value);
        Chord chord{s.chordRoot(6), -1, 6, Quality::Major};
        int pitch = frontNotes(chord, s)[frontKey_];
        return "Auto octave (" + std::to_string(pitch / 12 - 1) + ")";
    }
    if (s.frontMode) {
        static const char *degrees[]{"I", "II", "III", "IV", "V", "VI", "VII"};
        Chord chord{s.chordRoot(6), -1, 6, Quality::Major};
        int pitch = frontNotes(chord, s)[frontKey_];
        int interval = (pitch - s.frontKeyTonic() + 12) % 12;
        // Pentatonic and blues have fewer notes than keys: number them and mark the octave up.
        const int size = scaleInfo(s.frontKeyScale()).size;
        std::string name = size == 7 ? degrees[value]
                                     : "Note " + std::to_string(value % size + 1) +
                                           (value >= size ? " up" : "");
        return name + " (+" + std::to_string(interval) + ")";
    }
    if (value < 0) return "Auto note";
    if (value < 4) return "Chord tone " + std::to_string(value + 1);
    return "Fixed +" + std::to_string(value - 16) + " semitones";
}
int SettingsEditor::valueFor(const Settings &s, int index) const {
    if (index >= 26 && index <= 28) return s.eqDb[index - 26];
    if (mixField(index)) return s.mix(mixLayer(index));
    if (soundField(index)) {
        auto [program, id] = soundSource(s, index);
        return soundLibrary().selected(program, id);
    }
    if (index == drumKitField) return soundLibrary().selected(s.drumKit, s.drumSound);
    if (index == drumPatternField) return s.drumPattern;
    if (index == bassPatternField) return s.bassPattern;
    if (index == outputField) return s.output;
    if (index == frontMixField) return s.frontMix;
    if (index == chordVoicingField) return s.chordVoicing;
    if (index == padVoicingField) return s.padVoicing;
    const int values[] = {s.tonic,
                          s.scale,
                          s.preset,
                          0,
                          s.chordVolume,
                          s.harpVolume,
                          s.echo,
                          s.strumMs,
                          s.bpm,
                          s.pattern,
                          int(s.mode),
                          s.chordPattern,
                          s.padMotion,
                          s.chordProgram,
                          s.harpProgram,
                          0,
                          0,
                          s.masterVolume,
                          s.theme,
                          0,
                          0,
                          s.padProgram,
                          0};
    return index < int(std::size(values)) ? values[index] : 0;
}
int SettingsEditor::settingValueNumber() const {
    if (selection_ == systemVolumeField) return systemVolume_.value_or(0);
    return valueFor(instrument_.settings, selection_);
}
int SettingsEditor::fieldValue() const {
    const auto &s = instrument_.settings;
    switch (page_) {
    case Page::Setup:
    case Page::Backing:
    case Page::FrontSound:
    case Page::VolumeEffects: return settingValueNumber();
    case Page::Envelope: return envelopeValue(s);
    case Page::FrontNotes: return frontValue(s, frontField_);
    case Page::Combinations: return chordConfigValue(s);
    }
    return 0;
}
int SettingsEditor::defaultValue() const {
    if (page_ == Page::Envelope)
        return envelopeValue(instrument_.library.current(instrument_.settings).sound);
    if (page_ == Page::FrontNotes) {
        Settings defaults;
        // Reset a mode's field without switching modes.
        if (!(frontKey_ < 0 && frontField_ == 0))
            defaults.frontMode = instrument_.settings.frontMode;
        return frontValue(defaults, frontField_);
    }
    if (page_ == Page::Combinations) return chordConfigValue(Settings{});
    if (appField(selection_))
        return valueFor(Settings{}, selection_);
    return valueFor(instrument_.library.current(instrument_.settings).sound, selection_);
}
bool SettingsEditor::fieldModified(int index) const {
    const auto &s = instrument_.settings;
    const auto &preset = instrument_.library.current(s).sound;
    if (index == systemVolumeField) return false;
    if (index == volumeEffectsField)
        return std::any_of(volumeEffectsRows.begin(), volumeEffectsRows.end(),
                           [&](int field) { return fieldModified(field); });
    if (envelopeRow(index))
        return s.envelopes[envelopePartFor(index)] != preset.envelopes[envelopePartFor(index)];
    if (mixField(index)) {
        auto layer = mixLayer(index);
        return s.mix(layer) != preset.mix(layer) || s.enabled(layer) != preset.enabled(layer);
    }
    if (index == drumKitField)
        return s.drumKit != preset.drumKit || s.drumSound != preset.drumSound;
    if (index == backingField) {
        for (int field : {chordMixField, chordMixField + 1, chordMixField + 2, bassMixField,
                          drumMixField, 11, 13, 24, 9, arpSoundField, arpEnvelopeField, 12, 21, 25,
                          bassPatternField, bassSoundField, bassEnvelopeField, drumKitField,
                          drumPatternField, chordVoicingField, padVoicingField})
            if (fieldModified(field)) return true;
        return false;
    }
    if (index == frontSoundField)
        return fieldModified(14) || fieldModified(23) || fieldModified(frontMixField);
    if (index == 2)
        return instrument_.library.modified(s);
    if (index == 22) {
        const Settings d;
        return s.frontSource != d.frontSource || s.frontOffset != d.frontOffset ||
               s.frontOctave != d.frontOctave || s.frontMode != d.frontMode ||
               s.frontDegree != d.frontDegree || s.scaleOctave != d.scaleOctave ||
               s.frontAction != d.frontAction || s.strumMs != d.strumMs ||
               s.scaleStrumMs != d.scaleStrumMs || s.frontFollow != d.frontFollow ||
               (!s.frontFollow && (s.scale != d.scale || s.frontTonic != d.frontTonic));
    }
    if (index == 3) {
        const Settings d;
        return d.combinations != s.combinations || d.hold != s.hold ||
               d.combinationMs != s.combinationMs || d.chordOctave != s.chordOctave ||
               d.tonic != s.tonic || d.chordScale != s.chordScale ||
               d.degreeChordRoot != s.degreeChordRoot ||
               d.degreeChordOctave != s.degreeChordOctave ||
               d.degreeCombinations != s.degreeCombinations;
    }
    if (index >= 19 && index <= 20)
        return false;
    if (index == 8)
        return s.bpm != preset.bpm || s.swingMode != preset.swingMode ||
               s.swingAmount != preset.swingAmount;
    if (soundField(index))
        return valueFor(s, index) != valueFor(preset, index) ||
               soundSource(s, index).second != soundSource(preset, index).second;
    if (appField(index))
        return valueFor(s, index) != valueFor(Settings{}, index);
    return valueFor(s, index) != valueFor(preset, index);
}
std::string SettingsEditor::settingValue(int index) const {
    const auto &s = instrument_.settings;
    if (index >= 26 && index <= 28) {
        int gain = s.eqDb[index - 26];
        return (gain > 0 ? "+" : "") + std::to_string(gain) + " dB";
    }
    if (mixField(index)) return std::to_string(valueFor(s, index)) + "%";
    if (envelopeRow(index)) return envelopeSummary(s, envelopePartFor(index));
    if (soundField(index)) {
        auto [program, id] = soundSource(s, index);
        return soundLibrary().name(program, id);
    }
    switch (index) {
    case 0:
        return noteName(s.tonic);
    case 1:
        return scaleName(s.scale);
    case 2:
        return instrument_.library.current(s).name;
    case 3:
        return noteName(s.tonic) + " " + scaleInfo(s.chordScale).lower;
    case 22:
        return s.frontMode ? "Scale notes" : "Chord notes";
    case frontSoundField:
        return settingValue(14);
    case volumeEffectsField:
    case 19:
    case 20:
        return "";
    case systemVolumeField:
        return systemVolume_ ? std::to_string(*systemVolume_) + "%" : "Unavailable";
    case 7:
        return milliseconds(s.strumMs);
    case 8:
        return std::to_string(s.bpm) + " BPM" +
               (s.swingMode ? std::string(", ") + swingModeName(s.swingMode) + " swing " +
                                  std::to_string(s.swingAmount) + "%"
                            : "");
    case outputField:
        if (s.output != OutputAuto) return s.output == OutputSpeaker ? "Speaker" : "Headphones";
        return instrument_.jack < 0 ? "Auto" : instrument_.jack ? "Auto (headphones)" : "Auto (speaker)";
    case chordVoicingField:
        return voicingName(s.chordVoicing);
    case padVoicingField:
        return voicingName(s.padVoicing);
    case 9:
        return patterns(PlaybackMode::Arp)[s.pattern].name;
    case backingField:
        return backingLabel(s);
    case 18:
        return s.theme ? "Light" : "Dark";
    case 11:
        return patterns(PlaybackMode::Chord)[s.chordPattern].name;
    case 12:
        return patterns(PlaybackMode::Pad)[s.padMotion].name;
    case drumKitField:
        return drumKitName(s);
    case drumPatternField:
        return patterns(PlaybackMode::Drums)[s.drumPattern].name;
    case bassPatternField:
        return patterns(PlaybackMode::Bass)[s.bassPattern].name;
    default:
        return std::to_string(valueFor(s, index)) + "%";
    }
}
void SettingsEditor::assign(Settings &s, int index, int value) {
    if (index >= 26 && index <= 28) {
        s.eqDb[index - 26] = value;
        return;
    }
    if (mixField(index)) {
        s.mix(mixLayer(index)) = value;
        return;
    }
    if (soundField(index)) {
        auto &id = index == 14 ? s.frontSound
                   : index == 21 ? s.padSound
                   : index == bassSoundField ? s.bassSound
                   : index == arpSoundField ? s.arpSound : s.chordSound;
        auto &program = index == 14 ? s.harpProgram
                        : index == 21 ? s.padProgram
                        : index == bassSoundField ? s.bassProgram
                        : index == arpSoundField ? s.arpProgram : s.chordProgram;
        if (auto *sound = soundLibrary().choice(value); sound && !sound->bank.empty()) {
            id = sound->id;
            if (program < 0) program = soundSource(s, index).first;
        } else {
            id.clear();
            program = value;
        }
        return;
    }
    switch (index) {
    case 0:
        s.tonic = value;
        break;
    case 1:
        s.scale = value;
        break;
    case 4:
        s.chordVolume = value;
        break;
    case 5:
        s.harpVolume = value;
        break;
    case 6:
        s.echo = value;
        break;
    case 7:
        s.strumMs = value;
        break;
    case 8:
        s.bpm = value;
        break;
    case 9:
        s.pattern = value;
        break;
    case 10:
        s.mode = PlaybackMode(value);
        break;
    case 17:
        s.masterVolume = value;
        break;
    case 18:
        s.theme = value;
        break;
    case 11:
        s.chordPattern = value;
        break;
    case 12:
        s.padMotion = value;
        break;
    case drumKitField:
        if (auto *sound = soundLibrary().choice(value); sound && !sound->bank.empty())
            s.drumSound = sound->id; // drumKit stays as the built-in fallback.
        else if (value == s.drumKit && !soundLibrary().find(s.drumSound)) {
            // The kit already playing while its pack is missing: keep the pack preference.
        } else {
            s.drumSound.clear();
            s.drumKit = value;
        }
        break;
    case drumPatternField:
        s.drumPattern = value;
        break;
    case bassPatternField:
        s.bassPattern = value;
        break;
    case outputField:
        s.output = value;
        break;
    case chordVoicingField:
        s.chordVoicing = value;
        break;
    case padVoicingField:
        s.padVoicing = value;
        break;
    case frontMixField:
        s.frontMix = value;
        break;
    }
}
void SettingsEditor::setSettingValue(int value) {
    if (selection_ == systemVolumeField) {
        if (value == systemVolume_) return;
        if (writeSystemVolume_(value)) {
            systemVolume_ = value;
        } else {
            refreshSystemVolume();
            status_ = "Could not change system volume";
        }
        return;
    }
    auto &s = instrument_.settings;
    if (selection_ == 8) {
        s.bpm = value; // Keep the accompaniment clock running.
        return;
    }
    if (selection_ == 2)
        instrument_.library.select(s, value);
    else if (selection_ == 19) {
        if (value == 1) {
            instrument_.library.saveAs(s);
            status_ = "Saved " + instrument_.library.current(s).name;
        }
    } else if (selection_ == 20) {
        if (value == 1) {
            instrument_.library.restore(s);
            status_ = "Preset restored";
        }
    } else
        assign(s, selection_, value);
    instrument_.configure();
}
void SettingsEditor::buildChoices() {
    choices_.clear();
    const auto &s = instrument_.settings;
    if (page_ == Page::VolumeEffects && selection_ == outputField) {
        choices_ = {{OutputAuto, "Auto", "Speaker sound unless headphones are plugged in"},
                    {OutputSpeaker, "Speaker", "Bass harmonics and a little click for small speakers"},
                    {OutputHeadphones, "Headphones", "Full range with no extra processing"}};
        return;
    }
    if (page_ == Page::VolumeEffects) {
        const bool equalizer = selection_ >= 26 && selection_ <= 28;
        const int low = equalizer ? -12 : 0, high = equalizer ? 12 : selection_ == 6 ? 60 : 100;
        auto name = [&](int v) {
            return equalizer ? (v > 0 ? "+" : "") + std::to_string(v) + " dB" : std::to_string(v) + "%";
        };
        for (int v = low; v <= high; v += equalizer ? 1 : 5) choices_.push_back({v, name(v), ""});
        const int current = settingValueNumber();
        if (std::none_of(choices_.begin(), choices_.end(), [&](const Choice &c) { return c.value == current; })) {
            choices_.push_back({current, name(current), ""});
            std::sort(choices_.begin(), choices_.end(),
                      [](const Choice &a, const Choice &b) { return a.value < b.value; });
        }
        return;
    }
    if (settingListPage() && selection_ == 8) {
        for (int bpm = 40; bpm <= 240; bpm++)
            choices_.push_back({bpm, std::to_string(bpm) + " BPM", "Speed of patterns and arpeggios"});
        return;
    }
    if (page_ == Page::Envelope) {
        if (envelopeField_ == 4) {
            choices_ = {{0, "Keep envelope", "Keep this part's envelope"},
                        {1, "Restore envelope", "Reset only this part's envelope to preset defaults"}};
            return;
        }
        static const char *descriptions[]{"Time to reach full volume", "Time to fall to the sustain level",
                                           "Level while the note is held", "Fade after the note ends"};
        static const char *own[]{"The sound's own attack", "The sound's own decay",
                                 "The sound's own sustain", "The sound's own release"};
        const auto sound = soundEnvelope(s, envelopePart_);
        const int ownValue = sound ? (*sound)[envelopeField_] : -1;
        auto name = [&](int value) {
            if (value < 0 && !sound) return std::string("Sound's own");
            int shown = value < 0 ? ownValue : value;
            return envelopeField_ == 2 ? std::to_string(shown) + "%" : milliseconds(shown);
        };
        // -1 follows the sound. It is listed at the sound's own value, not as a separate item.
        choices_.push_back({-1, name(-1), own[envelopeField_]});
        auto add = [&](int value) {
            if (value < 0 || value == ownValue ||
                std::any_of(choices_.begin(), choices_.end(),
                            [&](const Choice &c) { return c.value == value; }))
                return;
            choices_.push_back({value, name(value), descriptions[envelopeField_]});
        };
        if (envelopeField_ == 2)
            for (int value = 0; value <= 100; value += 5) add(value);
        else
            for (int value : {0, 5, 10, 20, 50, 100, 200, 300, 500, 750, 1000, 1500, 2000, 3000, 5000})
                add(value);
        add(envelopeValue(s));
        add(defaultValue());
        auto order = [&](const Choice &c) { return c.value < 0 ? ownValue : c.value; };
        std::stable_sort(choices_.begin(), choices_.end(),
                         [&](const Choice &a, const Choice &b) { return order(a) < order(b); });
        return;
    }
    if (page_ == Page::FrontNotes) {
        if (frontReset())
            choices_ = {{0, "Keep settings", "Keep the current front note settings"},
                        {1, frontKey_ < 0 ? "Restore all front notes" : "Restore this key",
                         frontKey_ < 0 ? "Reset both modes, all keys and K/L settings"
                                       : "Reset this key's note and octave in this mode"}};
        else if (frontKey_ < 0) {
            if (frontField_ == 0)
                choices_ = {{0, "Chord notes", "Follow the selected chord and its quality"},
                            {1, "Scale notes", "I-VII of the front key; independent of chords"}};
            else if (frontField_ == 4)
            {
                choices_ = {{-1, std::string("Follow chord (") + scaleName(s.chordScale) + ")",
                             "Use the Chord config key and scale"}};
                for (int scale : scaleMenu(false))
                    choices_.push_back({scale, scaleName(scale),
                                        scaleInfo(scale).size == 7
                                            ? "Scale notes only; chord roots stay independent"
                                            : "Fewer notes than keys; continues an octave up"});
            }
            else if (frontField_ == frontKeyField) {
                choices_.push_back({-1, "Follow chord (" + noteName(s.tonic) + ")",
                                    "Use the Chord config key and scale"});
                for (int n = 0; n < 12; n++)
                    choices_.push_back({n, noteName(n), "Scale notes start here; chords stay"});
            } else if (frontField_ == 1)
                choices_ = {{0, "Strum", "K: M to Z / L: Z to M"},
                            {1, "Octave shift", "K: down / L: up; front notes only"}};
            else {
                for (int time = 60; time <= 600; time += 30)
                    choices_.push_back(
                        {time, milliseconds(time), "Time from first to last front note"});
                int current = frontValue(s, frontField_);
                if (std::none_of(choices_.begin(), choices_.end(),
                                 [&](const Choice &c) { return c.value == current; }))
                    choices_.push_back({current, milliseconds(current), "Current strum time"});
                std::sort(choices_.begin(), choices_.end(),
                          [](const Choice &a, const Choice &b) { return a.value < b.value; });
            }
        } else if (frontField_ == 1) {
            auto copy = s;
            assignFront(copy, 1, -2);
            choices_.push_back({-2, frontValueName(copy, 1), "Follow the original note layout"});
            for (int octave = -1; octave <= 9; octave++)
                choices_.push_back({octave, "Octave " + std::to_string(octave),
                                    "Actual note octave: middle C is C4"});
        } else if (s.frontMode) {
            for (int degree = 0; degree < 7; degree++) {
                auto copy = s;
                assignFront(copy, 0, degree);
                choices_.push_back(
                    {degree, frontValueName(copy, 0), "Degree of the front key and scale"});
            }
        } else {
            auto sample = s;
            const Chord chord{s.chordRoot(6), -1, 6, Quality::Major};
            auto addChordNote = [&](int value, const char *description) {
                assignFront(sample, 0, value);
                int pitch = frontNotes(chord, sample)[frontKey_];
                int interval = ((pitch - chord.root) % 12 + 12) % 12;
                choices_.push_back({value, frontValueName(sample, 0) + " (+" +
                                               std::to_string(interval) + ")",
                                    description});
            };
            addChordNote(-1, "Follow the original chord-based layout");
            for (int tone = 0; tone < 4; tone++)
                addChordNote(tone, "Follow the chord as its quality changes");
            static const char *intervals[]{"Root",      "Minor 2nd", "Major 2nd", "Minor 3rd",
                                           "Major 3rd", "Fourth",    "Tritone",   "Fifth",
                                           "Minor 6th", "Major 6th", "Minor 7th", "Major 7th"};
            for (int offset = 0; offset < 12; offset++)
                choices_.push_back({16 + offset,
                                    "Fixed +" + std::to_string(offset) + " semitones",
                                    offset ? std::string(intervals[offset]) + " above the chord root"
                                           : "Play the chord root"});
        }
        return;
    }
    if (page_ == Page::Combinations) {
        if (combination_ == 7 || combination_ == 8) {
            if (combination_ == 8)
                choices_.push_back({-1, "Follow Key (" + noteName(rootFor(s.tonic, s.chordScale, scope_)) + ")",
                                    "Default degree in the selected key"});
            for (int pitch = 0; pitch < 12; pitch++)
                choices_.push_back({pitch, noteName(pitch), combination_ == 7 ? "Transpose degrees that follow Key" : "Absolute root; stays fixed when Key changes"});
        } else if (combination_ == chordScaleField)
            for (int scale : scaleMenu(true)) {
                std::string roots;
                for (int column : {6, 0, 1, 2, 3, 4, 5})
                    roots += (roots.empty() ? "" : " ") + noteName(rootFor(s.tonic, scale, column));
                choices_.push_back({scale, scaleName(scale), "Roots " + roots});
            }
        else if (combination_ == 6) {
            if (scope_ >= 0)
                choices_.push_back({-2, "Follow All (" + std::to_string(s.chordOctave) + ")", "Use the shared chord root octave"});
            for (int octave = -1; octave <= 7; octave++)
                choices_.push_back({octave, "Octave " + std::to_string(octave), "Root note octave; chord intervals stay together"});
        } else if (combination_ == 4)
            choices_ = {{0, "Keep settings", "Keep the current chord settings"},
                        {1, "Restore chord config",
                         scope_ < 0 ? "Reset Hold, key, scale, roots, octaves, mappings, timing"
                                    : "Follow Key, shared octave and chord mappings"}};
        else if (combination_ == chordHoldField)
            choices_ = {{0, "Off", "Clear the chord when you release its buttons"},
                        {1, "On", "Keep the chord after you release its buttons"}};
        else if (combination_ == 5) {
            for (int time = 0; time <= Settings::maxCombinationMs; time += 5)
                choices_.push_back({time, time ? milliseconds(time) : "Off",
                                    time ? "More time to press together, with later backing"
                                         : "Play each chord button immediately"});
            if (s.combinationMs % 5) {
                choices_.push_back({s.combinationMs, milliseconds(s.combinationMs),
                                    "Current recognition window"});
                std::sort(choices_.begin(), choices_.end(),
                          [](const Choice &a, const Choice &b) { return a.value < b.value; });
            }
        }
        else {
            if (scope_ >= 0)
                choices_.push_back(
                    {-1,
                     "Follow All (" +
                         std::string(qualityName(Quality(s.combinations[combination_]))) + ")",
                     "Inherit the shared mapping"});
            for (int n = 0; n < qualityCount; n++)
                choices_.push_back({n, qualityName(Quality(n)), qualityDescription(Quality(n))});
        }
        return;
    }
    if (selection_ == 2)
        for (int n : instrument_.library.order()) {
            const auto &p = instrument_.library.entries[n];
            auto value = instrument_.library.resolved(n);
            choices_.push_back(
                {n, p.name, backingLabel(value) + " / " + std::to_string(value.bpm) + " BPM"});
        }
    else if (selection_ == drumKitField) {
        for (const auto &[kit, name] : drumKits)
            choices_.push_back({kit, name, "Built-in General MIDI kit"});
        for (const auto &sound : soundLibrary().entries)
            if (sound.category == "Drums")
                choices_.push_back({sound.choice, sound.name, "Sound pack / " + sound.bank});
        const int current = valueFor(s, drumKitField);
        if (std::none_of(choices_.begin(), choices_.end(),
                         [&](const Choice &c) { return c.value == current; }))
            choices_.push_back({current, drumKitName(s), "Saved kit"});
    } else if (selection_ == 9 || selection_ == 11 || selection_ == 12 ||
               selection_ == drumPatternField || selection_ == bassPatternField) {
        auto mode = selection_ == 11                 ? PlaybackMode::Chord
                    : selection_ == 12               ? PlaybackMode::Pad
                    : selection_ == drumPatternField ? PlaybackMode::Drums
                    : selection_ == bassPatternField ? PlaybackMode::Bass
                                                     : PlaybackMode::Arp;
        const auto &list = patterns(mode);
        const int presetDefault = mode == PlaybackMode::Arp ? defaultValue() : -1;
        auto add = [&](int n) { choices_.push_back({n, list[n].name, list[n].description}); };
        for (int n = 0; n < int(list.size()); n++) {
            // Older mixed Arp patterns stay available to presets that use them.
            if (mode == PlaybackMode::Arp && (n == 6 || n == 8) && n != s.pattern &&
                n != presetDefault)
                continue;
            if (mode == PlaybackMode::Chord && n == sustainedChordPattern) continue;
            add(n);
            if (mode == PlaybackMode::Chord && n == 0) add(sustainedChordPattern);
        }
    } else if (soundField(selection_)) {
        for (const auto &sound : soundLibrary().entries)
            if (sound.category == soundCategory_)
                choices_.push_back(
                    {sound.choice, sound.name,
                     soundLibrary().sampleRateLabel(sound.choice) +
                         (sound.bank.empty() ? " / Built-in" : " / " + sound.bank)});
        if (soundCategory_ == "Saved") {
            for (const auto &source : {s, instrument_.library.current(s).sound}) {
                int value = valueFor(source, selection_);
                if (!soundLibrary().choice(value) &&
                    std::none_of(choices_.begin(), choices_.end(),
                                 [&](const Choice &c) { return c.value == value; }))
                    choices_.push_back(
                        {value, soundLibrary().name(value, ""), soundLibrary().sampleRateLabel(value)});
            }
        }
    } else if (selection_ == frontMixField) {
        for (int v = 0; v <= 100; v += 5) choices_.push_back({v, std::to_string(v) + "%", ""});
        if (s.frontMix % 5) {
            choices_.push_back({s.frontMix, std::to_string(s.frontMix) + "%", ""});
            std::sort(choices_.begin(), choices_.end(),
                      [](const Choice &a, const Choice &b) { return a.value < b.value; });
        }
    } else if (voicingField(selection_)) {
        choices_ = {{VoicingClose, "Close", "Near middle C, moving to the nearest notes"},
                    {VoicingOpen, "Open", "Spread wide, for strings and pads"},
                    {VoicingPower, "Power", "Root, fifth and octave, for rock guitar"},
                    {VoicingRich, "Rich", "Adds 9ths and 7ths; the bass keeps the root"}};
    } else if (selection_ == 18)
        choices_ = {{0, "Dark", "App-wide appearance"}, {1, "Light", "App-wide appearance"}};
    else if (selection_ == 19 || selection_ == 20)
        choices_ = {{0, "Keep preset", "Keep the current preset as it is"},
                    {1, selection_ == 19 ? "Create preset copy" : "Restore preset",
                     selection_ == 19 ? "Make a new preset from these Style settings"
                                      : "Reset only the current preset's Style settings"}};
}
Chord SettingsEditor::previewChord() const {
    const auto &s = instrument_.settings;
    int col = scope_ < 0 ? 6 : scope_;
    Quality q = Quality::Major;
    if (page_ == Page::Combinations && combination_ < 4) {
        int v = choices_[choice_].value;
        q = Quality(v < 0 ? s.combinations[combination_] : v);
    }
    return {s.chordRoot(col), -1, col, q};
}
void SettingsEditor::preview() {
    if (repeating_) {
        deferred_ = [this] { if (choice_ < int(choices_.size())) preview(); };
        return;
    }
    // A chord playing from O already sounds each change; do not interrupt it.
    if (!picker_ && backingOpen() && instrument_.active) return;
    ++previews_;
    if (page_ == Page::VolumeEffects) {
        // Audition with the saved mix so returning to playing needs no audio reconfiguration.
        if (selection_ == 5 || selection_ == 6) instrument_.auditionFront(instrument_.settings, 0);
        else if (selection_ != 17 && selection_ != systemVolumeField)
            instrument_.audition(previewChord(), instrument_.settings, false);
        return;
    }
    const bool setting = page_ == Page::Setup || page_ == Page::Backing || page_ == Page::FrontSound;
    if (confirmation() != Confirmation::None ||
        (page_ == Page::Combinations && (combination_ == 5 || combination_ == chordHoldField)) ||
        (setting && selection_ == 18))
        return;
    Settings sound = instrument_.settings;
    auto solo = [&](PlaybackMode layer) {
        // Audition one layer at full mix, even if it is muted or quiet in the preset.
        sound.setEnabledLayers(backingLayers(layer));
        sound.mix(layer) = 100;
    };
    if (page_ == Page::Envelope) {
        if (envelopeField_ == 4) return;
        sound.envelopes[envelopePart_].values[envelopeField_] = choices_[choice_].value;
        if (envelopePart_ == FrontEnvelope) instrument_.auditionFront(sound, 0);
        else {
            solo(envelopePart_ == PadEnvelope    ? PlaybackMode::Pad
                 : envelopePart_ == ArpEnvelope  ? PlaybackMode::Arp
                 : envelopePart_ == BassEnvelope ? PlaybackMode::Bass
                                                 : PlaybackMode::Chord);
            instrument_.audition(previewChord(), sound, true);
        }
        return;
    }
    if (page_ == Page::FrontNotes) {
        assignFront(sound, frontField_, choices_[choice_].value);
        instrument_.auditionFront(sound, std::max(frontKey_, 0));
        return;
    }
    if (setting) {
        if (selection_ == 2) {
            auto next = instrument_.library.resolved(choices_[choice_].value);
            applyPresetSettings(sound, next);
            sound.preset = choices_[choice_].value;
        } else
            assign(sound, selection_, choices_[choice_].value);
        if (selection_ == 21 || selection_ == 12) solo(PlaybackMode::Pad);
        if (selection_ == 13 || selection_ == 11 || selection_ == chordVoicingField)
            solo(PlaybackMode::Chord);
        if (selection_ == padVoicingField) solo(PlaybackMode::Pad);
        if (selection_ == arpSoundField || selection_ == 9) solo(PlaybackMode::Arp);
        if (selection_ == drumKitField || selection_ == drumPatternField) solo(PlaybackMode::Drums);
        if (selection_ == bassSoundField || selection_ == bassPatternField) solo(PlaybackMode::Bass);
    }
    if (setting && (selection_ == 14 || selection_ == frontMixField)) {
        instrument_.auditionFront(sound, 0);
        return;
    }
    if (page_ == Page::Combinations) {
        int value = choices_[choice_].value;
        if (combination_ == 6) {
            if (scope_ < 0) sound.chordOctave = value;
            else sound.degreeChordOctave[scope_] = value;
        } else if (combination_ == 7) sound.tonic = value;
        else if (combination_ == chordScaleField) sound.chordScale = value;
        else if (combination_ == 8) sound.degreeChordRoot[scope_] = value;
    }
    auto c = previewChord();
    c.root = sound.chordRoot(c.column);
    instrument_.audition(c, sound, setting);
}
void SettingsEditor::openBacking(bool fromPlay) {
    if (fromPlay) {
        playReturn_ = {selection_, int(tab_)}; // O is a side trip: settings stay where they were.
    }
    fromPlay_ = fromPlay;
    settingsOpen = true;
    page_ = Page::Backing;
    tab_ = Tab::Style;
    selectBackingTab(fromPlay ? 0 : backingTab_);
}
void SettingsEditor::selectBackingTab(int tab) {
    if (page_ == Page::Backing && rowIndex() < int(rows().size())) backingRows_[backingTab_] = rowIndex();
    backingTab_ = std::clamp(tab, 0, backingTabCount - 1);
    selection_ = rows()[std::min<int>(backingRows_[backingTab_], int(rows().size()) - 1)];
}
void SettingsEditor::openTempo() {
    tempo_ = true;
    tempoRow_ = 0;
    tempoReplace_ = true;
    tempoInput_ = std::to_string(instrument_.settings.bpm);
    tempoError_.clear();
}
SettingsEditor::SetupDestination SettingsEditor::setupDestination() const {
    if (mixField(selection_)) return SetupDestination::Mix;
    if (envelopeRow(selection_)) return SetupDestination::Envelope;
    switch (selection_) {
    case 8: return SetupDestination::Tempo;
    case backingField: return SetupDestination::Backing;
    case frontSoundField: return SetupDestination::FrontSound;
    case 22: return SetupDestination::FrontNotes;
    case 3: return SetupDestination::Combinations;
    case volumeEffectsField: return SetupDestination::VolumeEffects;
    default: return SetupDestination::Picker;
    }
}
SettingsEditor::Confirmation SettingsEditor::confirmation() const {
    switch (page_) {
    case Page::Setup:
        if (selection_ == 19) return Confirmation::Create;
        return selection_ == 20 ? Confirmation::Restore : Confirmation::None;
    case Page::Envelope:
        return envelopeField_ == 4 ? Confirmation::Restore : Confirmation::None;
    case Page::FrontNotes:
        return frontReset() ? Confirmation::Restore : Confirmation::None;
    case Page::Combinations:
        return combination_ == 4 ? Confirmation::Restore : Confirmation::None;
    default:
        return Confirmation::None;
    }
}
void SettingsEditor::openSetting() {
    switch (setupDestination()) {
    case SetupDestination::Picker:
        openPicker();
        return;
    case SetupDestination::Mix:
        toggleLayer();
        return;
    case SetupDestination::Backing:
        openBacking(false);
        break;
    case SetupDestination::Tempo:
        openTempo();
        return;
    case SetupDestination::Envelope:
        envelopeParent_ = page_;
        envelopePart_ = envelopePartFor(selection_);
        envelopeField_ = 0;
        page_ = Page::Envelope;
        break;
    case SetupDestination::FrontSound:
        page_ = Page::FrontSound;
        selection_ = rows()[frontSoundRow_];
        break;
    case SetupDestination::FrontNotes:
        page_ = Page::FrontNotes;
        break;
    case SetupDestination::Combinations:
        page_ = Page::Combinations;
        break;
    case SetupDestination::VolumeEffects:
        page_ = Page::VolumeEffects;
        selection_ = volumeEffectsRows[volumeEffectsRow_];
        refreshSystemVolume();
        break;
    }
    instrument_.panic();
}
void SettingsEditor::returnToSetup() {
    if (page_ == Page::Envelope) page_ = envelopeParent_;
    if (page_ == Page::VolumeEffects) selection_ = volumeEffectsField;
    if (page_ == Page::Backing) {
        backingRows_[backingTab_] = std::max(0, rowIndex());
        selection_ = backingField;
    }
    if (page_ == Page::FrontSound) {
        frontSoundRow_ = std::max(0, rowIndex());
        selection_ = frontSoundField;
    }
    if (page_ == Page::Combinations) selection_ = 3;
    if (page_ == Page::FrontNotes) selection_ = 22;
    if (page_ != Page::Setup) {
        page_ = Page::Setup;
        tab_ = appField(selection_) ? Tab::App : Tab::Style;
        tabRows_[int(tab_)] = std::max(0, rowIndex());
    }
    fromPlay_ = false;
}
void SettingsEditor::back() {
    if (picker_) {
        cancelPending();
    } else if (page_ == Page::Envelope) {
        page_ = envelopeParent_;
    } else if (page_ == Page::Backing && fromPlay_) {
        // O was a quick visit from playing: return there with the chord still sounding,
        // leaving the settings pages as the player last left them.
        backingRows_[backingTab_] = std::max(0, rowIndex());
        page_ = Page::Setup;
        selection_ = playReturn_.first;
        tab_ = Tab(playReturn_.second);
        fromPlay_ = false;
        settingsOpen = false;
        return;
    } else if (page_ != Page::Setup) {
        returnToSetup();
    } else {
        settingsOpen = false;
    }
    instrument_.panic();
}
void SettingsEditor::refreshSystemVolume() {
    systemVolume_ = readSystemVolume_ && writeSystemVolume_ ? readSystemVolume_() : std::nullopt;
}
SettingsEditor::RowKind SettingsEditor::rowKind(int field) const {
    switch (page_) {
    case Page::Envelope: return field == 4 ? RowKind::Opens : RowKind::Number;
    case Page::FrontNotes:
        if ((frontKey_ < 0 && field == 3) || (frontKey_ >= 0 && field == 2)) return RowKind::Opens;
        return (frontKey_ < 0 && field == 2) || (frontKey_ >= 0 && field == 1) ? RowKind::Number
                                                                           : RowKind::Opens;
    case Page::Combinations:
        return field == 5 || field == 6 ? RowKind::Number : RowKind::Opens;
    case Page::VolumeEffects:
        if (field == outputField) return RowKind::Opens;
        return field == systemVolumeField && !systemVolume_ ? RowKind::Plain : RowKind::Number;
    default: break;
    }
    if (mixField(field) || field == 8 || field == frontMixField) return RowKind::Number;
    if (field == 19 || field == 20 || field == backingField || field == frontSoundField ||
        field == volumeEffectsField || field == 3 || field == 22 || envelopeRow(field))
        return RowKind::Opens;
    return RowKind::Opens; // Choices open their list with Enter.
}
SettingsEditor::RowKind SettingsEditor::selectedKind() const {
    if (page_ == Page::Setup && tab_ == Tab::About) return RowKind::Plain;
    return rowKind(selected());
}
void SettingsEditor::adjust(int direction) {
    if (page_ == Page::Backing && mixField(selection_)) {
        adjustMix(direction);
        return;
    }
    if (page_ == Page::FrontSound && selection_ == frontMixField) { // Steps of 5, like a mix.
        auto &level = instrument_.settings.frontMix;
        const int next = std::clamp(level + direction * 5, 0, 100);
        if (next == level) return;
        level = next;
        instrument_.configure();
        buildChoices();
        choice_ = 0;
        for (int n = 0; n < int(choices_.size()); n++)
            if (choices_[n].value == next) choice_ = n;
        preview();
        return;
    }
    const auto kind = selectedKind();
    if (kind != RowKind::Number) return; // Choices use their list, not left/right.
    if (page_ == Page::VolumeEffects && selection_ == systemVolumeField) {
        refreshSystemVolume(); // The system may have changed it meanwhile.
        if (!systemVolume_) return;
    }
    buildChoices();
    const int count = int(choices_.size());
    if (count < 2) return;
    const int value = fieldValue();
    int index = -1;
    for (int n = 0; n < count; n++)
        if (choices_[n].value == value) index = n;
    // Numbers stop at their ends; a long hold takes bigger steps through long ranges.
    const int stride = repeating_ && fastRepeat_ && count > 40 ? 5 : 1;
    const int next = std::clamp(index < 0 ? (direction > 0 ? 0 : count - 1) : index + direction * stride,
                                0, count - 1);
    if (next == index) return;
    choice_ = next;
    try {
        applyChoice(choices_[next].value);
    } catch (const std::exception &e) {
        status_ = e.what();
        instrument_.configure();
        return;
    }
    preview();
}
void SettingsEditor::nextTab() {
    switch (page_) {
    case Page::Setup: jumpTab((int(tab_) + 1) % 3); break;
    case Page::Backing: jumpTab((backingTab_ + 1) % backingTabCount); break;
    case Page::Combinations: jumpTab((scope_ + 2) % 8); break;
    case Page::FrontNotes: jumpTab((frontKey_ + 2) % 8); break;
    default: break;
    }
}
void SettingsEditor::jumpTab(int tab) {
    switch (page_) {
    case Page::Setup:
        if (tab >= 3) return;
        if (tab_ != Tab::About) tabRows_[int(tab_)] = rowIndex();
        tab_ = Tab(tab);
        selection_ = rows().empty() ? -1 : rows()[tabRows_[int(tab_)]];
        break;
    case Page::Backing:
        if (tab >= backingTabCount) return;
        selectBackingTab(tab);
        return; // Keep a chord from O sounding.
    case Page::Combinations:
        if (tab >= 8) return;
        scope_ = tab - 1;
        if (scope_ >= 0 && (combination_ == 5 || combination_ == chordHoldField ||
                            combination_ == chordScaleField))
            combination_ = 0;
        if (combination_ == 7 || combination_ == 8) combination_ = scope_ < 0 ? 7 : 8;
        break;
    case Page::FrontNotes:
        if (tab >= 8) return;
        frontKey_ = tab - 1;
        frontField_ = 0;
        break;
    default: return;
    }
    instrument_.panic();
}
void SettingsEditor::adjustSwing(int direction) {
    // Swing applies live, like a mix level; typing still sets the tempo on Enter.
    auto &s = instrument_.settings;
    if (tempoRow_ == 1) s.swingMode = (s.swingMode + direction + swingModeCount) % swingModeCount;
    if (tempoRow_ == 2) s.swingAmount = std::clamp(s.swingAmount + direction * 5, 0, 100);
}
void SettingsEditor::restoreSwing() {
    auto &s = instrument_.settings;
    const auto &preset = instrument_.library.current(s).sound;
    s.swingMode = preset.swingMode;
    s.swingAmount = preset.swingAmount;
}
void SettingsEditor::adjustMix(int direction) {
    auto s = instrument_.settings;
    const auto layer = mixLayer(selection_);
    const int value = std::clamp(s.mix(layer) + direction * 5, 0, 100);
    const bool unmute = direction > 0 && !s.enabled(layer); // Raising unmutes.
    if (value == s.mix(layer) && !unmute) return;
    if (unmute) s.setEnabledLayers(s.enabledLayers() | backingLayers(layer));
    s.mix(layer) = value;
    instrument_.setBacking(s.enabledLayers(), s.layerMix);
}
void SettingsEditor::toggleLayer() {
    auto s = instrument_.settings;
    const auto layer = mixLayer(selection_);
    s.setEnabledLayers(s.enabledLayers() ^ backingLayers(layer));
    if (s.enabled(layer) && s.mix(layer) == 0) {
        const int preset = instrument_.library.current(s).sound.mix(layer);
        s.mix(layer) = preset > 0 ? preset : layer == PlaybackMode::Drums ? 60 : 100;
    }
    instrument_.setBacking(s.enabledLayers(), s.layerMix);
}
void SettingsEditor::restoreLayer() {
    auto s = instrument_.settings;
    const auto &d = instrument_.library.current(s).sound;
    const auto layer = mixLayer(selection_);
    const unsigned bit = backingLayers(layer);
    s.setEnabledLayers((s.enabledLayers() & ~bit) | (d.enabledLayers() & bit));
    s.mix(layer) = d.mix(layer);
    instrument_.setBacking(s.enabledLayers(), s.layerMix);
}
void SettingsEditor::openPicker() {
    if (soundField(selection_) && (page_ == Page::Backing || page_ == Page::FrontSound)) {
        auto *sound = soundLibrary().choice(settingValueNumber());
        soundCategory_ = sound ? sound->category : "Saved";
    }
    picker_ = true;
    buildChoices();
    choice_ = 0;
    if (confirmation() != Confirmation::None) choice_ = 1; // Enter confirms; Esc keeps.
    else {
        const int value = fieldValue();
        for (int n = 0; n < int(choices_.size()); n++)
            if (choices_[n].value == value)
                choice_ = n;
    }
    preview();
}
void SettingsEditor::applyChoice(int value) {
    auto &s = instrument_.settings;
    if (page_ == Page::Envelope) {
        if (envelopeField_ == 4) {
            if (value == 1)
                s.envelopes[envelopePart_] = instrument_.library.current(s).sound.envelopes[envelopePart_];
        } else s.envelopes[envelopePart_].values[envelopeField_] = value;
        instrument_.configure();
        return;
    }
    if (page_ == Page::FrontNotes) {
        if (frontReset() && value == 1) {
            const Settings d;
            if (frontKey_ >= 0) {
                if (s.frontMode) {
                    s.frontDegree[frontKey_] = d.frontDegree[frontKey_];
                    s.scaleOctave[frontKey_] = d.scaleOctave[frontKey_];
                } else {
                    s.frontSource[frontKey_] = d.frontSource[frontKey_];
                    s.frontOffset[frontKey_] = d.frontOffset[frontKey_];
                    s.frontOctave[frontKey_] = d.frontOctave[frontKey_];
                }
            } else {
                s.frontMode = d.frontMode;
                s.scale = d.scale;
                s.frontFollow = d.frontFollow;
                s.frontTonic = d.frontTonic;
                s.frontSource = d.frontSource;
                s.frontOffset = d.frontOffset;
                s.frontOctave = d.frontOctave;
                s.frontDegree = d.frontDegree;
                s.scaleOctave = d.scaleOctave;
                s.frontAction = d.frontAction;
                s.strumMs = d.strumMs;
                s.scaleStrumMs = d.scaleStrumMs;
                instrument_.frontOctaveShift.fill(0);
            }
        } else if (!frontReset())
            assignFront(s, frontField_, value);
        instrument_.configure();
        return;
    }
    if (settingListPage()) {
        setSettingValue(value);
        return;
    }
    const Settings d;
    if (combination_ == 4) {
        if (value == 1) {
            if (scope_ < 0) {
                s.combinations = d.combinations;
                s.degreeCombinations = d.degreeCombinations;
                s.combinationMs = d.combinationMs;
                s.chordOctave = d.chordOctave;
                s.degreeChordOctave = d.degreeChordOctave;
                s.tonic = d.tonic;
                s.chordScale = d.chordScale;
                s.degreeChordRoot = d.degreeChordRoot;
                s.hold = d.hold;
            } else {
                s.degreeCombinations[scope_] = d.degreeCombinations[scope_];
                s.degreeChordOctave[scope_] = d.degreeChordOctave[scope_];
                s.degreeChordRoot[scope_] = d.degreeChordRoot[scope_];
            }
        }
    } else if (combination_ == 6) {
        if (scope_ < 0) s.chordOctave = value;
        else s.degreeChordOctave[scope_] = value;
    } else if (combination_ == 7) s.tonic = value;
    else if (combination_ == 8) s.degreeChordRoot[scope_] = value;
    else if (combination_ == chordHoldField) s.hold = value;
    else if (combination_ == chordScaleField) s.chordScale = value;
    else if (combination_ == 5)
        s.combinationMs = value;
    else if (scope_ < 0)
        s.combinations[combination_] = value;
    else
        s.degreeCombinations[scope_][combination_] = value;
    instrument_.configure();
}
bool SettingsEditor::canRestoreField() const {
    if (confirmation() != Confirmation::None) return false;
    if (page_ == Page::VolumeEffects) return selection_ != systemVolumeField;
    if (page_ != Page::Setup) return true;
    const auto destination = setupDestination();
    return tab_ != Tab::About && selection_ != 2 &&
           (destination == SetupDestination::Picker || destination == SetupDestination::Tempo);
}
void SettingsEditor::restoreField() {
    if (!canRestoreField())
        return;
    if (!picker_ && mixField(selection_)) {
        restoreLayer();
        status_ = "Restored to default";
        return;
    }
    if (!picker_ && envelopeRow(selection_)) {
        auto &s = instrument_.settings;
        const int part = envelopePartFor(selection_);
        s.envelopes[part] = instrument_.library.current(s).sound.envelopes[part];
        instrument_.configure();
        status_ = "Restored to default";
        return;
    }
    int v = defaultValue();
    if (picker_) {
        if (soundPicker()) {
            auto *sound = soundLibrary().choice(v);
            soundCategory_ = sound ? sound->category : "Saved";
            buildChoices();
        }
        for (int n = 0; n < int(choices_.size()); n++)
            if (choices_[n].value == v) {
                choice_ = n;
                preview();
                break;
            }
    } else {
        if (soundField(selection_)) {
            auto &s = instrument_.settings;
            const auto &d = instrument_.library.current(s).sound;
            if (selection_ == 13) {
                s.chordProgram = d.chordProgram;
                s.chordSound = d.chordSound;
            }
            if (selection_ == 14) {
                s.harpProgram = d.harpProgram;
                s.frontSound = d.frontSound;
            }
            if (selection_ == 21) {
                s.padProgram = d.padProgram;
                s.padSound = d.padSound;
            }
            if (selection_ == arpSoundField) {
                s.arpProgram = d.arpProgram;
                s.arpSound = d.arpSound;
            }
            if (selection_ == bassSoundField) {
                s.bassProgram = d.bassProgram;
                s.bassSound = d.bassSound;
            }
            instrument_.configure();
        } else if (selection_ == drumKitField) {
            auto &s = instrument_.settings;
            const auto &d = instrument_.library.current(s).sound;
            s.drumKit = d.drumKit;
            s.drumSound = d.drumSound;
            instrument_.configure();
        } else {
            if (selection_ == 8) restoreSwing(); // Tempo restores its swing too.
            applyChoice(v);
        }
        status_ = "Restored to default";
    }
}
void SettingsEditor::cancelPending() {
    tempo_ = false;
    tempoError_.clear();
    heldKey_ = 0;
    deferred_ = nullptr;
    if (picker_) {
        picker_ = false;
        instrument_.configure();
    }
}
bool SettingsEditor::repeatable(char key) const {
    if (tempo_) return tempoRow_ == 2 && (key == 19 || key == 20 || key == 'z' || key == 'c');
    if (!settingsOpen) return false;
    return key == 17 || key == 18 || key == 'f' || key == 'x' || key == 19 || key == 20 ||
           key == 'z' || key == 'c';
}
int SettingsEditor::step(int index, int delta, int count) const {
    if (repeating_) return std::clamp(index + delta, 0, count - 1); // Stop at the ends.
    return (index + delta + count) % count;
}
void SettingsEditor::tick(uint64_t now) {
    now_ = now;
    if (!heldKey_ || now < nextRepeat_) return;
    repeating_ = true;
    key(heldKey_, true);
    repeating_ = false;
    fastRepeat_ = now - heldSince_ >= fastRepeatAfterMs;
    nextRepeat_ = now + (fastRepeat_ ? fastRepeatMs : repeatMs);
}
void SettingsEditor::key(char key, bool down) {
    // A preview belongs to the option that started it: leaving the row, tab, list or page
    // ends it, so it never carries into another option or back to playing.
    const auto where = std::make_tuple(settingsOpen, tempo_, picker_, int(page_), int(tab_), backingTab_,
                                       selection_, scope_, combination_, frontKey_, frontField_,
                                       envelopeField_, soundCategory_);
    const unsigned previews = previews_;
    handleKey(key, down);
    if (previews_ == previews &&
        where != std::make_tuple(settingsOpen, tempo_, picker_, int(page_), int(tab_), backingTab_,
                                 selection_, scope_, combination_, frontKey_, frontField_,
                                 envelopeField_, soundCategory_))
        instrument_.stopAudition();
}
void SettingsEditor::handleKey(char key, bool down) {
    if (!down) {
        if (key == heldKey_) {
            heldKey_ = 0;
            if (deferred_) {
                auto action = std::move(deferred_);
                deferred_ = nullptr;
                action();
            }
        }
        return;
    }
    if (!repeating_) {
        heldKey_ = repeatable(key) ? key : 0;
        heldSince_ = now_;
        nextRepeat_ = now_ + repeatDelayMs;
    }
    if (key == '\t') {
        if (tempo_) return;
        if (!settingsOpen) {
            settingsOpen = true;
            page_ = Page::Setup;
            instrument_.panic();
        } else if (soundPicker()) {
            auto categories = soundCategories();
            auto it = std::find(categories.begin(), categories.end(), soundCategory_);
            soundCategory_ = categories[(int(it - categories.begin()) + 1) % int(categories.size())];
            buildChoices();
            choice_ = 0;
            preview();
        } else if (!picker_) {
            status_.clear();
            nextTab();
        }
        return;
    }
    if (tempo_) {
        // Rows: Tempo (typed, applied with Enter), Swing (Off / 8ths / 16ths) and Amount.
        // Swing changes apply at once; Esc keeps them and drops an unapplied tempo.
        tempoError_.clear();
        if (key == 27) {
            tempo_ = false;
        } else if (key >= '0' && key <= '9') {
            tempoRow_ = 0;
            if (tempoReplace_) tempoInput_.clear();
            tempoReplace_ = false;
            if (tempoInput_.size() < 3) tempoInput_ += key;
        } else if (key == '\b') {
            tempoRow_ = 0;
            if (tempoReplace_) tempoInput_.clear();
            else if (!tempoInput_.empty()) tempoInput_.pop_back();
            tempoReplace_ = false;
        } else if (key == 17 || key == 'f' || key == 18 || key == 'x') {
            tempoRow_ = std::clamp(tempoRow_ + (key == 18 || key == 'x' ? 1 : -1), 0, 2);
        } else if (key == 19 || key == 'z' || key == 20 || key == 'c') {
            adjustSwing(key == 19 || key == 'z' ? -1 : 1);
        } else if (key == 'r') {
            const auto &preset = instrument_.library.current(instrument_.settings).sound;
            tempoInput_ = std::to_string(preset.bpm);
            tempoReplace_ = true;
            restoreSwing();
        } else if (key == '\n' && tempoRow_ == 1) {
            adjustSwing(1);
        } else if (key == '\n') {
            int bpm = tempoInput_.empty() ? 0 : std::stoi(tempoInput_);
            if (bpm < 40 || bpm > 240) tempoError_ = "Tempo must be 40-240 BPM";
            else {
                instrument_.settings.bpm = bpm; // Keep the accompaniment clock running.
                tempo_ = false;
            }
        } else if (key == ' ') instrument_.panic();
        return;
    }
    if (!settingsOpen) {
        if (key == 'p') openTempo();
        if (key == 'o') openBacking(true);
        return;
    }
    status_.clear();
    if (key == ' ') {
        instrument_.panic();
        return;
    }
    if (key == 27) {
        heldKey_ = 0;
        back();
        return;
    }
    // Cardputer's printed arrows arrive as F/X/Z/C without Fn. Treat them
    // identically to dedicated arrow events on every page and in every picker.
    const int direction = key == 17 || key == 'f' ? -1 : key == 18 || key == 'x' ? 1 : 0;
    const int adjustment = key == 19 || key == 'z' ? -1 : key == 20 || key == 'c' ? 1 : 0;
    if (picker_) {
        if (soundPicker() && key >= '1' && key <= '8') {
            auto categories = soundCategories();
            if (key - '1' < int(categories.size())) {
                soundCategory_ = categories[key - '1'];
                buildChoices();
                choice_ = 0;
                preview();
            }
            return;
        }
        if (key == '\b') {
            restoreField();
            return;
        }
        // A confirmation is a single question: Enter confirms, Esc keeps.
        const bool confirming = confirmation() != Confirmation::None;
        if (!confirming && (direction || adjustment) && choices_.size() > 1) {
            int next = step(choice_, direction ? direction : adjustment, int(choices_.size()));
            if (next != choice_) {
                choice_ = next;
                preview();
            }
        }
        if (key == '\n') {
            try {
                const int value = choices_[choice_].value;
                applyChoice(value);
                if (page_ != Page::Setup && confirmation() == Confirmation::Restore && value == 1)
                    status_ = "Defaults restored";
            } catch (const std::exception &e) {
                status_ = e.what();
                instrument_.configure();
            }
            picker_ = false;
            deferred_ = nullptr;
        }
        return;
    }
    if (key >= '1' && key <= '8') {
        jumpTab(key - '1');
        return;
    }
    // About is informational: no hidden row, picker or restore action.
    if (page_ == Page::Setup && tab_ == Tab::About) return;
    if (key == '\b') {
        restoreField();
        return;
    }
    if (adjustment) {
        adjust(adjustment);
        return;
    }
    if (direction) {
        if (page_ == Page::Envelope) {
            envelopeField_ = step(envelopeField_, direction, 5);
        } else if (page_ == Page::FrontNotes) {
            auto list = frontRows();
            auto row = std::find(list.begin(), list.end(), frontField_) - list.begin();
            frontField_ = list[step(int(row), direction, int(list.size()))];
        } else if (page_ == Page::Combinations) {
            auto list = combinationRows();
            auto row = std::find(list.begin(), list.end(), combination_) - list.begin();
            combination_ = list[step(int(row), direction, int(list.size()))];
        } else {
            const auto &fields = rows();
            int next = step(rowIndex(), direction, int(fields.size()));
            if (page_ == Page::VolumeEffects) volumeEffectsRow_ = next;
            else if (page_ == Page::Setup) tabRows_[int(tab_)] = next;
            selection_ = fields[next];
        }
        // Keep the chord from O ringing while choosing layers.
        if (!backingOpen()) instrument_.panic();
        return;
    }
    if (key == '\n') {
        if (page_ == Page::Setup || page_ == Page::Backing || page_ == Page::FrontSound) openSetting();
        else if (page_ != Page::VolumeEffects || selection_ == outputField) openPicker();
    }
}
} // namespace zc
