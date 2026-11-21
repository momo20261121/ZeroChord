#include "music.hpp"
#include <algorithm>

namespace zc {
const std::array<Preset, 4> presets = {{{"Harp", 48, 46}, {"Dream", 89, 10}, {"Keys", 4, 24}, {"Organ", 16, 11}}};
int columnFor(char key) {
    for (const std::string row : {"2345678", "qwertyu", "asdfghj"}) {
        auto n = row.find(key); if (n != std::string::npos) return int(n);
    }
    return -1;
}
int rowFor(char key) {
    if (std::string("2345678").find(key) != std::string::npos) return 0;
    if (std::string("qwertyu").find(key) != std::string::npos) return 1;
    if (std::string("asdfghj").find(key) != std::string::npos) return 2;
    return -1;
}
int rootFor(int tonic, int scale, int column) {
    static constexpr int intervals[2][7] = {{0,2,4,5,7,9,11}, {0,2,3,5,7,8,10}};
    return 48 + (tonic + intervals[scale != 0][(column + 1) % 7]) % 12;
}
Quality qualityFor(unsigned mask) {
    switch (mask) {
        case 2: return Quality::Minor; case 4: return Quality::Seventh;
        case 5: return Quality::Major7; case 6: return Quality::Minor7;
        case 3: return Quality::Diminished; case 7: return Quality::Augmented;
        default: return Quality::Major;
    }
}
std::array<int, 4> chordNotes(const Chord& c) {
    // The interval choices follow minichord's documented combination qualities.
    static constexpr int intervals[7][4] = {{0,4,7,12},{0,3,7,12},{0,4,7,10},{0,4,7,11},{0,3,7,10},{0,3,6,12},{0,4,8,12}};
    std::array<int,4> n{};
    for (int i=0;i<4;i++) n[i] = c.root + intervals[int(c.quality)][i];
    if (c.bass >= 0) { int bass = c.bass; while (bass >= c.root) bass -= 12; n[0] = bass; }
    return n;
}
std::array<int, 7> harpNotes(const Chord& c) {
    auto notes = chordNotes(Chord{c.root,-1,c.column,c.quality});
    const bool seventh = c.quality == Quality::Seventh || c.quality == Quality::Major7 || c.quality == Quality::Minor7;
    std::array<int,7> out{};
    for (int i=0;i<7;i++) out[i] = notes[i % (seventh ? 4 : 3)] + 12 * (1 + i / (seventh ? 4 : 3));
    return out;
}
std::string noteName(int pitch) {
    static const char* names[] = {"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
    return names[(pitch % 12 + 12) % 12];
}
std::string chordName(const Chord& c) {
    static const char* suffix[] = {"","m","7","maj7","m7","dim","aug"};
    return noteName(c.root) + suffix[int(c.quality)] + (c.bass < 0 ? "" : "/" + noteName(c.bass));
}
const char* degreeName(int column) { static const char* names[] = {"II","III","IV","V","VI","VII","I"}; return names[column]; }
void Settings::clamp() {
    tonic = std::clamp(tonic,0,11); scale = std::clamp(scale,0,1); preset = std::clamp(preset,0,3);
    chordVolume = std::clamp(chordVolume,0,100); harpVolume = std::clamp(harpVolume,0,100); echo = std::clamp(echo,0,60);
    strumMs = std::clamp(strumMs,60,600); bpm = std::clamp(bpm,40,240); pattern = std::clamp(pattern,0,2);
}
}
