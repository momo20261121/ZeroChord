#pragma once
#include "settings_editor.hpp"
#include <memory>
#include <string>
namespace zc {
struct Hint {
    const char *key = nullptr;
    std::string action;
    std::string shortAction = {}; // Used when the full action does not fit the line.
};
class UI : public SettingsEditor {
  public:
    using SettingsEditor::SettingsEditor;
    void draw();

  private:
    std::string previous_, previousPlaying_;
    // Widgets the playing screen updates in place, so playing never waits for a full rebuild.
    struct PerformanceView;
    std::shared_ptr<PerformanceView> performance_;
    void updatePerformance();
    Hint restoreHint() const;
    void menuFooter(bool edited);
    void drawPerformance();
    void drawAbout();
    bool drawSettingRows(int top, int limit, int spacing); // True when a listed row is edited.
    void drawVolumeEffects();
    void drawMidiInput();
    void drawPicker();
    void drawTempo();
    void drawBacking();
    void drawFrontSound();
    void drawEnvelope();
    void drawCombinations();
    void drawFrontNotes();
};
} // namespace zc
