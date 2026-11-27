#pragma once
#include "instrument.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
namespace zc {
class SettingsEditor {
  public:
    enum class Page {
        Setup, Combinations, FrontNotes, Envelope, VolumeEffects, Backing, FrontSound, MidiInput,
        AllFront, Loop, LoopMix
    };
    enum class Tab { Style, App, About };
    // Number rows change with left/right; Opens rows (pages, lists, actions) open with Enter.
    enum class RowKind { Plain, Number, Opens };
    struct Choice {
        int value;
        std::string name, description;
    };
    static constexpr int backingField = 10;
    static constexpr int volumeEffectsField = 29;
    static constexpr int systemVolumeField = 30;
    static constexpr int arpSoundField = 31;
    static constexpr int arpEnvelopeField = 32;
    static constexpr int chordMixField = 33; // Chord, Arp, Pad mix rows: 33-35.
    static constexpr int frontSoundField = 36;
    static constexpr int drumMixField = 37;
    static constexpr int drumKitField = 38;
    static constexpr int drumPatternField = 39;
    static constexpr int bassMixField = 40;
    static constexpr int bassSoundField = 41;
    static constexpr int bassPatternField = 42;
    static constexpr int bassEnvelopeField = 43;
    static constexpr int outputField = 44;
    static constexpr int chordVoicingField = 45;
    static constexpr int padVoicingField = 46;
    static constexpr int frontMixField = 47;
    static constexpr int midiInputField = 48;    // App row that opens the MIDI input page.
    static constexpr int midiDeviceField = 49;   // Connected keyboard; information only.
    static constexpr int midiTargetField = 50;   // Plays: the layer whose sound MIDI uses.
    static constexpr int midiVelocityField = 51; // Velocity curve.
    static constexpr int allFrontField = 52;       // App row that opens the All front page.
    static constexpr int allFrontModeField = 53;   // Scale or Free play.
    static constexpr int allFrontKeyField = 54;    // Front key and scale, shared with Front notes.
    static constexpr int allFrontScaleField = 55;
    static constexpr int allFrontOctaveField = 56; // Scale mode: octave of Z-M.
    static constexpr int allFrontLowestField = 57; // Free play: note of Z.
    static constexpr int loopLayerRows = 6;
    static constexpr int loopField = 58;       // App row that opens the Loop page.
    static constexpr int loopStatusField = 59; // The recorded loop; information only.
    static constexpr int loopStyleField = 60;  // Recorded or Follow current.
    static constexpr int loopBarsField = 61;   // Length limit.
    static constexpr int loopShortField = 62;  // Short chords.
    static constexpr int loopSnapField = 63;   // Snap changes.
    static constexpr int loopMixField = 64;    // Opens Loop mix.
    static constexpr int loopDeleteField = 65;
    static constexpr int loopLayerField = 66; // Loop mix rows 66-71: Front, then backingLayerOrder.
    static constexpr int loopCountField = loopLayerField + loopLayerRows; // Count-in bars.
    static constexpr int settingCount = loopCountField + 1;
    static constexpr int backingTabCount = 6; // Layers, Chord, Arp, Pad, Bass, Drums.
    explicit SettingsEditor(Instrument &instrument,
                            std::function<std::optional<int>()> readSystemVolume = {},
                            std::function<bool(int)> writeSystemVolume = {})
        : instrument_(instrument), readSystemVolume_(std::move(readSystemVolume)),
          writeSystemVolume_(std::move(writeSystemVolume)) {}
    bool settingsOpen = false;
    void key(char key, bool down);
    // Repeats a held arrow key: rows, choices and numeric values.
    void tick(uint64_t now);
    void cancelPending();
    Page page() const { return page_; }
    int scope() const { return scope_; }
    int selected() const {
        return settingListPage() ? selection_
                                 : (page_ == Page::FrontNotes ? frontField_
                                    : page_ == Page::Envelope ? envelopeField_ : combination_);
    }
    bool pickerOpen() const { return picker_; }
    // Backing opened with O from the playing screen; playing keys still release normally.
    bool backingOpen() const { return settingsOpen && page_ == Page::Backing && fromPlay_; }
    int backingTab() const { return backingTab_; }
    bool tempoOpen() const { return tempo_; }
    // "Delete loop?" from a held Enter or the Loop page: Enter deletes, Esc keeps.
    bool loopQuestion() const { return loopQuestion_; }
    void askDeleteLoop();
    int tempoRow() const { return tempoRow_; } // 0 Tempo, 1 Swing, 2 Amount.
    const std::string &tempoText() const { return tempoInput_; }
    const std::string &tempoError() const { return tempoError_; }
    const std::vector<Choice> &choices() const { return choices_; }
    int highlighted() const { return choice_; }
    Tab tab() const { return tab_; }
    int frontKey() const { return frontKey_; }
    int envelopePart() const { return envelopePart_; }
    static bool appField(int index);
    bool soundPicker() const;
    std::vector<std::string> soundCategories() const;
    const std::string &soundCategory() const { return soundCategory_; }
    const std::vector<int> &rows() const;
    int rowIndex() const;
    static const char *settingName(int index);
    static const char *combinationName(int index);
    static int envelopeField(int part);

  protected:
    static constexpr int chordHoldField = 9;
    static constexpr int chordScaleField = 10;
    static constexpr int frontKeyField = 5;
    enum class SetupDestination {
        Picker, Tempo, Backing, Envelope, FrontNotes, Combinations, VolumeEffects, FrontSound, Mix,
        MidiInput, AllFront, Loop
    };
    enum class Confirmation { None, Create, Restore };
    RowKind rowKind(int field) const;
    RowKind selectedKind() const;
    static std::string backingLabel(unsigned layers);
    static std::string backingLabel(const Settings &settings);
    Instrument &instrument_;
    Page page_ = Page::Setup;
    Page envelopeParent_ = Page::Backing;
    Tab tab_ = Tab::Style;
    int tabRows_[3] = {0, 0, 0};
    int volumeEffectsRow_ = 0, midiRow_ = 1;
    std::array<int, backingTabCount> backingRows_{};
    int frontSoundRow_ = 0;
    int backingTab_ = 0;
    bool fromPlay_ = false;
    std::pair<int, int> playReturn_{2, 0}; // Setup row and tab to restore after O.
    int selection_ = 2, combination_ = 6, scope_ = -1;
    int frontKey_ = -1, frontField_ = 0;
    int envelopePart_ = 0, envelopeField_ = 0;
    std::string soundCategory_;
    bool picker_ = false;
    bool tempo_ = false, tempoReplace_ = true;
    int tempoRow_ = 0;
    std::string tempoInput_, tempoError_;
    int choice_ = 0;
    std::vector<Choice> choices_;
    std::string status_;
    std::optional<int> systemVolume_;
    bool loopQuestion_ = false;
    int loopRow_ = 1;
    bool settingListPage() const {
        return page_ == Page::Setup || page_ == Page::VolumeEffects || page_ == Page::Backing ||
               page_ == Page::FrontSound || page_ == Page::MidiInput || page_ == Page::AllFront ||
               page_ == Page::Loop || page_ == Page::LoopMix;
    }
    static bool loopLayerRow(int index) {
        return index >= loopLayerField && index < loopLayerField + loopLayerRows;
    }
    bool hasLoop() const;
    std::string loopSummary() const;
    static bool mixField(int index) {
        return (index >= chordMixField && index < chordMixField + 3) || index == drumMixField ||
               index == bassMixField;
    }
    static bool envelopeRow(int index) {
        return (index >= 23 && index <= 25) || index == arpEnvelopeField || index == bassEnvelopeField;
    }
    static bool voicingField(int index) {
        return index == chordVoicingField || index == padVoicingField;
    }
    static bool soundField(int index) {
        return index == 13 || index == 14 || index == 21 || index == arpSoundField ||
               index == bassSoundField;
    }
    static int envelopePartFor(int field);
    static PlaybackMode mixLayer(int field) {
        return field == drumMixField   ? PlaybackMode::Drums
               : field == bassMixField ? PlaybackMode::Bass
                                       : backingLayerOrder[field - chordMixField];
    }
    std::string rowName(int index) const;
    std::string rowDescription() const;
    std::string settingValue(int index) const;
    std::string scopeName() const;
    Chord previewChord() const;
    bool fieldModified(int index) const;
    SetupDestination setupDestination() const;
    Confirmation confirmation() const;
    bool canRestoreField() const;
    bool appPage() const;
    int fieldValue() const;
    int defaultValue() const;
    int settingValueNumber() const;
    const char *frontSettingName(int index) const;
    std::vector<int> frontRows() const;
    std::vector<int> combinationRows() const;
    bool frontReset() const;
    int frontValue(const Settings &s, int field) const;
    int envelopeValue(const Settings &s) const;
    int chordOctaveValue(const Settings &s) const;
    int chordConfigValue(const Settings &s) const;
    static const char *envelopePartName(int part);
    static const char *envelopeControlName(int field);
    // The sound's own envelope for a part, with backing articulation bounds applied.
    std::optional<std::array<int, 4>> soundEnvelope(const Settings &s, int part) const;
    std::string envelopeValueName(const Settings &s, int part, int field) const;
    std::string envelopeSummary(const Settings &s, int part) const;
    void assignFront(Settings &s, int field, int value);
    std::string frontValueName(const Settings &s, int field) const;

  private:
    std::function<std::optional<int>()> readSystemVolume_;
    std::function<bool(int)> writeSystemVolume_;
    uint64_t now_ = 0, heldSince_ = 0, nextRepeat_ = 0;
    char heldKey_ = 0;
    bool repeating_ = false, fastRepeat_ = false;
    std::function<void()> deferred_; // Audition to play when a repeat ends.
    unsigned previews_ = 0; // Counts auditions started, to tell a new preview from an old one.
    void handleKey(char key, bool down);
    bool repeatable(char key) const;
    int step(int index, int delta, int count) const;
    void refreshSystemVolume();
    void adjust(int direction); // Step the selected value; the same path for every row.
    void nextTab();
    void jumpTab(int tab);
    void adjustMix(int direction);
    void adjustLoopLayer(int direction);
    void adjustSwing(int direction);
    void restoreSwing();
    void toggleLayer();
    void restoreLayer();
    void selectBackingTab(int tab);
    std::pair<int, std::string> soundSource(const Settings &s, int field) const;
    int valueFor(const Settings &s, int index) const;
    void assign(Settings &s, int index, int value);
    void setSettingValue(int value);
    void buildChoices();
    void openSetting();
    void returnToSetup();
    void back();
    void openPicker();
    void openTempo();
    void openBacking(bool fromPlay);
    void applyChoice(int value);
    void preview();
    void restoreField();
};
} // namespace zc
