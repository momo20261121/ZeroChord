#pragma once
#include <array>
#include <vector>
#include <string>

namespace zc {
enum class Quality {
    Major,
    Minor,
    Seventh,
    Major7,
    Minor7,
    Diminished,
    Augmented,
    No3,
    Sus2,
    Sus4
};
constexpr int qualityCount = 10;
constexpr int drumPatternCount = 10; // patterns(PlaybackMode::Drums)
constexpr int bassPatternCount = 9; // patterns(PlaybackMode::Bass)
// Swing: Off, swung eighths or swung 16ths; the amount runs from straight (0) to triplet (100).
enum SwingMode { SwingOff, SwingEighths, SwingSixteenths };
constexpr int swingModeCount = 3;
const char *swingModeName(int mode);
// Upper-voice choices for Chord and Pad (see voicedChord).
enum Voicing { VoicingClose, VoicingOpen, VoicingPower, VoicingRich };
constexpr int voicingCount = 4;
const char *voicingName(int voicing);
// Effect parts: pan, reverb and echo sends (see PerformanceSettings).
enum FxPart { FrontFx, ChordFx, ArpFx, PadFx, BassFx, DrumsFx };
constexpr int fxPartCount = 6;
// App output: Auto follows the headphone jack where the device reports it.
enum OutputMode { OutputAuto, OutputSpeaker, OutputHeadphones };
constexpr int chordPatternCount = 11; // patterns(PlaybackMode::Chord)
constexpr std::array<unsigned, 4> combinationMasks{3, 5, 6, 7};
int combinationIndex(unsigned mask);
const char *qualityName(Quality quality);
const char *qualityDescription(Quality quality);
struct Chord {
    int root = 48, bass = -1, column = 6;
    Quality quality = Quality::Major;
    bool operator==(const Chord &b) const {
        return root == b.root && bass == b.bass && quality == b.quality;
    }
};
int columnFor(char key);
int rowFor(char key);
// Scales: 0 major and 1 natural minor keep their saved values; seven-note scales come first
// so the chord grid can use them. Shorter scales are front-only and continue up an octave.
struct ScaleInfo {
    const char *name, *lower, *abbr; // "Harmonic minor", "harmonic minor", "hmin"
    std::array<int, 7> steps; // Semitones above the tonic; `size` entries are used.
    int size;
};
constexpr int scaleCount = 11, chordScaleCount = 8;
const ScaleInfo &scaleInfo(int scale);
// Scales in menu order; seven-note scales only when `chordOnly`.
std::vector<int> scaleMenu(bool chordOnly);
// Semitones above the tonic for a zero-based degree; short scales wrap into the next octave.
int scaleStep(int scale, int degree);
int rootFor(int tonic, int scale, int column);
Quality qualityFor(unsigned mask);
std::array<int, 4> chordNotes(const Chord &chord);
// Voiced backing notes. [0] is the root (or slash bass) below the upper voices [1]-[3]; -1 is
// silent. Upper voices sit from E above the root's octave C, moving to the nearest notes of
// `previous` (upper voices from the last chord, or empty).
std::array<int, 4> voicedChord(const Chord &chord, int voicing, const std::array<int, 4> &previous);
std::array<int, 7> harpNotes(const Chord &chord);
std::string noteName(int pitch);
bool whiteKey(int pitch); // C D E F G A B.
std::string chordName(const Chord &chord);
const char *degreeName(int column);
struct Preset {
    const char *name;
    int chordProgram, harpProgram, padProgram = -1;
};
extern const std::array<Preset, 4> presets;
// Preserve the four original persisted values; append combinations.
// Drums and Bass name layers only; saved selections (mode) stay 0-7 with separate flags.
enum class PlaybackMode { Pad, Chord, Arp, FrontOnly, ChordPad, ChordArp, PadArp, All, Drums, Bass };
unsigned backingLayers(PlaybackMode mode); // Chord=1, Pad=2, Arp=4, Drums=8, Bass=16.
PlaybackMode modeForLayers(unsigned layers);
bool layerEnabled(PlaybackMode selection, PlaybackMode layer);
const char *modeName(PlaybackMode mode);
// Menu order of the backing layers; also indexes PerformanceSettings::layerMix.
constexpr std::array<PlaybackMode, 5> backingLayerOrder{PlaybackMode::Chord, PlaybackMode::Arp,
                                                        PlaybackMode::Pad, PlaybackMode::Bass,
                                                        PlaybackMode::Drums};
constexpr unsigned allLayers = 31u;
int layerIndex(PlaybackMode layer); // Chord 0, Arp 1, Pad 2, Bass 3, Drums 4.
struct Envelope {
    // Attack/decay/release in milliseconds, sustain in percent; -1 keeps the sound default.
    std::array<int, 4> values{-1, -1, -1, -1};
    bool operator==(const Envelope &b) const { return values == b.values; }
    bool operator!=(const Envelope &b) const { return !(*this == b); }
};
enum EnvelopePart { FrontEnvelope, ChordEnvelope, PadEnvelope, ArpEnvelope, BassEnvelope };
struct PerformanceSettings {
    std::array<Envelope, 5> envelopes; // Indexed by EnvelopePart.

    PlaybackMode mode = PlaybackMode::Pad; // Unmuted layers.
    int chordProgram = -1, harpProgram = -1, padProgram = -1;
    int arpProgram = -1; // -1 copies the Chord sound when a preset is loaded.
    std::string chordSound, padSound, frontSound, arpSound;
    // Percent, in backingLayerOrder; 0 is silent. Setups from before Bass/Drums keep them at 0.
    std::array<int, 5> layerMix{100, 100, 100, 0, 0};
    int drums = 1;                 // Drums unmuted; Chord, Arp and Pad are unmuted through mode.
    int drumKit = 0, drumPattern = 0; // Kit program in drum bank 128; drum pattern index.
    std::string drumSound;         // Sound-pack kit ID; empty uses drumKit in the built-in bank.
    int bass = 1;                  // Bass unmuted.
    int bassProgram = 33, bassPattern = 0; // Fingered Bass; bass pattern index.
    std::string bassSound;
    int swingMode = SwingOff, swingAmount = 50; // Amount: 0 straight, 100 triplet.
    int chordVoicing = VoicingClose, padVoicing = VoicingOpen;
    // Indexed by FxPart. Pan -50 (left) to 50 (right); sends in percent.
    std::array<int, fxPartCount> pan{0, -12, 22, 0, 0, 0};
    std::array<int, fxPartCount> reverbSend{25, 18, 30, 40, 0, 12};
    std::array<int, fxPartCount> echoSend{100, 25, 60, 0, 0, 0};
    int reverbMs = 1800; // Reverb decay time (RT60).
    int frontMix = 100;  // Front level in this style, like a layer mix.
    unsigned enabledLayers() const {
        return backingLayers(mode) | (drums ? 8u : 0u) | (bass ? 16u : 0u);
    }
    void setEnabledLayers(unsigned layers);
    bool enabled(PlaybackMode layer) const { return enabledLayers() & backingLayers(layer); }
    int &mix(PlaybackMode layer) { return layerMix[layerIndex(layer)]; }
    int mix(PlaybackMode layer) const { return layerMix[layerIndex(layer)]; }
    // Unmuted layers with a mix above zero; Chord=1, Pad=2, Arp=4, Drums=8, Bass=16.
    unsigned audibleLayers() const;
    bool audible(PlaybackMode layer) const { return audibleLayers() & backingLayers(layer); }
    int chordPattern = 0, padMotion = 0;
    int bpm = 100, pattern = 0;
    bool operator==(const PerformanceSettings &b) const;
    bool operator!=(const PerformanceSettings &b) const { return !(*this == b); }
};
struct Settings : PerformanceSettings {
    int tonic = 0;      // Chord grid key.
    int chordScale = 0; // Seven-note scale of the chord grid roots (see scaleInfo).
    int scale = 0;      // Front scale when frontFollow is off; may be pentatonic or blues.
    int frontFollow = 1; // Scale notes follow the chord key and scale.
    int frontTonic = 0;  // Front key when frontFollow is off.
    int preset = 0; // Index into PresetLibrary::entries; 0 is Dream.
    int frontKeyTonic() const { return frontFollow ? tonic : frontTonic; }
    int frontKeyScale() const { return frontFollow ? chordScale : scale; }
    int hold = 1;
    int chordVolume = 80, harpVolume = 100, echo = 15, strumMs = 210;
    int masterVolume = 100, theme = 0;
    int output = OutputAuto;
    // MIDI keyboard: the layer whose sound it plays (MidiTarget) and its velocity curve.
    int midiTarget = 0, midiVelocity = 1;
    // All front (hold Space): every letter and number key plays the front. Scale mode uses the
    // front key and scale (shared with Front notes) from allFrontOctave; Free play is a piano-style
    // layout whose lowest white key is allFrontLowest.
    int allFrontMode = 0, allFrontOctave = 3, allFrontLowest = 53; // Scale, octave 3, F3.
    std::array<int, 3> eqDb{}; // App-wide Low / Mid / High gain, in dB.
    int chordOctave = 3;
    std::array<int, 7> degreeChordRoot{-1, -1, -1, -1, -1, -1, -1}; // -1 follows Key; otherwise absolute pitch class.
    std::array<int, 7> degreeChordOctave{-2, -2, -2, -2, -2, -2, -2}; // -2 inherits All.
    int chordRoot(int column) const;
    static constexpr int maxCombinationMs = 120;
    int combinationMs = 30; // Fixed recognition window from the first chord button.
    int frontMode = 0;                    // 0 chord notes, 1 selected key/scale
    std::array<int, 2> frontAction{0, 1}; // 0 strum, 1 octave shift, per front mode
    int scaleStrumMs = 210;
    std::array<int, 7> frontSource{-1, -1, -1, -1,
                                   -1, -1, -1}; // -1 auto, 0–3 chord tone, 4 semitone
    std::array<int, 7> frontOffset{};
    // -2 follows the original layout; other values are actual MIDI note octaves.
    std::array<int, 7> frontOctave{-2, -2, -2, -2, -2, -2, -2};
    std::array<int, 7> frontDegree{0, 1, 2, 3, 4, 5, 6};
    std::array<int, 7> scaleOctave{-2, -2, -2, -2, -2, -2, -2};
    std::array<int, 4> combinations{int(Quality::Diminished), int(Quality::Major7),
                                    int(Quality::Minor7), int(Quality::Augmented)};
    std::array<std::array<int, 4>, 7> degreeCombinations = [] {
        std::array<std::array<int, 4>, 7> values{};
        for (auto &row : values)
            row.fill(-1);
        return values;
    }();
    Quality quality(unsigned mask, int column) const;
    void clamp();
};
std::array<int, 7> frontNotes(const Chord &chord, const Settings &settings);
// All front: 28 keys in four aligned rows, top to bottom 4-0, E-O, D-L and Z-M.
constexpr int allFrontKeyCount = 28;
constexpr const char *allFrontKeys = "4567890ertyuiodfghjklzxcvbnm";
int allFrontIndex(char key); // -1 for other keys.
// Pitch of each key, or -1 for a silent one. Scale mode: one octave of the front scale per row,
// Z-M lowest; short scales continue up, as on Z-M. Free play: E-O and Z-M are white keys
// (Z-M lower), and the keys above them are their sharps, silent above B and E.
std::array<int, allFrontKeyCount> allFrontNotes(const Settings &settings, int octaveShift = 0,
                                                int semitone = 0);
} // namespace zc
