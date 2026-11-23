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
    const char *enterHint() const;
    void drawAbout();
    void drawSettingRows(int top, int limit, int spacing);
    void drawVolumeEffects();
    void drawPicker();
    void drawTempo();
    void drawBacking();
    void drawEnvelope();
    void drawCombinations();
    void drawFrontNotes();
};
} // namespace zc
