#include "sounds.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "music.hpp"
#include <cmath>
#include <toml.hpp>
#include <tsf.h>
namespace zc {
namespace fs = std::filesystem;
namespace {
bool slug(const std::string &s) {
    return !s.empty() && s.size() <= 48 && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    });
}
bool cleanName(const std::string &s) {
    return !s.empty() && s.size() <= 40 &&
           std::none_of(s.begin(), s.end(), [](unsigned char c) { return c < 32 || c == 127; });
}
} // namespace
SoundLibrary::SoundLibrary() {
    entries = {{"piano", "Piano", "Keys", "", 0, 0, 0},
               {"electric-piano", "Electric Piano", "Keys", "", 4, 0, 4},
               {"organ", "Organ", "Keys", "", 16, 0, 16},
               {"vibraphone", "Vibraphone", "Keys", "", 11, 0, 11},
               {"celesta", "Celesta", "Keys", "", 8, 0, 8},
               {"harp", "Harp", "Plucked", "", 46, 0, 46},
               {"nylon-guitar", "Nylon Guitar", "Plucked", "", 24, 0, 24},
               {"steel-string-guitar", "Steel-string Guitar", "Plucked", "", 25, 0, 25},
               {"clean-electric-guitar", "Clean Electric Guitar", "Plucked", "", 27, 0, 27},
               {"overdriven-guitar", "Overdriven Guitar", "Plucked", "", 29, 0, 29},
               {"music-box", "Music Box", "Plucked", "", 10, 0, 10},
               {"strings", "Strings", "Strings", "", 48, 0, 48},
               {"warm-pad", "Warm Pad", "Pads", "", 89, 0, 89},
               {"airy-pad", "Airy Pad", "Pads", "", 88, 0, 88},
               // 1.4: more General MIDI instruments from the built-in bank.
               {"clavinet", "Clavinet", "Keys", "", 7, 0, 7},
               {"marimba", "Marimba", "Keys", "", 12, 0, 12},
               {"accordion", "Accordion", "Keys", "", 21, 0, 21},
               {"acoustic-bass", "Acoustic Bass", "Plucked", "", 32, 0, 32},
               {"fingered-bass", "Fingered Bass", "Plucked", "", 33, 0, 33},
               {"banjo", "Banjo", "Plucked", "", 105, 0, 105},
               {"kalimba", "Kalimba", "Plucked", "", 108, 0, 108},
               {"violin", "Violin", "Strings", "", 40, 0, 40},
               {"cello", "Cello", "Strings", "", 42, 0, 42},
               {"pizzicato-strings", "Pizzicato Strings", "Strings", "", 45, 0, 45},
               {"slow-strings", "Slow Strings", "Strings", "", 49, 0, 49},
               {"synth-strings", "Synth Strings", "Strings", "", 50, 0, 50},
               {"choir", "Choir", "Strings", "", 52, 0, 52},
               {"flute", "Flute", "Winds", "", 73, 0, 73},
               {"clarinet", "Clarinet", "Winds", "", 71, 0, 71},
               {"alto-sax", "Alto Sax", "Winds", "", 65, 0, 65},
               {"trumpet", "Trumpet", "Winds", "", 56, 0, 56},
               {"brass-section", "Brass Section", "Winds", "", 61, 0, 61},
               {"polysynth", "Polysynth", "Pads", "", 90, 0, 90},
               {"halo-pad", "Halo Pad", "Pads", "", 94, 0, 94},
               {"sweep-pad", "Sweep Pad", "Pads", "", 95, 0, 95},
               {"square-lead", "Square Lead", "Synths", "", 80, 0, 80},
               {"saw-lead", "Saw Lead", "Synths", "", 81, 0, 81},
               {"synth-bass", "Synth Bass", "Synths", "", 38, 0, 38},
               {"synth-brass", "Synth Brass", "Synths", "", 62, 0, 62}};
}
float SoundLibrary::gain(const std::string &bank, int program) const {
    auto it = gains_.find(bank);
    if (it == gains_.end() || program < 0 || program > 127) return 1.f;
    const float value = (*it->second)[size_t(program)].load(std::memory_order_relaxed);
    return value > 0 ? value : 1.f;
}
void SoundLibrary::measure(const std::string &bank, int program) {
    auto it = gains_.find(bank);
    if (it == gains_.end() || program < 0 || program > 127) return;
    auto &slot = (*it->second)[size_t(program)];
    if (slot.load(std::memory_order_relaxed) != 0.f) return;
    int number = 0;
    if (!bank.empty())
        for (const auto &sound : entries)
            if (sound.bank == bank && sound.program == program) number = sound.bankNumber;
    for (const auto &b : banks)
        if (b.id == bank) {
            // The built-in piano's level is the reference; extremes stay within -14/+12 dB.
            constexpr float reference = .178f; // About -15 dB, the piano.
            const float loudness = soundfontLoudness(b.data.get(), number, program);
            slot.store(loudness > 0 ? std::clamp(reference / loudness, .2f, 4.f) : 1.f,
                       std::memory_order_relaxed);
            return;
        }
}
void SoundLibrary::prepareGains(const Settings &s) {
    auto use = [&](int program, const std::string &id) {
        if (auto *sound = find(id); sound && !sound->bank.empty()) {
            if (sound->category != "Drums") measure(sound->bank, sound->program);
        } else if (program >= 0)
            measure("", program);
    };
    const auto &preset = presets[std::clamp(s.preset, 0, 3)];
    use(s.harpProgram < 0 ? preset.harpProgram : s.harpProgram, s.frontSound);
    use(s.chordProgram < 0 ? preset.chordProgram : s.chordProgram, s.chordSound);
    use(s.padProgram < 0 ? preset.chordProgram : s.padProgram, s.padSound);
    use(s.arpProgram < 0 ? s.chordProgram : s.arpProgram,
        s.arpProgram < 0 ? s.chordSound : s.arpSound);
    use(s.bassProgram, s.bassSound);
}
SoundLibrary &soundLibrary() {
    static SoundLibrary library;
    return library;
}
const Sound *SoundLibrary::find(const std::string &id) const {
    auto it =
        std::find_if(entries.begin(), entries.end(), [&](const Sound &s) { return s.id == id; });
    return it == entries.end() ? nullptr : &*it;
}
const Sound *SoundLibrary::choice(int value) const {
    auto it = std::find_if(entries.begin(), entries.end(),
                           [&](const Sound &s) { return s.choice == value; });
    return it == entries.end() ? nullptr : &*it;
}
int SoundLibrary::selected(int program, const std::string &id) const {
    if (auto *s = find(id); s && !s->bank.empty())
        return s->choice;
    return program;
}
std::string SoundLibrary::name(int program, const std::string &id) const {
    if (!id.empty()) {
        if (auto *s = find(id))
            return s->name;
        return "Missing pack (fallback)";
    }
    if (auto *s = choice(program))
        return s->name;
    if (!banks.empty() && banks[0].id.empty())
        if (auto *n = tsf_bank_get_presetname(banks[0].data.get(), 0, program); n && *n)
            return n;
    return "Saved sound " + std::to_string(program + 1);
}
std::string SoundLibrary::sampleRateLabel(int value) const {
    const auto *sound = choice(value);
    const std::string bankId = sound ? sound->bank : "";
    auto bank = std::find_if(banks.begin(), banks.end(),
                             [&](const Bank &b) { return b.id == bankId; });
    if (bank == banks.end()) return "Source rate unavailable";
    auto rates = soundfontSampleRates(bank->data.get(), sound ? sound->bankNumber : 0,
                                      sound ? sound->program : value);
    if (rates.empty()) return "Source rate unavailable";
    auto khz = [](int hz) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(3) << hz / 1000.;
        std::string result = out.str();
        while (result.back() == '0') result.pop_back();
        if (result.back() == '.') result.pop_back();
        return result;
    };
    return "Source " + khz(rates.front()) +
           (rates.size() > 1 ? "-" + khz(rates.back()) : "") + " kHz" +
           (rates.size() > 1 ? " (mixed)" : "");
}
std::optional<std::array<int, 4>> SoundLibrary::envelope(int program, const std::string &id) const {
    const Sound *sound = id.empty() ? nullptr : find(id);
    if (!id.empty() && (!sound || sound->bank.empty())) sound = nullptr;
    const std::string bankId = sound ? sound->bank : "";
    auto bank = std::find_if(banks.begin(), banks.end(),
                             [&](const Bank &b) { return b.id == bankId; });
    if (bank == banks.end()) return std::nullopt;
    return soundfontEnvelopeDefaults(bank->data.get(), sound ? sound->bankNumber : 0,
                                     sound ? sound->program : program);
}
std::vector<std::string> SoundLibrary::categories() const {
    std::vector<std::string> result{"Keys", "Plucked", "Strings", "Winds", "Pads", "Synths"};
    if (std::any_of(entries.begin(), entries.end(),
                    [](const Sound &s) { return s.category == "Custom"; }))
        result.push_back("Custom");
    return result;
}
void SoundLibrary::load(const std::string &config, const std::string &builtin,
                        const std::vector<std::string> &extra) {
    *this = SoundLibrary{};
    if (!builtin.empty()) {
        if (auto *f = tsf_load_filename(builtin.c_str()))
            banks.push_back({"", builtin, {f, tsf_close}});
        else
            throw std::runtime_error("Cannot load sound bank: " + builtin);
    }
    std::vector<fs::path> directories{fs::path(config) / "sounds"};
    if (const char *home = std::getenv("HOME"))
        directories.emplace_back(fs::path(home) / "ZeroChord/sounds");
    if (const char *env = std::getenv("ZEROCHORD_SOUNDS_DIR"))
        directories.emplace_back(env);
    for (auto &dir : extra)
        directories.emplace_back(dir);
    // CardputerZero normally boots from the TF card. Also discover separately mounted cards.
    for (auto root : {"/media", "/mnt", "/run/media"}) {
        std::error_code ec;
        for (const auto &entry : fs::directory_iterator(root, ec)) {
            directories.push_back(entry.path() / "ZeroChord/sounds");
            for (const auto &child : fs::directory_iterator(entry.path(), ec))
                directories.push_back(child.path() / "ZeroChord/sounds");
        }
    }
    std::set<fs::path> visited;
    uintmax_t total = 0;
    int custom = 128;
    auto warn = [&](const std::string &s) { warning += s + "\n"; };
    for (const auto &directory : directories) {
        std::error_code ec;
        if (!fs::is_directory(directory, ec))
            continue;
        auto canonical = fs::canonical(directory, ec);
        if (ec || !visited.insert(canonical).second)
            continue;
        std::vector<fs::path> packs;
        for (const auto &entry : fs::directory_iterator(canonical, ec)) {
            if (entry.is_directory(ec))
                packs.push_back(entry.path());
            else if (entry.path().extension() == ".sf2")
                packs.push_back(entry.path());
        }
        std::sort(packs.begin(), packs.end());
        for (const auto &pack : packs)
            try {
                std::string id = pack.stem().string();
                if (!slug(id))
                    throw std::runtime_error(
                        "Use lowercase letters, numbers and hyphens for pack folder/file names");
                if (std::any_of(banks.begin(), banks.end(),
                                [&](const Bank &b) { return b.id == id; }))
                    throw std::runtime_error("Duplicate pack id: " + id);
                bool manifest = fs::is_directory(pack);
                toml::table table;
                fs::path file = pack;
                if (manifest) {
                    auto path = pack / "pack.toml";
                    if (fs::file_size(path) > 65536)
                        throw std::runtime_error("Manifest exceeds 64 KiB");
                    table = toml::parse_file(path.string());
                    std::string filename = table["file"].value_or(std::string());
                    if (filename.empty() || fs::path(filename).filename() != fs::path(filename) ||
                        fs::path(filename).extension() != ".sf2")
                        throw std::runtime_error("Pack file must name a .sf2 in this folder");
                    file = pack / filename;
                }
                auto size = fs::file_size(file);
                if (size > 16 * 1024 * 1024 || total + size > 32 * 1024 * 1024 || banks.size() >= 9)
                    throw std::runtime_error(
                        "Sound pack memory limit (16 MiB each, 32 MiB total, eight packs)");
                auto *raw = tsf_load_filename(file.c_str());
                if (!raw)
                    throw std::runtime_error("Invalid SoundFont");
                std::shared_ptr<tsf> data(raw, tsf_close);
                std::vector<Sound> sounds;
                if (manifest) {
                    auto *list = table["sounds"].as_array();
                    if (!list || list->empty() || list->size() > 64)
                        throw std::runtime_error("Pack needs 1–64 [[sounds]] entries");
                    std::set<std::string> ids;
                    for (const auto &item : *list) {
                        auto *nodeTable = item.as_table();
                        if (!nodeTable)
                            throw std::runtime_error("Each sound must be a table");
                        const auto &node = *nodeTable;
                        auto soundId = node["id"].value_or(std::string());
                        auto name = node["name"].value_or(std::string());
                        auto category = node["category"].value_or(std::string("Custom"));
                        auto program = node["program"].value<int>();
                        auto bank = node["bank"].value_or(0);
                        auto categories = this->categories();
                        categories.push_back("Custom");
                        categories.push_back("Drums"); // Kits from bank 128 for the Drums layer.
                        if (!slug(soundId) || !ids.insert(soundId).second || !cleanName(name) ||
                            std::find(categories.begin(), categories.end(), category) ==
                                categories.end() ||
                            !program || *program < 0 || *program > 127 || bank < 0 ||
                            bank > 16383 || (category == "Drums") != (bank == 128) ||
                            tsf_get_presetindex(raw, bank, *program) < 0)
                            throw std::runtime_error("Invalid sound id/name/category/bank/program");
                        sounds.push_back({id + "/" + soundId, name, category, id, *program, bank,
                                          custom + int(sounds.size())});
                    }
                } else {
                    auto presets = soundfontPresets(raw);
                    for (size_t n = 0; n < presets.size() && sounds.size() < 64; n++) {
                        const auto &p = presets[n];
                        std::string name = p.name;
                        if (!cleanName(name))
                            name = "Sound " + std::to_string(n + 1);
                        sounds.push_back({id + "/bank-" + std::to_string(p.bank) + "-program-" +
                                              std::to_string(p.program),
                                          name, "Custom", id, p.program, p.bank,
                                          custom + int(sounds.size())});
                    }
                    if (tsf_get_presetcount(raw) > 64)
                        warn(id + ": showing first 64 sounds; use pack.toml to select favorites");
                }
                if (sounds.empty() || custom + int(sounds.size()) > 384)
                    throw std::runtime_error("At most 256 custom sounds can be loaded");
                total += size;
                custom += int(sounds.size());
                banks.push_back({id, file.string(), std::move(data)});
                entries.insert(entries.end(), sounds.begin(), sounds.end());
            } catch (const std::exception &e) {
                warn(pack.filename().string() + ": " + e.what());
            }
    }
    for (const auto &bank : banks) { // Zero marks a sound not measured yet.
        auto table = std::make_shared<Gains>();
        for (auto &value : *table) value.store(0.f);
        gains_[bank.id] = table;
    }
}
} // namespace zc
