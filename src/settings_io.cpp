#include "settings_io.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace zc {
namespace {
template <class F> void fields(Settings &s, F field) {
    int mode = int(s.mode);
    field("mode", mode);
    s.mode = PlaybackMode(mode);
    field("hold", s.hold);
    field("presetLevel", s.presetLevel);
    field("masterVolume", s.masterVolume);
    field("theme", s.theme);
    field("chordProgram", s.chordProgram);
    field("harpProgram", s.harpProgram);
    field("chordPattern", s.chordPattern);
    field("padMotion", s.padMotion);
    field("tonic", s.tonic);
    field("scale", s.scale);
    field("preset", s.preset);
    field("chordVolume", s.chordVolume);
    field("harpVolume", s.harpVolume);
    field("echo", s.echo);
    field("strumMs", s.strumMs);
    field("bpm", s.bpm);
    field("pattern", s.pattern);
    for (int n = 0; n < 4; n++) {
        auto name = "combo" + std::to_string(combinationMasks[n]);
        field(name, s.combinations[n]);
        for (int column = 0; column < 7; column++)
            field("degree" + std::to_string(column) + name, s.degreeCombinations[column][n]);
    }
}
} // namespace
void loadSettings(Settings &s, const std::string &path) {
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::string name;
        int value;
        if (!(row >> name >> value))
            continue;
        fields(s, [&](const std::string &key, int &setting) {
            if (key == name)
                setting = value;
        });
    }
    s.clamp();
}
void saveSettings(const Settings &settings, const std::string &path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent);
    std::ofstream out(path + ".tmp");
    Settings copy = settings;
    fields(copy, [&](const std::string &name, int &value) { out << name << ' ' << value << '\n'; });
    out.close();
    if (!out)
        throw std::runtime_error("Cannot save settings");
    std::filesystem::rename(path + ".tmp", path);
}
} // namespace zc
