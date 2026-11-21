#pragma once
#include "instrument.hpp"
#include <string>
#include <vector>
namespace zc {
class SettingsEditor {
  public:
    enum class Page { Setup, Combinations };
    struct Choice {
        int value;
        std::string name, description;
    };
    static constexpr int settingCount = 21;
    explicit SettingsEditor(Instrument &instrument) : instrument_(instrument) {}
    bool settingsOpen = false;
    void key(char key, bool down);
    void cancelPending();
    Page page() const { return page_; }
    int scope() const { return scope_; }
    int selected() const { return page_ == Page::Setup ? selection_ : combination_; }
    bool pickerOpen() const { return picker_; }
    const std::vector<Choice> &choices() const { return choices_; }
    int highlighted() const { return choice_; }
    static const char *settingName(int index);
    static const char *combinationName(int index);

  protected:
    Instrument &instrument_;
    Page page_ = Page::Setup;
    int selection_ = 0, combination_ = 0, scope_ = -1;
    bool picker_ = false, reset_ = false;
    int choice_ = 0, savedChoice_ = 0;
    std::vector<Choice> choices_;
    std::string status_;
    std::string settingValue(int index) const;
    std::string scopeName() const;
    Chord previewChord() const;
    bool fieldModified(int index) const;
    int defaultValue() const;

  private:
    int valueFor(const Settings &s, int index) const;
    int settingValueNumber() const;
    void assign(Settings &s, int index, int value);
    void setSettingValue(int value);
    void buildChoices();
    void openPicker();
    void applyChoice(int value);
    void preview();
    void adjust(int direction);
    void restoreField();
};
} // namespace zc
