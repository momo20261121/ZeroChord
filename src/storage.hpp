#pragma once
#include <string>
namespace zc {
// Everything the player keeps: settings, presets, the loop and sound packs live in
// ~/ZeroChord/. ZEROCHORD_HOME replaces it (tests and isolated device checks).
std::string dataDirectory();
// Where ZeroChord kept its files before 2.0: $XDG_CONFIG_HOME/zerochord or
// ~/.config/zerochord. Empty when ZEROCHORD_HOME is set, so isolated runs never touch it.
std::string legacyDirectory();
// Moves the old folder's contents into `to` once, when `to` has no settings yet, then
// deletes the emptied old folder. Entries `to` already has (such as its own sound packs)
// are kept; the old copy of a clashing file stays behind, with the folder. If a move
// fails, everything moved goes back and the old folder is used. Returns the folder to use.
std::string migrateData(const std::string &from, const std::string &to, std::string &message);
} // namespace zc
