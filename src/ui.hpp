#pragma once
#include "settings_editor.hpp"
#include <string>
namespace zc {
class UI : public SettingsEditor {
  public:
    explicit UI(Instrument &instrument) : SettingsEditor(instrument) {}
    void draw();

  private:
    std::string previous_;
    void drawPicker();
    void drawCombinations();
};
} // namespace zc
