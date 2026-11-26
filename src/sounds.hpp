#pragma once
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>
struct tsf;
namespace zc {
struct Envelope;
struct Sound {
    std::string id, name, category, bank;
    int program = 0, bankNumber = 0;
    int choice = 0;
};
class SoundLibrary {
  public:
    struct Bank {
        std::string id, path;
        std::shared_ptr<tsf> data;
    };
    SoundLibrary();
    std::vector<Sound> entries;
    std::vector<Bank> banks;
    std::string warning;
    void load(const std::string &configDirectory, const std::string &builtinPath,
              const std::vector<std::string> &extraDirectories = {});
    const Sound *find(const std::string &id) const;
    const Sound *choice(int value) const;
    std::vector<std::string> categories() const;
    std::string name(int program, const std::string &id) const;
    int selected(int program, const std::string &id) const;
    std::string sampleRateLabel(int choice) const;
    // The sound's own attack/decay (ms), sustain (%), release (ms) near middle C, if known.
    std::optional<std::array<int, 4>> envelope(int program, const std::string &id) const;
};
SoundLibrary &soundLibrary();
struct SoundFontPreset {
    std::string name;
    int bank, program;
};
std::vector<SoundFontPreset> soundfontPresets(tsf *synth);
std::vector<int> soundfontSampleRates(tsf *synth, int bank, int program);
std::optional<std::array<int, 4>> soundfontEnvelopeDefaults(tsf *synth, int bank, int program,
                                                            int key = 60);
void soundfontPanic(tsf *synth);
void soundfontEnvelope(tsf *synth, int channel, int pitch, int rate, const Envelope &settings);
void soundfontArticulate(tsf *synth, int channel, int pitch, int rate, bool sustained = false);
} // namespace zc
