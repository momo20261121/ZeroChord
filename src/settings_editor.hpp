#pragma once
#include "instrument.hpp"
#include <string>
#include <vector>
namespace zc {
class SettingsEditor {
  public:
    enum class Page { Setup, Combinations, FrontNotes, Envelope };
    enum class Tab { Style, App };
    struct Choice {
        int value;
        std::string name, description;
    };
    static constexpr int settingCount = 29;
    explicit SettingsEditor(Instrument &instrument) : instrument_(instrument) {}
    bool settingsOpen = false;
    void key(char key, bool down);
    void cancelPending();
    Page page() const { return page_; }
    int scope() const { return scope_; }
    int selected() const {
        return page_ == Page::Setup ? selection_
                                    : (page_ == Page::FrontNotes ? frontField_ :
                                       page_ == Page::Envelope ? envelopeField_ : combination_);
    }
    bool pickerOpen() const { return picker_; }
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
    Instrument &instrument_;
    Page page_ = Page::Setup;
    Tab tab_ = Tab::Style;
    int tabRows_[2] = {0, 0};
    int selection_ = 2, combination_ = 6, scope_ = -1;
    int frontKey_ = -1, frontField_ = 0;
    int envelopePart_ = 0, envelopeField_ = 0;
    std::string soundCategory_;
    bool picker_ = false, reset_ = false;
    bool tempo_ = false, tempoReplace_ = true;
    std::string tempoInput_, tempoError_;
    int choice_ = 0, savedChoice_ = 0;
    std::vector<Choice> choices_;
    std::string status_;
    std::string settingValue(int index) const;
    std::string scopeName() const;
    Chord previewChord() const;
    bool fieldModified(int index) const;
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
    int valueFor(const Settings &s, int index) const;
    void assign(Settings &s, int index, int value);
    void setSettingValue(int value);
    void buildChoices();
    void openPicker();
    void openTempo();
    void applyChoice(int value);
    void preview();
    void restoreField();
};
} // namespace zc
