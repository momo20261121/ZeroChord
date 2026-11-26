#include "settings_io.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
namespace zc {
namespace {
template <class F> void fields(Settings &s, F field) {
    int mode = int(s.mode);
    field("mode", mode);
    s.mode = PlaybackMode(mode);
    field("frontMode", s.frontMode);
    field("chordFrontAction", s.frontAction[0]);
    field("scaleFrontAction", s.frontAction[1]);
    field("scaleStrumMs", s.scaleStrumMs);
    field("combinationMs", s.combinationMs);
    field("chordOctave", s.chordOctave);
    for (int n = 0; n < 7; n++)
        field("degreeChordOctave" + std::to_string(n), s.degreeChordOctave[n]);
    for (int n = 0; n < 7; n++)
        field("degreeChordRoot" + std::to_string(n), s.degreeChordRoot[n]);
    for (int n = 0; n < 7; n++) {
        field("frontSource" + std::to_string(n), s.frontSource[n]);
        field("frontOffset" + std::to_string(n), s.frontOffset[n]);
        field("frontNoteOctave" + std::to_string(n), s.frontOctave[n]);
        field("frontDegree" + std::to_string(n), s.frontDegree[n]);
        field("scaleNoteOctave" + std::to_string(n), s.scaleOctave[n]);
    }
    field("hold", s.hold);
    field("masterVolume", s.masterVolume);
    field("eqLowDb", s.eqDb[0]);
    field("eqMidDb", s.eqDb[1]);
    field("eqHighDb", s.eqDb[2]);
    field("theme", s.theme);
    field("chordProgram", s.chordProgram);
    field("padProgram", s.padProgram);
    field("harpProgram", s.harpProgram);
    field("arpProgram", s.arpProgram);
    field("chordMix", s.layerMix[0]);
    field("arpMix", s.layerMix[1]);
    field("padMix", s.layerMix[2]);
    field("drumMix", s.mix(PlaybackMode::Drums));
    field("bassMix", s.mix(PlaybackMode::Bass));
    field("bass", s.bass);
    field("bassProgram", s.bassProgram);
    field("bassPattern", s.bassPattern);
    field("swingMode", s.swingMode);
    field("swingAmount", s.swingAmount);
    field("chordVoicing", s.chordVoicing);
    field("padVoicing", s.padVoicing);
    field("reverbMs", s.reverbMs);
    field("frontMix", s.frontMix);
    for (int n = 0; n < fxPartCount; n++) {
        field("pan" + std::to_string(n), s.pan[n]);
        field("reverbSend" + std::to_string(n), s.reverbSend[n]);
        field("echoSend" + std::to_string(n), s.echoSend[n]);
    }
    field("output", s.output);
    field("drums", s.drums);
    field("drumKit", s.drumKit);
    field("drumPattern", s.drumPattern);
    field("chordScale", s.chordScale);
    field("frontFollow", s.frontFollow);
    field("frontTonic", s.frontTonic);
    field("chordPattern", s.chordPattern);
    field("padMotion", s.padMotion);
    field("tonic", s.tonic);
    field("scale", s.scale);
    field("preset", s.preset);
    field("chordVolume", s.chordVolume);
    field("harpVolume", s.harpVolume);
    field("echo", s.echo);
    field("strumMs", s.strumMs);
    field("bpm", s.bpm);
    field("pattern", s.pattern);
    for (int part = 0; part < 5; part++)
        for (int control = 0; control < 4; control++)
            field("envelope" + std::to_string(part) + std::to_string(control),
                  s.envelopes[part].values[control]);
    for (int n = 0; n < 4; n++) {
        auto name = "combo" + std::to_string(combinationMasks[n]);
        field(name, s.combinations[n]);
        for (int column = 0; column < 7; column++)
            field("degree" + std::to_string(column) + name, s.degreeCombinations[column][n]);
    }
}
} // namespace
void loadSettings(Settings &s, const std::string &path) {
    std::ifstream in(path);
    std::string line;
    int version = 0, legacyRegister = 0, legacyLow = 0, legacyHigh = 127;
    std::array<int, 7> legacyOctave{};
    std::set<std::string> seen;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::string name;
        std::string text;
        if (!(row >> name >> text))
            continue;
        if (name == "padSound") {
            s.padSound = text;
            continue;
        }
        if (name == "chordSound") {
            s.chordSound = text;
            continue;
        }
        if (name == "frontSound") {
            s.frontSound = text;
            continue;
        }
        if (name == "arpSound") {
            s.arpSound = text;
            continue;
        }
        if (name == "drumSound") {
            s.drumSound = text;
            continue;
        }
        if (name == "bassSound") {
            s.bassSound = text;
            continue;
        }
        int value;
        std::istringstream number(text);
        if (!(number >> value))
            continue;
        if (name == "frontMappingVersion")
            version = value;
        seen.insert(name);
        if (name == "frontRegister")
            legacyRegister = value;
        if (name == "frontLow")
            legacyLow = value;
        if (name == "frontHigh")
            legacyHigh = value;
        for (int n = 0; n < 7; n++)
            if (name == "frontOctave" + std::to_string(n))
                legacyOctave[n] = value;
        fields(s, [&](const std::string &key, int &setting) {
            if (key == name)
                setting = value;
        });
    }
    if (version < 2) {
        // Old offsets/range become real note octaves, referenced to the selected
        // tonic. Untouched defaults retain the automatic chord-aware layout.
        Settings old = s;
        s.clamp();
        Chord c{rootFor(s.tonic, s.scale, 6), -1, 6, Quality::Major};
        auto notes = harpNotes(c);
        auto tones = chordNotes(c);
        int low = std::clamp(legacyLow, 0, 127);
        int high = std::clamp(legacyHigh, low, 127);
        for (int n = 0; n < 7; n++) {
            if (s.frontSource[n] >= 0 && s.frontSource[n] < 4)
                notes[n] = tones[s.frontSource[n]] + 12;
            if (s.frontSource[n] == 4)
                notes[n] = c.root + 12 + std::clamp(old.frontOffset[n], -24, 24);
            int pitch = std::clamp(notes[n] + 12 * (std::clamp(legacyRegister, -2, 2) +
                                                    std::clamp(legacyOctave[n], -2, 2)),
                                   low, high);
            if (s.frontSource[n] == 4)
                s.frontOffset[n] = (old.frontOffset[n] % 12 + 12) % 12;
            auto baseline = frontNotes(c, s)[n];
            if ((pitch % 12) != (baseline % 12)) {
                s.frontSource[n] = 4;
                s.frontOffset[n] = ((pitch - c.root) % 12 + 12) % 12;
            }
            if (pitch != frontNotes(c, s)[n])
                s.frontOctave[n] = pitch / 12 - 1;
        }
    }
    if (!seen.count("arpProgram")) {
        // Before 1.3, Arp played the Chord sound and envelope.
        s.arpProgram = s.chordProgram;
        s.arpSound = s.chordSound;
        s.envelopes[ArpEnvelope] = s.envelopes[ChordEnvelope];
    }
    if (!seen.count("frontFollow")) {
        // Before 1.3, one Key served both, chord roots were major, and Scale was front-only.
        s.frontTonic = s.tonic;
        s.frontFollow = s.scale == 0;
    }
    s.clamp();
}
void saveSettings(const Settings &settings, const std::string &path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent);
    std::ofstream out(path + ".tmp");
    out << "frontMappingVersion 2\n";
    Settings copy = settings;
    fields(copy, [&](const std::string &name, int &value) { out << name << ' ' << value << '\n'; });
    for (auto pair : {std::pair{"padSound", settings.padSound},
                      {"chordSound", settings.chordSound},
                      {"frontSound", settings.frontSound},
                      {"arpSound", settings.arpSound},
                      {"drumSound", settings.drumSound},
                      {"bassSound", settings.bassSound}})
        if (!pair.second.empty())
            out << pair.first << ' ' << pair.second << '\n';
    out.close();
    if (!out)
        throw std::runtime_error("Cannot save settings");
    std::filesystem::rename(path + ".tmp", path);
}
} // namespace zc
