#pragma once
#include "music.hpp"
#include <iosfwd>
namespace zc {
void loadSettings(Settings &settings, const std::string &path);
void saveSettings(const Settings &settings, const std::string &path);
// The same "name value" lines, for files that hold settings among other data.
void readSettings(Settings &settings, std::istream &in);
void writeSettings(const Settings &settings, std::ostream &out);
} // namespace zc
