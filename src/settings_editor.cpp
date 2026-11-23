#include "settings_editor.hpp"
#include "patterns.hpp"
#include "sounds.hpp"
#include <algorithm>
namespace zc {
namespace {
const std::vector<int> volumeEffectsRows{17, 4, 5, 6, 26, 27, 28};
}
const char *SettingsEditor::envelopePartName(int part) {
    static const char *names[]{"Front", "Chord/Arp", "Pad"};
    return names[part];
}
const char *SettingsEditor::envelopeControlName(int field) {
    static const char *names[]{"Attack", "Decay", "Sustain", "Release", "Restore envelope..."};
    return names[field];
}
std::string SettingsEditor::envelopeValueName(int field, int value) {
    return value < 0 ? "Use sound's envelope" : std::to_string(value) + (field == 2 ? "%" : " ms");
}
int SettingsEditor::chordOctaveValue(const Settings &s) const {
    return scope_ < 0 ? s.chordOctave : s.degreeChordOctave[scope_];
}
int SettingsEditor::chordConfigValue(const Settings &s) const {
    if (combination_ == 6) return chordOctaveValue(s);
    if (combination_ == 7) return s.tonic;
    if (combination_ == 8) return s.degreeChordRoot[scope_];
    if (combination_ == 5) return s.combinationMs;
    if (combination_ == 4) return 0;
    return scope_ < 0 ? s.combinations[combination_] : s.degreeCombinations[scope_][combination_];
}
int SettingsEditor::envelopeValue(const Settings &s) const {
    return envelopeField_ == 4 ? 0 : s.envelopes[envelopePart_].values[envelopeField_];
}
bool SettingsEditor::appField(int index) {
    return index == 0 || index == 1 || (index >= 3 && index <= 7) || index == 15 || index == 17 ||
           index == 18 || index == 22 || (index >= 26 && index <= volumeEffectsField);
}
const std::vector<int> &SettingsEditor::rows() const {
    static const std::vector<int> style{2, 8, 10, 11, 9, 12, 14, 23, 13, 24, 21, 25, 19, 20};
    static const std::vector<int> app{18, volumeEffectsField, 15, 22, 3};
    static const std::vector<int> about;
    if (page_ == Page::VolumeEffects) return volumeEffectsRows;
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
                                  "Chord/Arp sound",
                                  "Front sound",
                                  "Hold",
                                  "" /* Retired field 16; preserve internal field IDs. */,
                                  "Master volume",
                                  "Theme",
                                  "Save as new preset",
                                  "Restore preset...",
                                  "Pad sound",
                                  "Front notes", "Front envelope", "Chord/Arp envelope", "Pad envelope",
                                  "EQ Low", "EQ Mid", "EQ High", "Volume & effects"};
    return names[index];
}
std::string SettingsEditor::backingLabel(PlaybackMode mode) {
    // Preset files keep their existing mode names. Display layers in menu order.
    std::string label;
    for (const auto &row : backingLayerRows) {
        if (!layerEnabled(mode, row.mode)) continue;
        if (!label.empty()) label += " + ";
        label += modeName(row.mode);
    }
    if (label.empty()) return modeName(PlaybackMode::FrontOnly);
    return label;
}
const char *SettingsEditor::combinationName(int index) {
    static const char *names[] = {"Top + middle", "Top + bottom", "Middle + bottom", "All three",
                                  "Restore defaults...", "Chord combo timing", "Octave", "Key", "Root"};
    return names[index];
}
std::vector<int> SettingsEditor::combinationRows() const {
    return scope_ < 0 ? std::vector<int>{6, 7, 0, 1, 2, 3, 5, 4}
                      : std::vector<int>{6, 8, 0, 1, 2, 3, 4};
}
std::string SettingsEditor::scopeName() const {
    return scope_ < 0 ? "All degrees"
                      : std::string(degreeName(scope_)) + " (" +
                            noteName(instrument_.settings.chordRoot(scope_)) +
                            ")";
}
bool SettingsEditor::soundPicker() const {
    return picker_ && page_ == Page::Setup &&
           (selection_ == 13 || selection_ == 14 || selection_ == 21);
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
    static const char *all[]{"Front mode", "K/L action", "Strum time", "Restore all front notes...", "Scale"};
    static const char *key[]{"Note", "Octave", "Restore this key..."};
    return frontKey_ < 0 ? all[field] : key[field];
}
std::vector<int> SettingsEditor::frontRows() const {
    if (frontKey_ >= 0)
        return {0, 1, 2};
    const auto &s = instrument_.settings;
    std::vector<int> rows{0};
    if (s.frontMode) rows.push_back(4);
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
        const int values[]{s.frontMode, s.frontAction[s.frontMode],
                           s.frontMode ? s.scaleStrumMs : s.strumMs, 0, s.scale};
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
        if (field == 4) s.scale = value;
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
        return "Reset >";
    if (frontKey_ < 0) {
        if (field == 0)
            return value ? "Scale notes" : "Chord notes";
        if (field == 1)
            return value ? "Octave shift" : "Strum";
        if (field == 4) return value ? "Natural minor" : "Major";
        return std::to_string(value) + " ms";
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
        int interval = (pitch - s.tonic + 12) % 12;
        return std::string(degrees[value]) + " (+" + std::to_string(interval) + ")";
    }
    if (value < 0) return "Auto note";
    if (value < 4) return "Chord tone " + std::to_string(value + 1);
    return "Fixed +" + std::to_string(value - 16) + " semitones";
}
int SettingsEditor::valueFor(const Settings &s, int index) const {
    if (index >= 26 && index <= 28) return s.eqDb[index - 26];
    if (index == 13 || index == 14 || index == 21) {
        int program = index == 14 ? s.harpProgram : (index == 21 ? s.padProgram : s.chordProgram);
        if (program < 0)
            program = index == 14 ? presets[std::clamp(s.preset, 0, 3)].harpProgram
                                  : presets[std::clamp(s.preset, 0, 3)].chordProgram;
        return soundLibrary().selected(
            program, index == 14 ? s.frontSound : (index == 21 ? s.padSound : s.chordSound));
    }
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
                          s.hold,
                          0,
                          s.masterVolume,
                          s.theme,
                          0,
                          0,
                          s.padProgram,
                          0};
    return values[index];
}
int SettingsEditor::settingValueNumber() const {
    return valueFor(instrument_.settings, selection_);
}
int SettingsEditor::fieldValue() const {
    const auto &s = instrument_.settings;
    switch (page_) {
    case Page::Setup:
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
    const auto d = page_ == Page::Combinations || appField(selection_)
                       ? Settings{}
                       : instrument_.library.current(instrument_.settings).sound;
    if (page_ == Page::Combinations) {
        return chordConfigValue(d);
    }
    if (appField(selection_))
        return valueFor(Settings{}, selection_);
    // Pattern default depends on the mode currently being edited.
    auto copy = d;
    return valueFor(copy, selection_);
}
bool SettingsEditor::fieldModified(int index) const {
    if (index == volumeEffectsField)
        return std::any_of(volumeEffectsRows.begin(), volumeEffectsRows.end(),
                           [&](int field) { return fieldModified(field); });
    if (index >= 23 && index <= 25)
        return instrument_.settings.envelopes[index - 23] !=
               instrument_.library.current(instrument_.settings).sound.envelopes[index - 23];
    if (index == 2)
        return instrument_.library.modified(instrument_.settings);
    if (index == 22) {
        const Settings d;
        const auto &s = instrument_.settings;
        return s.frontSource != d.frontSource || s.frontOffset != d.frontOffset ||
               s.frontOctave != d.frontOctave || s.frontMode != d.frontMode ||
               s.frontDegree != d.frontDegree || s.scaleOctave != d.scaleOctave ||
               s.frontAction != d.frontAction || s.strumMs != d.strumMs ||
               s.scaleStrumMs != d.scaleStrumMs || s.scale != d.scale;
    }
    if (index == 3) {
        const Settings d;
        return d.combinations != instrument_.settings.combinations ||
               d.combinationMs != instrument_.settings.combinationMs ||
               d.chordOctave != instrument_.settings.chordOctave ||
               d.tonic != instrument_.settings.tonic ||
               d.degreeChordRoot != instrument_.settings.degreeChordRoot ||
               d.degreeChordOctave != instrument_.settings.degreeChordOctave ||
               d.degreeCombinations != instrument_.settings.degreeCombinations;
    }
    if (index >= 19 && index <= 20)
        return false;
    auto d = instrument_.library.current(instrument_.settings).sound;
    if (index == 13 || index == 14 || index == 21) {
        const auto &s = instrument_.settings;
        auto id = [&](const Settings &v) -> const std::string & {
            return index == 14 ? v.frontSound : (index == 21 ? v.padSound : v.chordSound);
        };
        return valueFor(s, index) != valueFor(d, index) || id(s) != id(d);
    }
    if (appField(index))
        d = Settings{};
    return valueFor(instrument_.settings, index) != valueFor(d, index);
}
std::string SettingsEditor::settingValue(int index) const {
    const auto &s = instrument_.settings;
    if (index >= 26 && index <= 28) {
        int gain = s.eqDb[index - 26];
        return (gain > 0 ? "+" : "") + std::to_string(gain) + " dB";
    }
    switch (index) {
    case 0:
        return noteName(s.tonic);
    case 1:
        return s.scale ? "Natural minor" : "Major";
    case 2:
        return instrument_.library.current(s).name;
    case 3:
    case 22:
    case 23:
    case 24:
    case 25:
        return "Edit >";
    case volumeEffectsField:
        return "Open >";
    case 7:
        return std::to_string(s.strumMs) + " ms";
    case 8:
        return std::to_string(s.bpm) + " BPM";
    case 9:
        return patterns(PlaybackMode::Arp)[s.pattern].name;
    case 10:
        return backingLabel(s.mode);
    case 21:
    case 13:
    case 14: {
        auto value = valueFor(s, index);
        return soundLibrary().name(value < 128 ? value
                                               : (index == 14   ? s.harpProgram
                                                  : index == 21 ? s.padProgram
                                                                : s.chordProgram),
                                   index == 14   ? s.frontSound
                                   : index == 21 ? s.padSound
                                                 : s.chordSound);
    }
    case 15:
        return s.hold ? "On" : "Off";
    case 18:
        return s.theme ? "Light" : "Dark";
    case 19:
        return "Create >";
    case 20:
        return "Restore >";
    case 11:
        return patterns(PlaybackMode::Chord)[s.chordPattern].name;
    case 12:
        return patterns(PlaybackMode::Pad)[s.padMotion].name;
    default:
        return std::to_string(valueFor(s, index)) + "%";
    }
}
void SettingsEditor::assign(Settings &s, int index, int value) {
    if (index >= 26 && index <= 28) {
        s.eqDb[index - 26] = value;
        return;
    }
    if (index == 13 || index == 14 || index == 21) {
        auto &id = index == 14 ? s.frontSound : (index == 21 ? s.padSound : s.chordSound);
        auto &program = index == 14 ? s.harpProgram : (index == 21 ? s.padProgram : s.chordProgram);
        if (auto *sound = soundLibrary().choice(value); sound && !sound->bank.empty())
            id = sound->id;
        else {
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
    case 21:
        s.padProgram = value;
        break;
    case 13:
        s.chordProgram = value;
        break;
    case 14:
        s.harpProgram = value;
        break;
    case 15:
        s.hold = value;
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
    }
}
void SettingsEditor::setSettingValue(int value) {
    auto &s = instrument_.settings;
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
    if (page_ == Page::Envelope) {
        if (envelopeField_ == 4) {
            choices_ = {{0, "Keep envelope", "Keep this part's envelope"},
                        {1, "Restore envelope", "Reset only this part's envelope to preset defaults"}};
            return;
        }
        static const char *descriptions[]{"Time to reach full volume", "Time to reach sustain level",
                                           "Level while the note remains active", "Fade after the note ends"};
        auto add = [&](int value) {
            if (std::none_of(choices_.begin(), choices_.end(),
                             [&](const Choice &c) { return c.value == value; }))
                choices_.push_back({value, envelopeValueName(envelopeField_, value),
                                     value < 0 ? "Keep the sound's original behavior" : descriptions[envelopeField_]});
        };
        add(-1);
        if (envelopeField_ == 2)
            for (int value = 0; value <= 100; value += 5) add(value);
        else
            for (int value : {0, 5, 10, 20, 50, 100, 200, 300, 500, 750, 1000, 1500, 2000, 3000, 5000})
                add(value);
        add(envelopeValue(s)); add(defaultValue());
        std::sort(choices_.begin(), choices_.end(), [](const Choice &a, const Choice &b) { return a.value < b.value; });
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
                            {1, "Scale notes", "I-VII in the selected key; independent of chords"}};
            else if (frontField_ == 4)
                choices_ = {{0, "Major", "Scale notes only; chord roots stay independent"},
                            {1, "Natural minor", "Scale notes only; chord roots stay independent"}};
            else if (frontField_ == 1)
                choices_ = {{0, "Strum", "K: M to Z / L: Z to M"},
                            {1, "Octave shift", "K: down / L: up; front notes only"}};
            else {
                for (int time = 60; time <= 600; time += 30)
                    choices_.push_back(
                        {time, std::to_string(time) + " ms", "Time from first to last front note"});
                int current = frontValue(s, frontField_);
                if (std::none_of(choices_.begin(), choices_.end(),
                                 [&](const Choice &c) { return c.value == current; }))
                    choices_.push_back(
                        {current, std::to_string(current) + " ms", "Current strum time"});
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
                    {degree, frontValueName(copy, 0), "Degree of the selected key and scale"});
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
                choices_.push_back({-1, "Follow Key (" + noteName(rootFor(s.tonic, 0, scope_)) + ")", "Default degree in the selected key"});
            for (int pitch = 0; pitch < 12; pitch++)
                choices_.push_back({pitch, noteName(pitch), combination_ == 7 ? "Transpose degrees that follow Key" : "Absolute root; stays fixed when Key changes"});
        } else if (combination_ == 6) {
            if (scope_ >= 0)
                choices_.push_back({-2, "Follow All (" + std::to_string(s.chordOctave) + ")", "Use the shared chord root octave"});
            for (int octave = -1; octave <= 7; octave++)
                choices_.push_back({octave, "Octave " + std::to_string(octave), "Root note octave; chord intervals stay together"});
        } else if (combination_ == 4)
            choices_ = {{0, "Keep settings", "Keep the current chord settings"},
                        {1, "Restore chord config",
                         scope_ < 0 ? "Reset key, roots, octaves, chord mappings and timing"
                                    : "Follow Key, shared octave and chord mappings"}};
        else if (combination_ == 5) {
            for (int time = 0; time <= 60; time += 5)
                choices_.push_back({time, time ? std::to_string(time) + " ms" : "Off",
                                    time ? "More time to press together, with later backing"
                                         : "Play each chord button immediately"});
            if (s.combinationMs % 5) {
                choices_.push_back({s.combinationMs, std::to_string(s.combinationMs) + " ms",
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
    if (selection_ == 0)
        for (int n = 0; n < 12; n++)
            choices_.push_back({n, noteName(n), "Tonic for all scale degrees"});
    else if (selection_ == 1)
        choices_ = {{0, "Major", "Chord rows stay explicit"},
                    {1, "Natural minor", "Chord rows stay explicit"}};
    else if (selection_ == 2)
        for (int n : instrument_.library.order()) {
            const auto &p = instrument_.library.entries[n];
            auto value = instrument_.library.resolved(n);
            choices_.push_back(
                {n, p.name,
                 backingLabel(value.mode) + " / " + std::to_string(value.bpm) + " BPM"});
        }
    else if (selection_ == 9 || selection_ == 11 || selection_ == 12) {
        auto mode = selection_ == 11   ? PlaybackMode::Chord
                    : selection_ == 12 ? PlaybackMode::Pad
                                       : PlaybackMode::Arp;
        const auto &list = patterns(mode);
        const int presetDefault = mode == PlaybackMode::Arp ? defaultValue() : -1;
        auto add = [&](int n) { choices_.push_back({n, list[n].name, list[n].description}); };
        for (int n = 0; n < int(list.size()); n++) {
            if (mode == PlaybackMode::Arp && n >= 6 && n != s.pattern && n != presetDefault) continue;
            if (mode == PlaybackMode::Chord && n == sustainedChordPattern) continue;
            add(n);
            if (mode == PlaybackMode::Chord && n == 0) add(sustainedChordPattern);
        }
    } else if (selection_ == 13 || selection_ == 14 || selection_ == 21) {
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
    } else if (selection_ == 15)
        choices_ = {{0, "Off", "Clear the chord when you release its buttons"},
                    {1, "On", "Keep the chord after you release its buttons"}};
    else if (selection_ == 18)
        choices_ = {{0, "Dark", "App-wide appearance"}, {1, "Light", "App-wide appearance"}};
    else if (selection_ == 19 || selection_ == 20)
        choices_ = {{0, "Keep preset", "Keep the current preset as it is"},
                    {1, selection_ == 19 ? "Create preset copy" : "Restore preset",
                     selection_ == 19 ? "Make a new preset from these Style settings"
                                      : "Reset only the current preset's Style settings"}};
    else {
        int low = 0, high = selection_ == 6 ? 60 : 100, step = 5;
        std::string unit = "%";
        if (selection_ >= 26 && selection_ <= 28) {
            static const char *descriptions[]{"Bass shelf at 250 Hz", "Broad mid band at 1 kHz",
                                               "Treble shelf at 4 kHz"};
            for (int gain = -12; gain <= 12; gain++)
                choices_.push_back({gain, (gain > 0 ? "+" : "") + std::to_string(gain) + " dB",
                                     descriptions[selection_ - 26]});
            return;
        }
        if (selection_ == 7) {
            low = 60;
            high = 600;
            step = 30;
            unit = " ms";
        }
        std::string description;
        switch (selection_) {
        case 4: description = "Volume of " + backingLabel(PlaybackMode::All); break;
        case 5: description = "Volume of the notes played with Z-M and K/L"; break;
        case 6: description = "Level of the repeat you hear after each note"; break;
        case 17: description = "Overall output level"; break;
        }
        for (int n = low; n <= high; n += step)
            choices_.push_back({n, std::to_string(n) + unit, description});
        for (int v : {settingValueNumber(), defaultValue()})
            if (std::none_of(choices_.begin(), choices_.end(),
                             [&](const Choice &c) { return c.value == v; }))
                choices_.push_back({v, std::to_string(v) + unit, description});
        std::sort(choices_.begin(), choices_.end(),
                  [](const Choice &a, const Choice &b) { return a.value < b.value; });
    }
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
    const bool setting = settingListPage();
    if (confirmation() != Confirmation::None || (page_ == Page::Combinations && combination_ == 5) ||
        (setting && (selection_ == 17 || selection_ == 18)))
        return;
    Settings sound = instrument_.settings;
    if (page_ == Page::Envelope) {
        if (envelopeField_ == 4) return;
        sound.envelopes[envelopePart_].values[envelopeField_] = choices_[choice_].value;
        if (envelopePart_ == 0) instrument_.auditionFront(sound, 0);
        else {
            sound.mode = envelopePart_ == 2 ? PlaybackMode::Pad : PlaybackMode::Chord;
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
        if (selection_ >= 26 && selection_ <= 28) {
            sound.mode = PlaybackMode::Chord;
            sound.chordPattern = sustainedChordPattern;
        }
        if (selection_ == 4 && sound.mode == PlaybackMode::FrontOnly)
            sound.mode = PlaybackMode::Chord;
        if (selection_ == 21)
            sound.mode = PlaybackMode::Pad;
        if (selection_ == 13)
            sound.mode = PlaybackMode::Chord;
        if (selection_ == 9)
            sound.mode = PlaybackMode::Arp;
        if (selection_ == 11)
            sound.mode = PlaybackMode::Chord;
        if (selection_ == 12)
            sound.mode = PlaybackMode::Pad;
    }
    if (setting && (selection_ == 14 || selection_ == 5)) {
        instrument_.auditionFront(sound, 0);
        return;
    }
    if (page_ == Page::Combinations) {
        int value = choices_[choice_].value;
        if (combination_ == 6) {
            if (scope_ < 0) sound.chordOctave = value;
            else sound.degreeChordOctave[scope_] = value;
        } else if (combination_ == 7) sound.tonic = value;
        else if (combination_ == 8) sound.degreeChordRoot[scope_] = value;
    }
    auto c = previewChord();
    c.root = sound.chordRoot(c.column);
    instrument_.audition(c, sound, setting);
}
void SettingsEditor::openBacking() {
    backing_ = true;
    pendingLayers_ = backingLayers(instrument_.settings.mode);
    backingRow_ = 0;
}
void SettingsEditor::openTempo() {
    tempo_ = true;
    tempoReplace_ = true;
    tempoInput_ = std::to_string(instrument_.settings.bpm);
    tempoError_.clear();
}
SettingsEditor::SetupDestination SettingsEditor::setupDestination() const {
    switch (selection_) {
    case 8: return SetupDestination::Tempo;
    case 10: return SetupDestination::Backing;
    case 23:
    case 24:
    case 25: return SetupDestination::Envelope;
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
    case Page::VolumeEffects:
        return Confirmation::None;
    }
    return Confirmation::None;
}
void SettingsEditor::openSetting() {
    switch (setupDestination()) {
    case SetupDestination::Picker:
        openPicker();
        return;
    case SetupDestination::Backing:
        openBacking();
        return;
    case SetupDestination::Tempo:
        openTempo();
        return;
    case SetupDestination::Envelope:
        envelopePart_ = selection_ - 23;
        envelopeField_ = 0;
        page_ = Page::Envelope;
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
        break;
    }
    instrument_.panic();
}
void SettingsEditor::returnToSetup() {
    if (page_ == Page::VolumeEffects) selection_ = volumeEffectsField;
    page_ = Page::Setup;
}
void SettingsEditor::openPicker() {
    if (page_ == Page::Setup && (selection_ == 13 || selection_ == 14 || selection_ == 21)) {
        auto *sound = soundLibrary().choice(settingValueNumber());
        soundCategory_ = sound ? sound->category : "Saved";
    }
    buildChoices();
    choice_ = 0;
    if (confirmation() == Confirmation::None) {
        const int value = fieldValue();
        for (int n = 0; n < int(choices_.size()); n++)
            if (choices_[n].value == value)
                choice_ = n;
    }
    picker_ = true;
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
                s.degreeChordRoot = d.degreeChordRoot;
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
    if (page_ != Page::Setup) return true;
    const auto destination = setupDestination();
    return tab_ != Tab::About && selection_ != 2 &&
           (destination == SetupDestination::Picker || destination == SetupDestination::Tempo ||
            destination == SetupDestination::Backing);
}
void SettingsEditor::restoreField() {
    if (!canRestoreField())
        return;
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
        if (page_ == Page::Setup && (selection_ == 13 || selection_ == 14 || selection_ == 21)) {
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
            instrument_.configure();
        } else
            applyChoice(v);
        status_ = "Restored to default";
    }
}
void SettingsEditor::cancelPending() {
    tempo_ = false;
    backing_ = false;
    tempoError_.clear();
    if (picker_) {
        picker_ = false;
        instrument_.configure();
    }
}
void SettingsEditor::key(char key, bool down) {
    if (!down)
        return;
    if (key == '\t') {
        cancelPending();
        settingsOpen = !settingsOpen;
        returnToSetup();
        instrument_.panic();
        return;
    }
    if (backing_) {
        int direction = key == 17 || key == 'f' ? -1 : key == 18 || key == 'x' ? 1 : 0;
        const int rows = backingApplyRow + 1;
        if (direction) backingRow_ = (backingRow_ + direction + rows) % rows;
        if (key == 27) backing_ = false;
        else if (key == '\b' || key == 'r')
            pendingLayers_ = backingLayers(instrument_.library.current(instrument_.settings).sound.mode);
        else if (key == '\n') {
            if (backingRow_ != backingApplyRow)
                pendingLayers_ ^= backingLayers(backingLayerRows[backingRow_].mode);
            else {
                instrument_.setBackingMode(modeForLayers(pendingLayers_));
                backing_ = false;
            }
        } else if (key == ' ') instrument_.panic();
        return;
    }
    if (tempo_) {
        tempoError_.clear();
        if (key == 27) {
            tempo_ = false;
        } else if (key >= '0' && key <= '9') {
            if (tempoReplace_) tempoInput_.clear();
            tempoReplace_ = false;
            if (tempoInput_.size() < 3) tempoInput_ += key;
        } else if (key == '\b') {
            if (tempoReplace_) tempoInput_.clear();
            else if (!tempoInput_.empty()) tempoInput_.pop_back();
            tempoReplace_ = false;
        } else if (key == 'r') {
            tempoInput_ = std::to_string(instrument_.library.current(instrument_.settings).sound.bpm);
            tempoReplace_ = true;
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
        if (key == 'o') openBacking();
        return;
    }
    status_.clear();
    if (key == ' ') {
        instrument_.panic();
        return;
    }
    if (key == 27) {
        if (picker_)
            cancelPending();
        else if (page_ != Page::Setup)
            returnToSetup();
        else
            settingsOpen = false;
        instrument_.panic();
        return;
    }
    // Cardputer's printed arrows arrive as F/X/Z/C without Fn. Treat them
    // identically to dedicated arrow events on every page and in every picker.
    int direction = key == 17 || key == 'f' ? -1 : key == 18 || key == 'x' ? 1 : 0;
    int adjustment = key == 19 || key == 'z' ? -1 : key == 20 || key == 'c' ? 1 : 0;
    if (page_ == Page::Setup && !picker_ && adjustment) {
        if (tab_ != Tab::About) tabRows_[int(tab_)] = rowIndex();
        tab_ = Tab((int(tab_) + adjustment + 3) % 3);
        selection_ = rows().empty() ? -1 : rows()[tabRows_[int(tab_)]];
        instrument_.panic();
        return;
    }
    // About is informational: no hidden row, picker or restore action.
    if (page_ == Page::Setup && tab_ == Tab::About) return;
    if (key == '\b') {
        restoreField();
        return;
    }
    if (picker_) {
        if (soundPicker() && adjustment) {
            auto categories = soundCategories();
            auto it = std::find(categories.begin(), categories.end(), soundCategory_);
            int n = int(it - categories.begin());
            soundCategory_ =
                categories[(n + adjustment + int(categories.size())) % int(categories.size())];
            buildChoices();
            choice_ = 0;
            preview();
            return;
        }
        if ((direction || adjustment) && choices_.size() > 1) {
            choice_ = (choice_ + (direction ? direction : adjustment) + int(choices_.size())) %
                      int(choices_.size());
            preview();
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
        }
        return;
    }
    if (page_ == Page::Envelope) {
        if (adjustment) {
            envelopePart_ = (envelopePart_ + adjustment + 3) % 3;
            selection_ = 23 + envelopePart_;
            tabRows_[0] = rowIndex();
            instrument_.panic();
        }
        if (direction) {
            envelopeField_ = (envelopeField_ + direction + 5) % 5;
            instrument_.panic();
        }
        if (key == '\n') openPicker();
        return;
    }
    if (page_ == Page::FrontNotes) {
        if (adjustment) {
            frontKey_ = (frontKey_ + 1 + adjustment + 8) % 8 - 1;
            frontField_ = 0;
            instrument_.panic();
        }
        if (direction) {
            auto list = frontRows();
            auto row = std::find(list.begin(), list.end(), frontField_) - list.begin();
            frontField_ = list[(row + direction + int(list.size())) % int(list.size())];
            instrument_.panic();
        }
        if (key == '\n')
            openPicker();
        return;
    }
    if (page_ == Page::Combinations && adjustment) {
        scope_ = (scope_ + 1 + adjustment + 8) % 8 - 1;
        if (scope_ >= 0 && combination_ == 5)
            combination_ = 0;
        if (combination_ == 7 || combination_ == 8) combination_ = scope_ < 0 ? 7 : 8;
        instrument_.panic();
        return;
    }
    if (page_ == Page::Combinations && key >= '1' && key <= '8') {
        scope_ = key == '1' ? -1 : key - '2';
        if (scope_ >= 0 && combination_ == 5)
            combination_ = 0;
        if (combination_ == 7 || combination_ == 8) combination_ = scope_ < 0 ? 7 : 8;
        instrument_.panic();
        return;
    }
    if (direction) {
        if (settingListPage()) {
            const auto &fields = rows();
            int next = (rowIndex() + direction + int(fields.size())) % int(fields.size());
            (page_ == Page::VolumeEffects ? volumeEffectsRow_ : tabRows_[int(tab_)]) = next;
            selection_ = fields[next];
        } else {
            auto list = combinationRows();
            auto row = std::find(list.begin(), list.end(), combination_) - list.begin();
            combination_ = list[(row + direction + int(list.size())) % int(list.size())];
        }
        instrument_.panic();
    }
    if (key == '\n') {
        if (page_ == Page::Setup) openSetting();
        else openPicker();
    }
}
} // namespace zc
