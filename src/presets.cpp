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
    f("arp_sound", s.arpProgram, 0, 127);
    f("bpm", s.bpm, 40, 240);
    f("chord_mix", s.layerMix[0], 0, 100);
    f("arp_mix", s.layerMix[1], 0, 100);
    f("pad_mix", s.layerMix[2], 0, 100);
    f("drum_mix", s.mix(PlaybackMode::Drums), 0, 100);
    f("bass_mix", s.mix(PlaybackMode::Bass), 0, 100);
    f("bass", s.bass, 0, 1);
    f("bass_sound", s.bassProgram, 0, 127);
    f("swing_amount", s.swingAmount, 0, 100);
    f("reverb_ms", s.reverbMs, 300, 8000);
    f("front_mix", s.frontMix, 0, 100);
    static const char *fx[]{"front", "chord", "arp", "pad", "bass", "drums"};
    for (int part = 0; part < fxPartCount; part++) {
        const std::string name = fx[part];
        f((name + "_pan").c_str(), s.pan[part], -50, 50);
        f((name + "_reverb").c_str(), s.reverbSend[part], 0, 100);
        f((name + "_echo").c_str(), s.echoSend[part], 0, 100);
    }
    f("drums", s.drums, 0, 1);
    f("drum_kit", s.drumKit, 0, 127);
    static const char *parts[]{"front", "chord", "pad", "arp", "bass"};
    static const char *controls[]{"attack_ms", "decay_ms", "sustain_percent", "release_ms"};
    for (int part = 0; part < 5; part++)
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
    if (s.arpProgram < 0) {
        // Arp shared the Chord sound before 1.3; keep older setups sounding the same.
        s.arpProgram = s.chordProgram;
        s.arpSound = s.chordSound;
    }
}
bool oldStockDream(const Settings &s) {
    Settings old;
    old.chordProgram = old.padProgram = old.arpProgram = 89;
    old.harpProgram = 10;
    return static_cast<const PerformanceSettings &>(s) ==
           static_cast<const PerformanceSettings &>(old);
}
const char *patternKey(PlaybackMode mode) {
    return mode == PlaybackMode::Pad     ? "pad_motion"
           : mode == PlaybackMode::Chord ? "chord_pattern"
           : mode == PlaybackMode::Drums ? "drum_pattern"
           : mode == PlaybackMode::Bass  ? "bass_pattern"
                                         : "arp_pattern";
}
// Named choices stored as words: swing mode and the Chord and Pad voicings.
const char *swingKey(int mode) {
    static const char *keys[]{"off", "8ths", "16ths"};
    return keys[std::clamp(mode, 0, swingModeCount - 1)];
}
const char *voicingKey(int voicing) {
    static const char *keys[]{"close", "open", "power", "rich"};
    return keys[std::clamp(voicing, 0, voicingCount - 1)];
}
template <class F> void choiceFields(Settings &s, F f) {
    f("swing_mode", s.swingMode, swingModeCount, swingKey);
    f("chord_voicing", s.chordVoicing, voicingCount, voicingKey);
    f("pad_voicing", s.padVoicing, voicingCount, voicingKey);
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
                                                           {"foreground_sound_id", s.frontSound},
                                                           {"arp_sound_id", s.arpSound},
                                                           {"drum_sound_id", s.drumSound},
                                                           {"bass_sound_id", s.bassSound}})
        if (!value.empty())
            t.insert(key, value);
    t.insert("mode", modeName(s.mode));
    choiceFields(s, [&](const char *name, int &v, int, const char *(*key)(int)) {
        t.insert(name, key(v));
    });
    for (auto mode : {PlaybackMode::Pad, PlaybackMode::Chord, PlaybackMode::Arp,
                      PlaybackMode::Drums, PlaybackMode::Bass}) {
        Settings copy = s;
        copy.mode = mode;
        t.insert(patternKey(mode), patterns(mode)[patternIndex(copy)].id);
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
    std::set<std::string> allowed{"schema_version", "id",           "name",           "swing",
                                  "mode",           "pad_motion",   "chord_pattern",
                                  "arp_pattern",    "drum_pattern", "bass_pattern",
                                  "combinations",   "degree_combinations"};
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
             {"foreground_sound_id", &p.sound.frontSound},
             {"arp_sound_id", &p.sound.arpSound},
             {"drum_sound_id", &p.sound.drumSound},
             {"bass_sound_id", &p.sound.bassSound}}) {
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
    // A 1.4 development build stored swing as a 16th ratio of 50-75%; styles now start
    // without swing, so the old value is accepted and left out.
    if (t.contains("swing")) {
        auto n = t["swing"].value<int64_t>();
        if (!t["swing"].is_integer() || !n || *n < 50 || *n > 75)
            throw std::runtime_error("Invalid swing");
    }
    choiceFields(p.sound, [&](const char *name, int &v, int count, const char *(*key)(int)) {
        allowed.insert(name);
        if (!t.contains(name)) return;
        auto word = t[name].value<std::string>();
        for (int n = 0; n < count; n++)
            if (word && *word == key(n)) {
                v = n;
                return;
            }
        throw std::runtime_error(std::string("Invalid ") + name);
    });
    for (const auto &[key, value] : t)
        if (!allowed.count(std::string(key.str())))
            throw std::runtime_error("Unknown preset field: " + std::string(key.str()));
    if (t.contains("mode")) {
        auto value = t["mode"].value<std::string>();
        bool found = false;
        for (int n = 0; n < 8; n++)
            if (value && *value == modeName(PlaybackMode(n))) {
                p.sound.mode = PlaybackMode(n);
                found = true;
            }
        if (!found)
            throw std::runtime_error("Unknown playback mode");
    }
    for (auto mode : {PlaybackMode::Pad, PlaybackMode::Chord, PlaybackMode::Arp,
                      PlaybackMode::Drums, PlaybackMode::Bass}) {
        const char *key = patternKey(mode);
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
    if (!t.contains("arp_attack_ms") && !t.contains("arp_decay_ms") &&
        !t.contains("arp_sustain_percent") && !t.contains("arp_release_ms"))
        p.sound.envelopes[ArpEnvelope] = p.sound.envelopes[ChordEnvelope];
    materialize(p.sound);
    return p;
}
namespace {
// Generations of the built-in presets. Saved copies equal to an older generation upgrade.
// Drums14 was a 1.4 development build: drums, Dance, and the 1.3 layer choices.
// Styles14 was a later 1.4 development build: five layers each, mixes from measured levels.
enum class Factory { Legacy, FirstTuning, Release12, Release13, Drums14, Styles14, Current };
std::vector<PresetDefinition> factoryPresets(Factory generation = Factory::Current) {
    const bool legacy = generation == Factory::Legacy;
    const bool previousTuning = generation == Factory::FirstTuning;
    std::vector<PresetDefinition> entries;
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
    if (!legacy) {
        // Stable IDs and indices; deliberately distinct, playable musical starting points.
        const int front[]{46,10,0,16,25,4,29,8,11};
        const int chord[]{24,88,0,16,4,4,27,0,24};
        const int pad[]{48,89,89,89,89,89,16,48,89};
        const int bpm[]{96,80,100,100,112,82,120,96,128};
        const int chordPattern[]{5,5,5,5,1,3,6,4,7};
        const PlaybackMode modes[]{PlaybackMode::Pad, PlaybackMode::Pad, PlaybackMode::Chord,
            PlaybackMode::Chord, PlaybackMode::Arp, PlaybackMode::Chord, PlaybackMode::Chord,
            PlaybackMode::Chord, PlaybackMode::Chord};
        for (int n = 0; n < 9; ++n) {
            auto &s = entries[n].sound;
            s.harpProgram = front[n]; s.chordProgram = chord[n]; s.padProgram = pad[n];
            s.bpm = bpm[n]; s.mode = modes[n]; s.chordPattern = chordPattern[n];
            if (previousTuning && n < 3) s.chordPattern = 0;
            if (!previousTuning && n == 1) s.mode = PlaybackMode::All;
            s.pattern = n == 5 ? 5 : n == 4 ? 4 : 0;
            s.padMotion = n == 1 ? 1 : 0;
            // Acoustic attacks and decays come from the sample, including key-dependent timing.
            s.envelopes[0].values = {-1,-1,-1, n == 1 || n == 7 || n == 8 ? 320 : 180};
            s.envelopes[1].values = {5,-1,-1,100};
            s.envelopes[2].values = pad[n] == 48 ? std::array<int,4>{150,600,75,400}
                : pad[n] == 16 ? std::array<int,4>{80,300,75,250}
                              : std::array<int,4>{300,800,75,550};
        }
        entries[1].sound.envelopes[1].values = {8,400,35,200};
        entries[3].sound.envelopes[0].values = {5,100,85,90};
        entries[3].sound.envelopes[1].values = {5,100,85,100};
        entries[6].sound.envelopes[1].values = {3,160,45,65};
    }
    for (auto &entry : entries) {
        auto &s = entry.sound;
        s.envelopes[ArpEnvelope] = s.envelopes[ChordEnvelope];
        s.arpProgram = s.chordProgram;
        s.arpSound = s.chordSound;
        if (generation < Factory::Release13) continue;
        // Every layer starts selected. Layers that were off join quietly, so each preset
        // keeps its main character; mix or mute them in Backing.
        for (auto layer : {PlaybackMode::Chord, PlaybackMode::Arp, PlaybackMode::Pad})
            if (!layerEnabled(s.mode, layer)) s.mix(layer) = 30;
        s.mode = PlaybackMode::All;
    }
    if (generation < Factory::Drums14) return entries;
    // 1.4: Harp, Keys and Organ retire; Dance joins; every style gets a drum groove.
    {
        PresetDefinition dance{"dance", "Dance", entries[4].sound, true};
        auto &s = dance.sound;
        s.bpm = 124;
        s.harpProgram = 80;  // Square lead
        s.chordProgram = 62; // Synth brass: short offbeat stabs
        s.arpProgram = 81;   // Saw lead
        s.padProgram = 89;   // Warm pad
        s.chordPattern = 2;  // Offbeat
        s.pattern = 0;       // Up
        s.padMotion = 0;
        s.layerMix = {80, 40, 30, 0, 75};
        s.envelopes[FrontEnvelope].values = {-1, -1, -1, 180};
        s.envelopes[ChordEnvelope].values = {3, 160, 45, 80};
        s.envelopes[ArpEnvelope].values = {3, 120, 40, 80};
        s.envelopes[PadEnvelope].values = {300, 800, 75, 550};
        entries.push_back(dance);
    }
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [](const PresetDefinition &p) {
                                     return p.id == "harp" || p.id == "keys" || p.id == "organ";
                                 }),
                  entries.end());
    struct Groove {
        const char *id;
        int kit, pattern, mix;
    };
    // Kits from the General MIDI drum bank: 0 Standard, 8 Room, 16 Power, 25 TR-808,
    // 32 Jazz, 40 Brush. Patterns index patterns(PlaybackMode::Drums).
    static constexpr Groove grooves[]{{"dream", 40, 8, 35}, {"pop", 0, 1, 60},
                                      {"rnb", 8, 3, 55},    {"rock", 16, 2, 65},
                                      {"waltz", 32, 7, 45}, {"bossa", 32, 6, 50},
                                      {"dance", 25, 5, 75}};
    for (auto &entry : entries)
        for (const auto &groove : grooves)
            if (entry.id == groove.id) {
                entry.sound.drums = 1;
                entry.sound.drumKit = groove.kit;
                entry.sound.drumPattern = groove.pattern;
                if (entry.id != "dance") entry.sound.mix(PlaybackMode::Drums) = groove.mix;
            }
    for (auto &entry : entries)
        if (entry.id == "waltz") entry.sound.pattern = 7; // Broken 3/4 Arp, matching the bar.
    if (generation == Factory::Drums14) return entries;
    // 1.4 styles: every layer selected, each with its own sound, pattern, envelope and level.
    // Drum kits prefer the optional FluidR3 pack and fall back to the same built-in kit.
    struct Layer {
        int program, pattern, mix;
        std::array<int, 4> envelope;
    };
    struct Style {
        const char *id;
        int bpm, swing, front;
        std::array<int, 4> frontEnvelope;
        Layer chord, arp, pad, bass;
        int kit;
        const char *kitId;
        int drumPattern, drumMix;
    };
    // Programs: 0 Piano, 4 Electric Piano, 8 Celesta, 10 Music Box, 11 Vibraphone, 16 Organ,
    // 21 Accordion, 24 Nylon, 25 Steel-string, 27 Clean Electric, 29 Overdriven Guitar,
    // 32 Acoustic Bass, 33 Fingered Bass, 38 Synth Bass, 48 Strings, 50 Synth Strings,
    // 62 Synth Brass, 73 Flute, 80 Square Lead, 81 Saw Lead, 88 Airy Pad, 89 Warm Pad,
    // 90 Polysynth. Patterns index patterns(): Chord 2 Offbeat, 3 Syncopated, 4 Bass/chord 3/4,
    // 5 Sustained, 6 Every eighth, 7 Bossa; Arp 2 Up/down, 4 Broken eighths, 5 Syncopated,
    // 6 Root/fifth and chord, 7 Broken 3/4, 8 Bossa mixed, 9 Sixteenths; Pad 0 Steady,
    // 1 Slow swell, 2 Breathe, 3 Pulse; Bass 1 Held, 3 Eighths, 5 Offbeat, 6 Syncopated,
    // 7 Bossa, 8 Waltz; Drums 1 Pop, 2 Rock, 3 R&B, 5 Four on the floor, 6 Bossa, 7 Waltz,
    // 8 Brushes.
    // Mixes were set from measured levels so each layer sits where the style wants it,
    // relative to its lead layer; quiet samples (Celesta, the basses) need higher numbers.
    static const Style styles[]{
        {"dream", 76, 50, 10, {-1, -1, -1, 320},
         {88, 5, 50, {400, -1, -1, 1200}}, {8, 2, 100, {5, 700, 0, 600}},
         {89, 1, 70, {300, 800, 75, 900}}, {32, 1, 65, {60, -1, -1, 500}},
         40, "fluid-drums/brush", 8, 35},
        {"pop", 108, 54, 25, {-1, -1, -1, 180},
         {0, 6, 100, {5, -1, -1, 120}}, {27, 4, 80, {5, -1, -1, 100}},
         {50, 0, 25, {300, 800, 75, 550}}, {33, 3, 100, {5, -1, -1, 80}},
         0, "fluid-drums/standard", 1, 95},
        {"rnb", 84, 60, 4, {-1, -1, -1, 250},
         {4, 3, 70, {8, -1, -1, 300}}, {11, 5, 20, {5, -1, -1, 250}},
         {89, 2, 30, {300, 800, 75, 550}}, {33, 6, 100, {5, -1, -1, 100}},
         8, "fluid-drums/room", 3, 90},
        {"rock", 124, 50, 27, {-1, -1, -1, 180},
         {29, 6, 100, {3, 160, 45, 65}}, {27, 6, 35, {3, -1, -1, 80}},
         {16, 0, 15, {80, 300, 75, 250}}, {33, 3, 75, {3, -1, -1, 60}},
         16, "fluid-drums/power", 2, 40},
        {"waltz", 100, 50, 21, {-1, -1, -1, 250},
         {0, 4, 100, {5, -1, -1, 150}}, {8, 7, 40, {5, -1, -1, 300}},
         {48, 1, 30, {150, 600, 75, 400}}, {32, 8, 85, {5, -1, -1, 150}},
         40, "fluid-drums/brush", 7, 30},
        {"bossa", 132, 50, 73, {-1, -1, -1, 250},
         {24, 7, 100, {5, -1, -1, 150}}, {11, 8, 30, {5, -1, -1, 250}},
         {89, 0, 25, {300, 800, 75, 550}}, {32, 7, 95, {5, -1, -1, 150}},
         32, "fluid-drums/jazz", 6, 60},
        {"dance", 124, 54, 80, {-1, -1, -1, 180},
         {62, 2, 60, {2, 120, 30, 60}}, {81, 9, 35, {2, 100, 40, 60}},
         {90, 3, 55, {10, 300, 80, 200}}, {38, 5, 100, {2, -1, -1, 60}},
         24, "fluid-drums/electronic", 5, 80}};
    for (auto &entry : entries)
        for (const auto &style : styles) {
            if (entry.id != style.id) continue;
            auto &s = entry.sound;
            s.mode = PlaybackMode::All;
            s.drums = s.bass = 1;
            s.bpm = style.bpm;
            s.harpProgram = style.front;
            s.chordProgram = style.chord.program;
            s.arpProgram = style.arp.program;
            s.padProgram = style.pad.program;
            s.bassProgram = style.bass.program;
            s.frontSound = s.chordSound = s.arpSound = s.padSound = s.bassSound = "";
            s.chordPattern = style.chord.pattern;
            s.pattern = style.arp.pattern;
            s.padMotion = style.pad.pattern;
            s.bassPattern = style.bass.pattern;
            s.drumKit = style.kit;
            s.drumSound = style.kitId;
            s.drumPattern = style.drumPattern;
            s.layerMix = {style.chord.mix, style.arp.mix, style.pad.mix, style.bass.mix,
                          style.drumMix};
            s.envelopes[FrontEnvelope].values = style.frontEnvelope;
            s.envelopes[ChordEnvelope].values = style.chord.envelope;
            s.envelopes[ArpEnvelope].values = style.arp.envelope;
            s.envelopes[PadEnvelope].values = style.pad.envelope;
            s.envelopes[BassEnvelope].values = style.bass.envelope;
        }
    if (generation == Factory::Styles14) return entries;
    // 1.4 styles for the built-in bank, arranged as records are: each layer in its own
    // register and place in the stereo field, in a room of the style's size. Sounds are
    // level-matched, so a mix is the layer's level in the arrangement.
    struct Voice {
        int program, pattern, mix;
        std::array<int, 4> envelope;
        int pan, reverb, echo;
    };
    struct Arrangement {
        const char *id;
        int bpm, reverbMs;
        Voice front, chord, arp, pad, bass;
        int chordVoicing, padVoicing;
        int kit, drumPattern, drumMix, drumReverb;
    };
    // Programs: 0 Piano, 4 Electric Piano, 10 Music Box, 11 Vibraphone, 18 Organ 3,
    // 24 Nylon, 25 Steel-string, 27 Clean Electric, 29 Overdriven, 30 Distortion,
    // 32 Acoustic Bass, 33 Fingered, 34 Picked, 35 Fretless, 38 Synth Bass, 46 Harp,
    // 48 Strings, 49 Slow Strings, 62 Synth Brass, 65 Alto Sax, 68 Oboe, 73 Flute,
    // 80 Square, 81 Saw, 89 Warm Pad, 90 Polysynth, 94 Halo Pad.
    // Patterns: Chord 2 Offbeat, 3 Syncopated, 5 Sustained, 7 Bossa, 8 Pop strum,
    // 9 Chords on 2 and 3, 10 Rock eighths; Arp 2 Up/down, 4 Broken eighths, 5 Syncopated,
    // 7 Broken 3/4, 8 Bossa mixed, 9 Sixteenths; Pad 0 Steady, 1 Slow swell, 3 Pulse;
    // Bass 1 Held, 3 Eighths, 5 Offbeat, 6 Syncopated, 7 Bossa, 8 Waltz; Drums 1 Pop,
    // 2 Rock, 3 R&B, 5 Four on the floor, 6 Bossa, 7 Waltz, 9 Chimes.
    static const Arrangement arrangements[]{
        {"dream", 70, 4800,
         {11, 0, 78, {-1, -1, -1, 2600}, 0, 60, 60},
         {49, 5, 37, {600, -1, -1, 2500}, -22, 55, 0},
         {10, 2, 72, {5, -1, -1, 900}, 28, 55, 80},
         {94, 1, 73, {1200, 900, 80, 3000}, 0, 70, 0},
         {35, 1, 71, {150, -1, -1, 1200}, 0, 15, 0},
         VoicingClose, VoicingOpen, 0, 9, 100, 70},
        {"pop", 100, 1500,
         {0, 0, 73, {-1, -1, -1, 300}, 0, 22, 15},
         {25, 8, 79, {-1, -1, -1, 150}, -28, 15, 0},
         {0, 4, 67, {-1, -1, -1, 200}, 22, 20, 10},
         {49, 0, 41, {400, 800, 80, 1200}, 0, 40, 0},
         {33, 3, 93, {5, -1, -1, 80}, 0, 0, 0},
         VoicingClose, VoicingOpen, 0, 1, 98, 12},
        {"rnb", 78, 1300,
         {65, 0, 53, {-1, -1, -1, 250}, 0, 30, 20},
         {4, 3, 74, {-1, -1, -1, 350}, -15, 25, 10},
         {11, 5, 77, {5, -1, -1, 300}, 30, 30, 30},
         {89, 0, 51, {400, 800, 75, 900}, 0, 40, 0},
         {35, 6, 100, {5, -1, -1, 100}, 0, 0, 0},
         VoicingRich, VoicingOpen, 0, 3, 100, 10},
        {"rock", 120, 1100,
         {30, 0, 63, {-1, -1, -1, 200}, 0, 20, 25},
         {29, 10, 85, {3, -1, -1, 70}, -30, 10, 0},
         {27, 2, 68, {3, -1, -1, 120}, 30, 20, 20},
         {18, 0, 50, {80, 300, 80, 250}, 15, 25, 0},
         {34, 3, 94, {3, -1, -1, 60}, 0, 0, 0},
         VoicingPower, VoicingOpen, 0, 2, 92, 15},
        {"waltz", 92, 2200,
         {68, 0, 47, {-1, -1, -1, 300}, 0, 35, 10},
         {0, 9, 88, {5, -1, -1, 200}, -12, 25, 0},
         {46, 7, 73, {5, -1, -1, 400}, 25, 35, 10},
         {48, 1, 55, {300, 700, 80, 900}, 0, 45, 0},
         {32, 8, 100, {5, -1, -1, 200}, 0, 5, 0},
         VoicingClose, VoicingOpen, 40, 7, 70, 25},
        {"bossa", 132, 1300,
         {73, 0, 51, {-1, -1, -1, 250}, 0, 25, 15},
         {24, 7, 80, {5, -1, -1, 150}, -18, 15, 0},
         {11, 8, 65, {5, -1, -1, 250}, 25, 25, 20},
         {89, 0, 48, {300, 800, 75, 550}, 0, 35, 0},
         {32, 7, 96, {5, -1, -1, 150}, 0, 0, 0},
         VoicingClose, VoicingOpen, 32, 6, 95, 12},
        {"dance", 124, 1800,
         {80, 0, 68, {-1, -1, -1, 180}, 0, 20, 30},
         {62, 2, 88, {2, 120, 30, 60}, -20, 20, 10},
         {81, 9, 99, {2, 100, 40, 60}, 25, 20, 40},
         {90, 3, 84, {10, 300, 80, 200}, 0, 35, 0},
         {38, 5, 100, {2, -1, -1, 60}, 0, 0, 0},
         VoicingClose, VoicingOpen, 25, 5, 79, 8}};
    for (auto &entry : entries)
        for (const auto &a : arrangements) {
            if (entry.id != a.id) continue;
            auto &s = entry.sound;
            s.mode = PlaybackMode::All;
            s.drums = s.bass = 1;
            s.bpm = a.bpm;
            s.reverbMs = a.reverbMs;
            s.harpProgram = a.front.program;
            s.chordProgram = a.chord.program;
            s.arpProgram = a.arp.program;
            s.padProgram = a.pad.program;
            s.bassProgram = a.bass.program;
            s.frontSound = s.chordSound = s.arpSound = s.padSound = s.bassSound = "";
            s.drumSound = ""; // Built-in kits; the Studio bank has its own styles.
            s.chordPattern = a.chord.pattern;
            s.pattern = a.arp.pattern;
            s.padMotion = a.pad.pattern;
            s.bassPattern = a.bass.pattern;
            s.drumKit = a.kit;
            s.drumPattern = a.drumPattern;
            s.chordVoicing = a.chordVoicing;
            s.padVoicing = a.padVoicing;
            s.layerMix = {a.chord.mix, a.arp.mix, a.pad.mix, a.bass.mix, a.drumMix};
            s.frontMix = a.front.mix;
            const Voice *parts[]{&a.front, &a.chord, &a.arp, &a.pad, &a.bass};
            static constexpr int envelopeOf[]{FrontEnvelope, ChordEnvelope, ArpEnvelope,
                                              PadEnvelope, BassEnvelope};
            for (int n = 0; n < 5; n++) { // FxPart order: front, chord, arp, pad, bass.
                s.envelopes[envelopeOf[n]].values = parts[n]->envelope;
                s.pan[n] = parts[n]->pan;
                s.reverbSend[n] = parts[n]->reverb;
                s.echoSend[n] = parts[n]->echo;
            }
            s.pan[DrumsFx] = 0;
            s.reverbSend[DrumsFx] = a.drumReverb;
            s.echoSend[DrumsFx] = 0;
        }
    return entries;
}
} // namespace
std::vector<PresetDefinition> stockPresets(int release) {
    static constexpr Factory generations[]{Factory::Legacy,    Factory::FirstTuning,
                                           Factory::Release12, Factory::Release13,
                                           Factory::Drums14,   Factory::Styles14,
                                           Factory::Current};
    return factoryPresets(generations[std::clamp(release, 0, 6)]);
}
PresetLibrary::PresetLibrary() : entries(factoryPresets()) {}
int PresetLibrary::indexOf(const std::string &id) const {
    for (int n = 0; n < int(entries.size()); n++)
        if (entries[n].id == id) return n;
    // Retired or unknown presets open Dream, the first style.
    for (int n = 0; n < int(entries.size()); n++)
        if (entries[n].id == "dream") return n;
    return 0;
}
const PresetDefinition &PresetLibrary::current(const Settings &s) const {
    return entries.at(std::clamp(s.preset, 0, int(entries.size()) - 1));
}
std::vector<int> PresetLibrary::order() const {
    std::vector<int> result; // Presentation order; presets are found by their IDs.
    for (int n = 0; n < int(entries.size()); n++)
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
    const std::vector<std::vector<PresetDefinition>> generations{
        factoryPresets(Factory::Legacy), factoryPresets(Factory::FirstTuning),
        factoryPresets(Factory::Release12), factoryPresets(Factory::Release13),
        factoryPresets(Factory::Drums14), factoryPresets(Factory::Styles14)};
    auto stock = [&](const std::string &id, const Settings &sound) {
        for (const auto &generation : generations)
            for (const auto &old : generation)
                if (old.id == id && samePresetSettings(sound, old.sound)) return true;
        return id == "dream" && oldStockDream(sound);
    };
    for (auto &p : entries) {
        fs::path path = fs::path(dir) / "presets" / "factory" / (p.id + ".toml");
        try {
            if (fs::exists(path)) {
                auto loaded = readPreset(path.string());
                if (loaded.id != p.id)
                    throw std::runtime_error("Factory preset id changed");
                if (stock(p.id, loaded.sound)) {
                    loaded.sound = p.sound;
                    if (writable) writePreset(loaded, path.string());
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
                    if (p.factory && stock(p.id, w.sound)) w.sound = p.sound;
                    working[p.id] = w.sound;
                }
            } catch (const std::exception &e) {
                warn(e);
            }
        }
        try {
            auto t = parse(session.string());
            auto id = t["selected"].value_or(std::string());
            s.preset = indexOf(id);
        } catch (const std::exception &e) {
            warn(e);
            s.preset = indexOf("dream");
        }
        applyPresetSettings(s, resolved(s.preset));
    } else {
        // Migrate an existing setup; a clean first launch uses its TOML defaults.
        bool migrate =
            fs::exists(fs::path(dir) / "settings.conf") || !samePresetSettings(s, Settings{});
        // Before sessions, the preset was saved as a position among Harp, Dream, Keys, Organ.
        static const char *legacyIds[]{"harp", "dream", "keys", "organ"};
        s.preset = std::clamp(s.preset, 0, 3);
        const char *legacyId = legacyIds[s.preset];
        if (migrate) materialize(s); // Fills unsaved sounds from that original preset.
        s.preset = indexOf(legacyId);
        if (migrate) {
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
