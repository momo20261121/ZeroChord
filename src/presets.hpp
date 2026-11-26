#pragma once
#include "music.hpp"
#include <map>
#include <string>
#include <vector>
namespace zc {
struct PresetDefinition {
    std::string id, name;
    Settings sound;
    bool factory = true;
};
class PresetLibrary {
  public:
    PresetLibrary();
    std::vector<PresetDefinition> entries;
    std::map<std::string, Settings> working;
    std::string directory, warning;
    bool writable = true;
    void load(const std::string &configDirectory, Settings &settings, bool writable = true);
    void save(Settings &settings);
    Settings resolved(int index) const;
    std::vector<int> order() const;
    void select(Settings &settings, int index);
    bool modified(const Settings &settings) const;
    void restore(Settings &settings);
    void saveAs(Settings &settings);
    const PresetDefinition &current(const Settings &settings) const;
    int indexOf(const std::string &id) const; // Dream when the ID is not available.
};
bool samePresetSettings(const Settings &a, const Settings &b);
void applyPresetSettings(Settings &target, const Settings &source);
std::string programName(int program);
void writePreset(const PresetDefinition &preset, const std::string &path);
PresetDefinition readPreset(const std::string &path);
// Built-in presets as shipped: 0 first release, 1 first tuning, 2 1.2, 3 1.3, 4 and 5 1.4
// development builds, 6 current. Saved copies equal to an older release upgrade on load.
std::vector<PresetDefinition> stockPresets(int release);
} // namespace zc
