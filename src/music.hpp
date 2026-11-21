#pragma once
#include <array>
#include <string>

namespace zc {
enum class Quality { Major, Minor, Seventh, Major7, Minor7, Diminished, Augmented };
struct Chord {
    int root = 48, bass = -1, column = 6;
    Quality quality = Quality::Major;
    bool operator==(const Chord& b) const { return root == b.root && bass == b.bass && quality == b.quality; }
};
int columnFor(char key);
int rowFor(char key);
int rootFor(int tonic, int scale, int column);
Quality qualityFor(unsigned mask);
std::array<int, 4> chordNotes(const Chord& chord);
std::array<int, 7> harpNotes(const Chord& chord);
std::string noteName(int pitch);
std::string chordName(const Chord& chord);
const char* degreeName(int column);
struct Preset { const char* name; int chordProgram, harpProgram; };
extern const std::array<Preset, 4> presets;
struct Settings {
    int tonic = 0, scale = 0, preset = 0;
    int chordVolume = 65, harpVolume = 85, echo = 15;
    int strumMs = 210, bpm = 100, pattern = 0;
    void clamp();
};
}
