#include "audio.hpp"
#include <array>
#include <atomic>
#include <miniaudio.h>
#include <stdexcept>
namespace zc {
struct Audio::Impl {
    Renderer renderer;
    ma_context context{};
    ma_device device{};
    bool contextReady = false, deviceReady = false;
    std::array<Command, 1024> queue{};
    std::atomic<unsigned> read{0}, write{0};
    std::atomic<bool> emergency{false};
    std::atomic<uint64_t> callbackCount{0}, overflowCount{0};
    explicit Impl(const std::string &path) : renderer(path) {}
    ~Impl() {
        if (deviceReady)
            ma_device_uninit(&device);
        if (contextReady)
            ma_context_uninit(&context);
    }
    static void callback(ma_device *device, void *out, const void *, ma_uint32 frames) {
        auto &self = *static_cast<Impl *>(device->pUserData);
        if (self.emergency.exchange(false)) {
            self.read.store(self.write.load(std::memory_order_acquire), std::memory_order_release);
            self.renderer.send(Command{});
        }
        unsigned r = self.read.load(std::memory_order_relaxed),
                 w = self.write.load(std::memory_order_acquire);
        while (r != w) {
            self.renderer.send(self.queue[r]);
            r = (r + 1) % self.queue.size();
        }
        self.read.store(r, std::memory_order_release);
        self.renderer.render(static_cast<float *>(out), int(frames));
        self.callbackCount++;
    }
};
Audio::Audio(const std::string &path) : impl_(new Impl(path)) {
    auto &s = *impl_;
#ifdef __linux__
    ma_backend backends[] = {ma_backend_pulseaudio};
    auto result = ma_context_init(backends, 1, nullptr, &s.context);
#else
    auto result = ma_context_init(nullptr, 0, nullptr, &s.context);
#endif
    if (result != MA_SUCCESS)
        throw std::runtime_error("Audio service unavailable (miniaudio " + std::to_string(result) +
                                 ")");
    s.contextReady = true;
    auto config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 48000;
    config.periodSizeInFrames = 256;
    config.periods = 3;
    config.dataCallback = Impl::callback;
    config.pUserData = &s;
    result = ma_device_init(&s.context, &config, &s.device);
    if (result != MA_SUCCESS)
        throw std::runtime_error("Cannot open audio output (miniaudio " + std::to_string(result) +
                                 ")");
    s.deviceReady = true;
    if (ma_device_start(&s.device) != MA_SUCCESS)
        throw std::runtime_error("Cannot start audio output");
}
Audio::~Audio() = default;
void Audio::send(const Command &c) {
    auto &s = *impl_;
    unsigned w = s.write.load(std::memory_order_relaxed), next = unsigned((w + 1) % s.queue.size());
    if (next == s.read.load(std::memory_order_acquire)) {
        s.overflowCount++;
        s.emergency = true;
        return;
    }
    s.queue[w] = c;
    s.write.store(unsigned(next), std::memory_order_release);
}
uint64_t Audio::callbacks() const { return impl_->callbackCount.load(); }
uint64_t Audio::overflows() const { return impl_->overflowCount.load(); }
} // namespace zc
