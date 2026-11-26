#pragma once
#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
struct tsf;
namespace zc {
struct Envelope;
struct Settings;
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
    // Level matching: the gain that brings a melodic sound to a common loudness, so a layer's
    // mix means the same whatever it plays. 1 until measured; safe from the audio thread.
    float gain(const std::string &bank, int program) const;
    // Measure (once, on the calling thread) the sounds these settings play.
    void prepareGains(const Settings &settings);

  private:
    using Gains = std::array<std::atomic<float>, 128>;
    std::map<std::string, std::shared_ptr<Gains>> gains_; // By bank ID; fixed after load.
    void measure(const std::string &bank, int program);
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
// Short-term peak loudness (RMS over 100 ms windows, first 800 ms) of a note at middle C.
float soundfontLoudness(tsf *source, int bank, int program);
void soundfontEnvelope(tsf *synth, int channel, int pitch, int rate, const Envelope &settings);
void soundfontArticulate(tsf *synth, int channel, int pitch, int rate, bool sustained = false);
} // namespace zc
