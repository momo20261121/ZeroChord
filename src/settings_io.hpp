#pragma once
#include "music.hpp"
namespace zc {
void loadSettings(Settings &settings, const std::string &path);
void saveSettings(const Settings &settings, const std::string &path);
} // namespace zc
