#include "storage.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <utility>
#include <vector>
namespace zc {
namespace fs = std::filesystem;
std::string dataDirectory() {
    if (const char *own = std::getenv("ZEROCHORD_HOME"); own && *own) return own;
    if (const char *home = std::getenv("HOME"); home && *home)
        return (fs::path(home) / "ZeroChord").string();
    return "/tmp/zerochord";
}
std::string legacyDirectory() {
    if (const char *own = std::getenv("ZEROCHORD_HOME"); own && *own) return "";
    if (const char *xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg)
        return (fs::path(xdg) / "zerochord").string();
    if (const char *home = std::getenv("HOME"); home && *home)
        return (fs::path(home) / ".config/zerochord").string();
    return "";
}
namespace {
// Moves `from`'s entries into `to`, merging folders both have. Records each move so a
// failure can put everything back. Leaves clashing files where they are.
bool moveInto(const fs::path &from, const fs::path &to,
              std::vector<std::pair<fs::path, fs::path>> &moved) {
    std::error_code ec;
    std::vector<fs::path> entries;
    for (const auto &entry : fs::directory_iterator(from, ec)) entries.push_back(entry.path());
    if (ec) return false;
    // Settings last: until they arrive, the next launch still finds the old folder in use.
    std::stable_partition(entries.begin(), entries.end(),
                          [](const fs::path &p) { return p.filename() != "settings.conf"; });
    for (const auto &source : entries) {
        const auto target = to / source.filename();
        if (fs::exists(fs::symlink_status(target, ec))) {
            if (fs::is_directory(source, ec) && fs::is_directory(target, ec) &&
                !fs::is_symlink(source, ec) && !fs::is_symlink(target, ec) &&
                !moveInto(source, target, moved))
                return false;
            continue; // Two files of the same name: keep both, the old one where it is.
        }
        fs::rename(source, target, ec);
        if (ec) return false; // Another filesystem, or no permission.
        moved.push_back({source, target});
    }
    return true;
}
// Deletes the folders the move emptied, deepest first; anything left behind stays.
void removeEmpty(const fs::path &dir) {
    std::error_code ec;
    for (const auto &entry : fs::directory_iterator(dir, ec))
        if (entry.is_directory(ec) && !entry.is_symlink(ec)) removeEmpty(entry.path());
    if (fs::is_empty(dir, ec) && !ec) fs::remove(dir, ec);
}
} // namespace
std::string migrateData(const std::string &fromText, const std::string &toText,
                        std::string &message) {
    message.clear();
    const fs::path from(fromText), to(toText);
    std::error_code ec;
    if (fromText.empty() || !fs::is_directory(from, ec) || fs::exists(to / "settings.conf", ec) ||
        fs::equivalent(from, to, ec))
        return toText;
    fs::create_directories(to, ec);
    std::vector<std::pair<fs::path, fs::path>> moved;
    if (ec || !moveInto(from, to, moved)) {
        for (auto it = moved.rbegin(); it != moved.rend(); ++it) fs::rename(it->second, it->first, ec);
        message = "Could not move " + fromText + " to " + toText + "; using the old folder";
        return fromText;
    }
    removeEmpty(from);
    if (fs::exists(from, ec))
        message = "Moved settings to " + toText + "; files with clashing names stay in " + fromText;
    return toText;
}
} // namespace zc
