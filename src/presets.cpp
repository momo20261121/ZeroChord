#include "presets.hpp"
#include "patterns.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <toml.hpp>

namespace zc {
namespace fs = std::filesystem;
namespace {
template <class F> void fields(Settings &s, F f) {
    f("backing_sound", s.chordProgram, 0, 127);
    f("pad_sound", s.padProgram, 0, 127);
    f("foreground_sound", s.harpProgram, 0, 127);
    f("bpm", s.bpm, 40, 240);
    static const char *parts[]{"front", "chord", "pad"};
    static const char *controls[]{"attack_ms", "decay_ms", "sustain_percent", "release_ms"};
    for (int part = 0; part < 3; part++)
        for (int control = 0; control < 4; control++) {
            auto name = std::string(parts[part]) + "_" + controls[control];
            f(name.c_str(), s.envelopes[part].values[control], -1, control == 2 ? 100 : 5000);
        }
}
void materialize(Settings &s) {
    const auto &p = presets[std::clamp(s.preset, 0, 3)];
    if (s.chordProgram < 0) {
        s.chordProgram = p.chordProgram;
        if (s.padProgram < 0 && p.padProgram >= 0)
            s.padProgram = p.padProgram;
    }
    if (s.padProgram < 0)
        s.padProgram = s.chordProgram;
    if (s.harpProgram < 0)
        s.harpProgram = p.harpProgram;
}
bool oldStockDream(const Settings &s) {
    Settings old;
    old.chordProgram = old.padProgram = 89;
    old.harpProgram = 10;
    return static_cast<const PerformanceSettings &>(s) ==
           static_cast<const PerformanceSettings &>(old);
}
bool validId(const std::string &s) {
    return !s.empty() && s.size() <= 48 && std::all_of(s.begin(), s.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    });
}
void atomicWrite(const fs::path &path, const toml::table &table) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path.string() + ".tmp");
    out << table << '\n';
    out.close();
    if (!out)
        throw std::runtime_error("Cannot save " + path.string());
    fs::rename(path.string() + ".tmp", path);
}
toml::table parse(const std::string &path) {
    if (fs::file_size(path) > 128 * 1024)
        throw std::runtime_error("Preset exceeds 128 KiB");
    return toml::parse_file(path);
}
} // namespace
bool samePresetSettings(const Settings &a, const Settings &b) {
    Settings x = a, y = b;
    materialize(x);
    materialize(y);
    return static_cast<const PerformanceSettings &>(x) ==
           static_cast<const PerformanceSettings &>(y);
}
void applyPresetSettings(Settings &target, const Settings &source) {
    // App controls and the learned keyboard layout survive every style operation.
    static_cast<PerformanceSettings &>(target) = source;
}
std::string programName(int n) { return soundLibrary().name(n, ""); }
void writePreset(const PresetDefinition &p, const std::string &path) {
    Settings s = p.sound;
    materialize(s);
    toml::table t{{"schema_version", 2}, {"id", p.id}, {"name", p.name}};
    fields(s, [&](const char *name, int &v, int, int) { t.insert(name, v); });
    for (const auto &[key, value] :
         std::vector<std::pair<const char *, std::string>>{{"pad_sound_id", s.padSound},
                                                           {"backing_sound_id", s.chordSound},
                                                           {"foreground_sound_id", s.frontSound}})
        if (!value.empty())
            t.insert(key, value);
    t.insert("mode", modeName(s.mode));
    for (auto mode : {PlaybackMode::Pad, PlaybackMode::Chord, PlaybackMode::Arp}) {
        Settings copy = s;
        copy.mode = mode;
        t.insert(mode == PlaybackMode::Pad     ? "pad_motion"
                 : mode == PlaybackMode::Chord ? "chord_pattern"
                                               : "arp_pattern",
                 patterns(mode)[patternIndex(copy)].id);
    }
    atomicWrite(path, t);
}
PresetDefinition readPreset(const std::string &path) {
    auto t = parse(path);
    if (t["schema_version"].value<int>() != 1 && t["schema_version"].value<int>() != 2)
        throw std::runtime_error("Unsupported preset version");
    PresetDefinition p;
    p.sound.preset = 0; // Sparse files retain the original Harp fallback.
    p.id = t["id"].value_or(std::string());
    p.name = t["name"].value_or(std::string());
    if (!validId(p.id) || p.name.empty() || p.name.size() > 40 ||
        std::any_of(p.name.begin(), p.name.end(),
                    [](unsigned char c) { return c < 32 || c == 127; }))
        throw std::runtime_error("Invalid preset id/name");
    std::set<std::string> allowed{"schema_version", "id",           "name",
                                  "mode",           "pad_motion",   "chord_pattern",
                                  "arp_pattern",    "combinations", "degree_combinations"};
    fields(p.sound, [&](const char *name, int &v, int low, int high) {
        allowed.insert(name);
        if (!t.contains(name))
            return;
        auto n = t[name].value<int64_t>();
        if (!t[name].is_integer() || !n || *n < low || *n > high)
            throw std::runtime_error(std::string("Invalid ") + name);
        v = int(*n);
    });
    for (auto [key, target] : std::vector<std::pair<const char *, std::string *>>{
             {"pad_sound_id", &p.sound.padSound},
             {"backing_sound_id", &p.sound.chordSound},
             {"foreground_sound_id", &p.sound.frontSound}}) {
        allowed.insert(key);
        if (!t.contains(key))
            continue;
        auto value = t[key].value<std::string>();
        if (!value || value->size() > 100 || value->empty() ||
            std::any_of(value->begin(), value->end(), [](unsigned char c) {
                return !(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '-' && c != '/';
            }))
            throw std::runtime_error(std::string("Invalid ") + key);
        *target = *value;
    }
    // Former style controls are app-wide now. Accept old files without applying them.
    for (const auto &[key, bounds] :
         std::vector<std::pair<const char *, std::pair<int, int>>>{{"backing_volume", {0, 100}},
                                                                   {"foreground_volume", {0, 100}},
                                                                   {"level", {0, 100}},
                                                                   {"echo", {0, 60}},
                                                                   {"sweep_ms", {60, 600}}}) {
        allowed.insert(key);
        if (!t.contains(key))
            continue;
        auto n = t[key].value<int64_t>();
        if (!t[key].is_integer() || !n || *n < bounds.first || *n > bounds.second)
            throw std::runtime_error(std::string("Invalid ") + key);
    }
    // Read v1's former app fields for compatibility; style application ignores them.
    for (const char *key : {"tonic", "scale", "hold"}) {
        allowed.insert(key);
        if (t.contains(key)) {
            auto n = t[key].value<int64_t>();
            int high = std::string(key) == "tonic" ? 11 : 1;
            if (!t[key].is_integer() || !n || *n < 0 || *n > high)
                throw std::runtime_error(std::string("Invalid ") + key);
            if (std::string(key) == "tonic")
                p.sound.tonic = int(*n);
            else if (std::string(key) == "scale")
                p.sound.scale = int(*n);
            else
                p.sound.hold = int(*n);
        }
    }
    for (const auto &[key, value] : t)
        if (!allowed.count(std::string(key.str())))
            throw std::runtime_error("Unknown preset field: " + std::string(key.str()));
    if (t.contains("mode")) {
        auto value = t["mode"].value<std::string>();
        bool found = false;
        for (int n = 0; n < 4; n++)
            if (value && *value == modeName(PlaybackMode(n))) {
                p.sound.mode = PlaybackMode(n);
                found = true;
            }
        if (!found)
            throw std::runtime_error("Unknown playback mode");
    }
    for (auto mode : {PlaybackMode::Pad, PlaybackMode::Chord, PlaybackMode::Arp}) {
        const char *key = mode == PlaybackMode::Pad     ? "pad_motion"
                          : mode == PlaybackMode::Chord ? "chord_pattern"
                                                        : "arp_pattern";
        if (!t.contains(key))
            continue;
        auto id = t[key].value<std::string>();
        bool found = false;
        const auto &list = patterns(mode);
        for (int n = 0; n < int(list.size()); n++)
            if (id && *id == list[n].id) {
                auto old = p.sound.mode;
                p.sound.mode = mode;
                setPatternIndex(p.sound, n);
                p.sound.mode = old;
                found = true;
            }
        if (!found)
            throw std::runtime_error(std::string("Unknown ") + key);
    }
    auto quality = [](const toml::node &node, bool inherit) {
        auto name = node.value<std::string>();
        if (name && inherit && *name == "inherit")
            return -1;
        for (int n = 0; n < qualityCount; n++)
            if (name && *name == qualityName(Quality(n)))
                return n;
        throw std::runtime_error("Unknown combination quality");
    };
    if (t.contains("combinations")) {
        auto *a = t["combinations"].as_array();
        if (!a || a->size() != 4)
            throw std::runtime_error("Expected four combinations");
        for (int n = 0; n < 4; n++)
            p.sound.combinations[n] = quality(*a->get(n), false);
    }
    if (t.contains("degree_combinations")) {
        auto *a = t["degree_combinations"].as_array();
        if (!a || a->size() != 7)
            throw std::runtime_error("Expected seven degree mappings");
        for (int col = 0; col < 7; col++) {
            auto *row = a->get(col)->as_array();
            if (!row || row->size() != 4)
                throw std::runtime_error("Expected four degree combinations");
            for (int n = 0; n < 4; n++)
                p.sound.degreeCombinations[col][n] = quality(*row->get(n), true);
        }
    }
    materialize(p.sound);
    return p;
}
PresetLibrary::PresetLibrary() {
    const char *ids[] = {"harp", "dream", "keys", "organ"};
    for (int n = 0; n < 4; n++) {
        Settings s;
        s.preset = n;
        materialize(s);
        entries.push_back({ids[n], presets[n].name, s, true});
    }
    const char *ids2[] = {"pop", "rnb", "rock", "waltz", "bossa"};
    const char *names[] = {"Pop", "R&B", "Rock", "Waltz", "Bossa"};
    const int tempos[] = {112, 82, 120, 96, 128};
    const int backing[] = {4, 4, 16, 48, 24}, front[] = {24, 11, 24, 46, 11};
    const int chordPatterns[] = {1, 3, 1, 4, 2}, padMotions[] = {0, 2, 3, 0, 2};
    for (int n = 0; n < 5; n++) {
        Settings s = entries[n % 4].sound;
        s.mode = PlaybackMode::Arp;
        s.pattern = n + 4;
        s.bpm = tempos[n];
        s.chordProgram = backing[n];
        s.padProgram = backing[n];
        s.harpProgram = front[n];
        s.chordPattern = chordPatterns[n];
        s.padMotion = padMotions[n];
        entries.push_back({ids2[n], names[n], s, true});
    }
}
const PresetDefinition &PresetLibrary::current(const Settings &s) const {
    return entries.at(std::clamp(s.preset, 0, int(entries.size()) - 1));
}
std::vector<int> PresetLibrary::order() const {
    std::vector<int> result{1, 0}; // Presentation order; persisted indices and IDs stay stable.
    for (int n = 2; n < int(entries.size()); n++)
        result.push_back(n);
    return result;
}
Settings PresetLibrary::resolved(int index) const {
    const auto &p = entries.at(index);
    auto it = working.find(p.id);
    Settings s = it == working.end() ? p.sound : it->second;
    s.preset = index;
    return s;
}
void PresetLibrary::select(Settings &s, int index) {
    working[current(s).id] = s;
    index = std::clamp(index, 0, int(entries.size()) - 1);
    auto next = resolved(index);
    applyPresetSettings(s, next);
    s.preset = index;
}
bool PresetLibrary::modified(const Settings &s) const {
    return !samePresetSettings(s, current(s).sound);
}
void PresetLibrary::restore(Settings &s) {
    applyPresetSettings(s, current(s).sound);
    working[current(s).id] = s;
}
void PresetLibrary::load(const std::string &dir, Settings &s, bool canWrite) {
    directory = dir;
    writable = canWrite;
    auto warn = [&](const std::exception &e) { warning += std::string(e.what()) + "\n"; };
    for (auto &p : entries) {
        fs::path path = fs::path(dir) / "presets" / "factory" / (p.id + ".toml");
        try {
            if (fs::exists(path)) {
                auto loaded = readPreset(path.string());
                if (loaded.id != p.id)
                    throw std::runtime_error("Factory preset id changed");
                if (p.id == "dream" && oldStockDream(loaded.sound)) {
                    loaded.sound.chordProgram = 88;
                    if (writable)
                        writePreset(loaded, path.string());
                }
                p = loaded;
            } else if (writable)
                writePreset(p, path.string());
        } catch (const std::exception &e) {
            warn(e);
        }
    }
    auto user = fs::path(dir) / "presets" / "user";
    if (fs::exists(user)) {
        std::vector<fs::path> files;
        for (const auto &f : fs::directory_iterator(user))
            if (f.path().extension() == ".toml")
                files.push_back(f.path());
        std::sort(files.begin(), files.end());
        for (const auto &path : files) {
            try {
                if (entries.size() >= 128)
                    throw std::runtime_error("Preset limit is 128");
                auto p = readPreset(path.string());
                if (std::any_of(entries.begin(), entries.end(),
                                [&](const auto &x) { return x.id == p.id; }))
                    throw std::runtime_error("Duplicate preset id: " + p.id);
                p.factory = false;
                entries.push_back(p);
            } catch (const std::exception &e) {
                warn(e);
            }
        }
    }
    auto session = fs::path(dir) / "session.toml";
    if (fs::exists(session)) {
        for (const auto &p : entries) {
            auto path = fs::path(dir) / "working" / (p.id + ".toml");
            try {
                if (fs::exists(path)) {
                    auto w = readPreset(path.string());
                    if (w.id != p.id)
                        throw std::runtime_error("Working preset id mismatch");
                    if (p.id == "dream" && oldStockDream(w.sound) &&
                        p.sound.chordProgram == 88 && p.sound.padProgram == 89)
                        w.sound.chordProgram = 88;
                    working[p.id] = w.sound;
                }
            } catch (const std::exception &e) {
                warn(e);
            }
        }
        try {
            auto t = parse(session.string());
            auto id = t["selected"].value_or(std::string());
            for (int n = 0; n < int(entries.size()); n++)
                if (entries[n].id == id)
                    s.preset = n;
        } catch (const std::exception &e) {
            warn(e);
        }
        s.preset = std::clamp(s.preset, 0, int(entries.size()) - 1);
        applyPresetSettings(s, resolved(s.preset));
    } else {
        // Migrate an existing setup; a clean first launch uses its TOML defaults.
        bool migrate =
            fs::exists(fs::path(dir) / "settings.conf") || !samePresetSettings(s, Settings{});
        s.preset = std::clamp(s.preset, 0, 3);
        if (migrate) {
            materialize(s);
            working[current(s).id] = s;
        } else
            applyPresetSettings(s, current(s).sound);
    }
}
void PresetLibrary::save(Settings &s) {
    working[current(s).id] = s;
    if (!writable || directory.empty())
        return;
    for (const auto &p : entries) {
        auto it = working.find(p.id);
        if (it == working.end())
            continue;
        auto copy = p;
        copy.sound = it->second;
        writePreset(copy, (fs::path(directory) / "working" / (p.id + ".toml")).string());
    }
    atomicWrite(fs::path(directory) / "session.toml",
                toml::table{{"schema_version", 1}, {"selected", current(s).id}});
}
void PresetLibrary::saveAs(Settings &s) {
    if (entries.size() >= 128)
        throw std::runtime_error("Preset limit is 128");
    int n = 1;
    std::string id, name;
    do {
        std::ostringstream num;
        num << std::setw(3) << std::setfill('0') << n++;
        id = "user-" + num.str();
        name = "User" + num.str();
    } while (
        std::any_of(entries.begin(), entries.end(), [&](const auto &p) { return p.id == id; }) ||
        (!directory.empty() &&
         fs::exists(fs::path(directory) / "presets" / "user" / (id + ".toml"))));
    PresetDefinition copy{id, name, s, false};
    materialize(copy.sound);
    if (writable && !directory.empty())
        writePreset(copy, (fs::path(directory) / "presets" / "user" / (id + ".toml")).string());
    entries.push_back(copy);
    select(s, int(entries.size()) - 1);
    save(s);
}
} // namespace zc
