#include "settings_editor.hpp"
#include "patterns.hpp"
#include <algorithm>
namespace zc {
const char *SettingsEditor::settingName(int index) {
    static const char *names[] = {"Key",
                                  "Scale",
                                  "Preset",
                                  "Chord combinations",
                                  "Backing volume",
                                  "Front volume",
                                  "Echo",
                                  "K/L sweep",
                                  "Tempo",
                                  "Arp pattern",
                                  "Mode",
                                  "Chord pattern",
                                  "Pad motion",
                                  "Backing sound",
                                  "Front sound",
                                  "Hold",
                                  "Preset level",
                                  "Master volume",
                                  "Theme",
                                  "Save as new preset",
                                  "Restore preset..."};
    return names[index];
}
const char *SettingsEditor::combinationName(int index) {
    static const char *names[] = {"Top + middle", "Top + bottom", "Middle + bottom", "All three"};
    return names[index];
}
std::string SettingsEditor::scopeName() const {
    return scope_ < 0 ? "All degrees"
                      : std::string(degreeName(scope_)) + " (" +
                            noteName(rootFor(instrument_.settings.tonic, instrument_.settings.scale,
                                             scope_)) +
                            ")";
}
int SettingsEditor::valueFor(const Settings &s, int index) const {
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
                          s.presetLevel,
                          s.masterVolume,
                          s.theme,
                          0,
                          0};
    return values[index];
}
int SettingsEditor::settingValueNumber() const {
    return valueFor(instrument_.settings, selection_);
}
int SettingsEditor::defaultValue() const {
    const auto &d = instrument_.library.current(instrument_.settings).sound;
    if (page_ == Page::Combinations) {
        if (combination_ == 4)
            return 0;
        return scope_ < 0 ? d.combinations[combination_]
                          : d.degreeCombinations[scope_][combination_];
    }
    if (selection_ == 17 || selection_ == 18)
        return valueFor(Settings{}, selection_);
    // Pattern default depends on the mode currently being edited.
    auto copy = d;
    return valueFor(copy, selection_);
}
bool SettingsEditor::fieldModified(int index) const {
    if (index == 2)
        return instrument_.library.modified(instrument_.settings);
    if (index == 3) {
        const auto &d = instrument_.library.current(instrument_.settings).sound;
        return d.combinations != instrument_.settings.combinations ||
               d.degreeCombinations != instrument_.settings.degreeCombinations;
    }
    if (index >= 19 && index <= 20)
        return false;
    auto d = instrument_.library.current(instrument_.settings).sound;
    if (index == 17 || index == 18)
        d = Settings{};
    return valueFor(instrument_.settings, index) != valueFor(d, index);
}
std::string SettingsEditor::settingValue(int index) const {
    const auto &s = instrument_.settings;
    switch (index) {
    case 0:
        return noteName(s.tonic);
    case 1:
        return s.scale ? "Natural minor" : "Major";
    case 2:
        return instrument_.library.current(s).name;
    case 3:
        return "Edit >";
    case 7:
        return std::to_string(s.strumMs) + " ms";
    case 8:
        return std::to_string(s.bpm) + " BPM";
    case 9:
        return patterns(PlaybackMode::Arp)[s.pattern].name;
    case 10:
        return modeName(s.mode);
    case 13:
        return programName(s.chordProgram < 0 ? presets[std::clamp(s.preset, 0, 3)].chordProgram
                                              : s.chordProgram);
    case 14:
        return programName(s.harpProgram < 0 ? presets[std::clamp(s.preset, 0, 3)].harpProgram
                                             : s.harpProgram);
    case 15:
        return s.hold ? "On" : "Off";
    case 18:
        return s.theme ? "Light" : "Dark";
    case 19:
        return "Save >";
    case 20:
        return "Reset >";
    case 11:
        return patterns(PlaybackMode::Chord)[s.chordPattern].name;
    case 12:
        return patterns(PlaybackMode::Pad)[s.padMotion].name;
    default:
        return std::to_string(valueFor(s, index)) + "%";
    }
}
void SettingsEditor::assign(Settings &s, int index, int value) {
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
    case 13:
        s.chordProgram = value;
        break;
    case 14:
        s.harpProgram = value;
        break;
    case 15:
        s.hold = value;
        break;
    case 16:
        s.presetLevel = value;
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
    if (page_ == Page::Combinations) {
        if (combination_ == 4)
            choices_ = {{0, "Cancel", "Keep mappings"},
                        {1, "Restore mappings", "Restore selected scope to preset defaults"}};
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
        for (int n = 0; n < int(instrument_.library.entries.size()); n++) {
            const auto &p = instrument_.library.entries[n];
            auto value = instrument_.library.resolved(n);
            choices_.push_back(
                {n, p.name,
                 std::string(modeName(value.mode)) + " / " + std::to_string(value.bpm) + " BPM"});
        }
    else if (selection_ == 9 || selection_ == 11 || selection_ == 12) {
        auto mode = selection_ == 11   ? PlaybackMode::Chord
                    : selection_ == 12 ? PlaybackMode::Pad
                                       : PlaybackMode::Arp;
        const auto &list = patterns(mode);
        for (int n = 0; n < int(list.size()); n++)
            choices_.push_back({n, list[n].name, list[n].description});
    } else if (selection_ == 10)
        choices_ = {{0, "Pad", "Sustained backing chord"},
                    {1, "Chord", "All notes together; clear attack"},
                    {2, "Arp", "Rhythmic chord-note patterns"}};
    else if (selection_ == 13 || selection_ == 14)
        for (int n = 0; n < 128; n++)
            choices_.push_back(
                {n, programName(n), "General MIDI program " + std::to_string(n + 1)});
    else if (selection_ == 15)
        choices_ = {{0, "Off", "Release stops backing"}, {1, "On", "Keep harmony after release"}};
    else if (selection_ == 18)
        choices_ = {{0, "Dark", "App-wide appearance"}, {1, "Light", "App-wide appearance"}};
    else if (selection_ == 19 || selection_ == 20)
        choices_ = {{0, "Cancel", "Keep current preset"},
                    {1, selection_ == 19 ? "Save new preset" : "Restore entire preset",
                     selection_ == 19 ? "Copy these settings to a new editable preset"
                                      : "Reset every musical setting; keep app volume/theme"}};
    else {
        int low = 0, high = selection_ == 6 ? 60 : 100, step = 5;
        std::string unit = "%";
        if (selection_ == 7) {
            low = 60;
            high = 600;
            step = 30;
            unit = " ms";
        }
        if (selection_ == 8) {
            low = 40;
            high = 240;
            unit = " BPM";
        }
        for (int n = low; n <= high; n += step)
            choices_.push_back({n, std::to_string(n) + unit, "Left / right also adjusts directly"});
        for (int v : {settingValueNumber(), defaultValue()})
            if (std::none_of(choices_.begin(), choices_.end(),
                             [&](const Choice &c) { return c.value == v; }))
                choices_.push_back({v, std::to_string(v) + unit, "Preset or current value"});
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
    return {rootFor(s.tonic, s.scale, col), -1, col, q};
}
void SettingsEditor::preview() {
    if (reset_ || (page_ == Page::Setup &&
                   (selection_ == 17 || selection_ == 18 || selection_ == 19 || selection_ == 20)))
        return;
    Settings sound = instrument_.settings;
    if (page_ == Page::Setup) {
        if (selection_ == 2) {
            auto next = instrument_.library.resolved(choices_[choice_].value);
            applyPresetSettings(sound, next);
            sound.preset = choices_[choice_].value;
        } else
            assign(sound, selection_, choices_[choice_].value);
        if (selection_ == 9)
            sound.mode = PlaybackMode::Arp;
        if (selection_ == 11)
            sound.mode = PlaybackMode::Chord;
        if (selection_ == 12)
            sound.mode = PlaybackMode::Pad;
    }
    auto c = previewChord();
    c.root = rootFor(sound.tonic, sound.scale, c.column);
    instrument_.audition(c, sound, page_ == Page::Setup);
}
void SettingsEditor::openPicker() {
    buildChoices();
    reset_ = (page_ == Page::Combinations && combination_ == 4) ||
             (page_ == Page::Setup && (selection_ == 19 || selection_ == 20));
    int v = page_ == Page::Setup
                ? settingValueNumber()
                : (reset_       ? 0
                   : scope_ < 0 ? instrument_.settings.combinations[combination_]
                                : instrument_.settings.degreeCombinations[scope_][combination_]);
    choice_ = 0;
    for (int n = 0; n < int(choices_.size()); n++)
        if (choices_[n].value == v)
            choice_ = n;
    savedChoice_ = choice_;
    picker_ = true;
    preview();
}
void SettingsEditor::applyChoice(int value) {
    auto &s = instrument_.settings;
    if (page_ == Page::Setup) {
        setSettingValue(value);
        return;
    }
    const auto &d = instrument_.library.current(s).sound;
    if (combination_ == 4) {
        if (value == 1) {
            if (scope_ < 0) {
                s.combinations = d.combinations;
                s.degreeCombinations = d.degreeCombinations;
            } else
                s.degreeCombinations[scope_] = d.degreeCombinations[scope_];
        }
    } else if (scope_ < 0)
        s.combinations[combination_] = value;
    else
        s.degreeCombinations[scope_][combination_] = value;
    instrument_.configure();
}
void SettingsEditor::restoreField() {
    if ((page_ == Page::Setup &&
         (selection_ == 2 || selection_ == 3 || selection_ == 19 || selection_ == 20)) ||
        (page_ == Page::Combinations && combination_ == 4))
        return;
    int v = defaultValue();
    if (picker_) {
        for (int n = 0; n < int(choices_.size()); n++)
            if (choices_[n].value == v) {
                choice_ = n;
                preview();
                break;
            }
    } else {
        applyChoice(v);
        status_ = "Restored to default";
    }
}
void SettingsEditor::adjust(int d) {
    if ((page_ == Page::Setup && (selection_ == 3 || selection_ == 19 || selection_ == 20)) ||
        (page_ == Page::Combinations && combination_ == 4))
        return;
    buildChoices();
    int current = page_ == Page::Setup ? settingValueNumber()
                  : scope_ < 0         ? instrument_.settings.combinations[combination_]
                               : instrument_.settings.degreeCombinations[scope_][combination_];
    int index = 0;
    for (int n = 0; n < int(choices_.size()); n++)
        if (choices_[n].value == current)
            index = n;
    bool wrap = page_ == Page::Combinations || selection_ <= 2 || selection_ == 9 ||
                selection_ == 10 || selection_ == 15 || selection_ == 18 ||
                (selection_ == 11 || selection_ == 12);
    choice_ = wrap ? (index + d + int(choices_.size())) % int(choices_.size())
                   : std::clamp(index + d, 0, int(choices_.size()) - 1);
    applyChoice(choices_[choice_].value);
    preview();
}
void SettingsEditor::cancelPending() {
    if (picker_) {
        picker_ = false;
        reset_ = false;
        instrument_.configure();
    }
}
void SettingsEditor::key(char key, bool down) {
    if (!down)
        return;
    if (key == '\t') {
        cancelPending();
        settingsOpen = !settingsOpen;
        page_ = Page::Setup;
        instrument_.panic();
        return;
    }
    if (!settingsOpen)
        return;
    status_.clear();
    if (key == ' ') {
        instrument_.panic();
        return;
    }
    if (key == 27) {
        if (picker_)
            cancelPending();
        else if (page_ == Page::Combinations)
            page_ = Page::Setup;
        else
            settingsOpen = false;
        instrument_.panic();
        return;
    }
    int direction = key == 17 || key == 'f' ? -1 : key == 18 || key == 'x' ? 1 : 0;
    int adjustment = key == 19 || key == 'z' ? -1 : key == 20 || key == 'c' ? 1 : 0;
    if (key == '\b') {
        restoreField();
        return;
    }
    if (picker_) {
        if (direction || adjustment) {
            choice_ = (choice_ + (direction ? direction : adjustment) + int(choices_.size())) %
                      int(choices_.size());
            preview();
        }
        if (key == '\n') {
            try {
                applyChoice(choices_[choice_].value);
            } catch (const std::exception &e) {
                status_ = e.what();
                instrument_.configure();
            }
            picker_ = false;
            reset_ = false;
        }
        return;
    }
    if (page_ == Page::Combinations && key >= '1' && key <= '8') {
        scope_ = key == '1' ? -1 : key - '2';
        instrument_.panic();
        return;
    }
    if (direction) {
        int &selected = page_ == Page::Setup ? selection_ : combination_;
        int count = page_ == Page::Setup ? settingCount : 5;
        selected = (selected + direction + count) % count;
        instrument_.panic();
    }
    if (adjustment)
        adjust(adjustment);
    if (key == '\n') {
        if (page_ == Page::Setup && selection_ == 3) {
            page_ = Page::Combinations;
            instrument_.panic();
        } else
            openPicker();
    }
}
} // namespace zc
