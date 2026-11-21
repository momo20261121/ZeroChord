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
    void select(Settings &settings, int index);
    bool modified(const Settings &settings) const;
    void restore(Settings &settings);
    void saveAs(Settings &settings);
    const PresetDefinition &current(const Settings &settings) const;
};
bool samePresetSettings(const Settings &a, const Settings &b);
void applyPresetSettings(Settings &target, const Settings &source);
std::string programName(int program);
void writePreset(const PresetDefinition &preset, const std::string &path);
PresetDefinition readPreset(const std::string &path);
} // namespace zc
