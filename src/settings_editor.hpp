#pragma once
#include "instrument.hpp"
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>
namespace zc {
class SettingsEditor {
  public:
    enum class Page { Setup, Combinations, FrontNotes, Envelope, VolumeEffects };
    enum class Tab { Style, App, About };
    struct Choice {
        int value;
        std::string name, description;
    };
    static constexpr int volumeEffectsField = 29;
    static constexpr int systemVolumeField = 30;
    static constexpr int settingCount = systemVolumeField + 1;
    explicit SettingsEditor(Instrument &instrument,
                            std::function<std::optional<int>()> readSystemVolume = {},
                            std::function<bool(int)> writeSystemVolume = {})
        : instrument_(instrument), readSystemVolume_(std::move(readSystemVolume)),
          writeSystemVolume_(std::move(writeSystemVolume)) {}
    bool settingsOpen = false;
    void key(char key, bool down);
    void cancelPending();
    Page page() const { return page_; }
    int scope() const { return scope_; }
    int selected() const {
        return settingListPage() ? selection_
                                    : (page_ == Page::FrontNotes ? frontField_ :
                                       page_ == Page::Envelope ? envelopeField_ : combination_);
    }
    bool pickerOpen() const { return picker_; }
    bool backingOpen() const { return backing_; }
    unsigned pendingLayers() const { return pendingLayers_; }
    int backingRow() const { return backingRow_; }
    bool tempoOpen() const { return tempo_; }
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

  protected:
    static constexpr int chordHoldField = 9;
    enum class SetupDestination { Picker, Tempo, Backing, Envelope, FrontNotes, Combinations, VolumeEffects };
    enum class Confirmation { None, Create, Restore };
    struct BackingLayerRow {
        PlaybackMode mode;
        const char *description;
    };
    static constexpr std::array<BackingLayerRow, 3> backingLayerRows{{
        {PlaybackMode::Chord, "Simultaneous chord notes"},
        {PlaybackMode::Arp, "Patterned chord notes"},
        {PlaybackMode::Pad, "Sustained harmony"},
    }};
    static constexpr int backingApplyRow = int(backingLayerRows.size());
    static std::string backingLabel(PlaybackMode mode);
    Instrument &instrument_;
    Page page_ = Page::Setup;
    Tab tab_ = Tab::Style;
    int tabRows_[3] = {0, 0, 0};
    int volumeEffectsRow_ = 0;
    int selection_ = 2, combination_ = 6, scope_ = -1;
    int frontKey_ = -1, frontField_ = 0;
    int envelopePart_ = 0, envelopeField_ = 0;
    std::string soundCategory_;
    bool picker_ = false;
    bool backing_ = false;
    unsigned pendingLayers_ = 0;
    int backingRow_ = 0;
    bool tempo_ = false, tempoReplace_ = true;
    std::string tempoInput_, tempoError_;
    int choice_ = 0;
    std::vector<Choice> choices_;
    std::string status_;
    std::optional<int> systemVolume_;
    bool settingListPage() const { return page_ == Page::Setup || page_ == Page::VolumeEffects; }
    std::string settingValue(int index) const;
    std::string scopeName() const;
    Chord previewChord() const;
    bool fieldModified(int index) const;
    SetupDestination setupDestination() const;
    Confirmation confirmation() const;
    bool canRestoreField() const;
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
    static std::string envelopeValueName(int field, int value);
    void assignFront(Settings &s, int field, int value);
    std::string frontValueName(const Settings &s, int field) const;

  private:
    std::function<std::optional<int>()> readSystemVolume_;
    std::function<bool(int)> writeSystemVolume_;
    void refreshSystemVolume();
    void adjustVolumeEffects(int direction);
    int valueFor(const Settings &s, int index) const;
    void assign(Settings &s, int index, int value);
    void setSettingValue(int value);
    void buildChoices();
    void openSetting();
    void returnToSetup();
    void openPicker();
    void openTempo();
    void openBacking();
    void applyChoice(int value);
    void preview();
    void restoreField();
};
} // namespace zc
