#include "sounds.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <set>
#include <stdexcept>
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
               {"harp", "Harp", "Plucked", "", 46, 0, 46},
               {"nylon-guitar", "Nylon Guitar", "Plucked", "", 24, 0, 24},
               {"music-box", "Music Box", "Plucked", "", 10, 0, 10},
               {"strings", "Strings", "Strings", "", 48, 0, 48},
               {"warm-pad", "Warm Pad", "Pads", "", 89, 0, 89},
               {"airy-pad", "Airy Pad", "Pads", "", 88, 0, 88}};
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
std::vector<std::string> SoundLibrary::categories() const {
    std::vector<std::string> result{"Keys", "Plucked", "Strings", "Pads"};
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
                        if (!slug(soundId) || !ids.insert(soundId).second || !cleanName(name) ||
                            std::find(categories.begin(), categories.end(), category) ==
                                categories.end() ||
                            !program || *program < 0 || *program > 127 || bank < 0 ||
                            bank > 16383 || tsf_get_presetindex(raw, bank, *program) < 0)
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
}
} // namespace zc
