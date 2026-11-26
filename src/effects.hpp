#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace zc {
// Small, allocation-free (after construction) effects for the render callback.
// Biquads follow the RBJ cookbook: https://www.w3.org/TR/audio-eq-cookbook/
struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    std::array<float, 2> z1{}, z2{};
    void reset() { z1.fill(0), z2.fill(0); }
    float process(float x, int channel) {
        const float y = b0 * x + z1[channel];
        z1[channel] = b1 * x - a1 * y + z2[channel];
        z2[channel] = b2 * x - a2 * y;
        return y;
    }
    static Biquad make(int type, double frequency, double rate, double db = 0, double q = .7071) {
        constexpr double pi = 3.14159265358979323846;
        const double w = 2 * pi * std::min(frequency, rate * .45) / rate;
        const double c = std::cos(w), alpha = std::sin(w) / (2 * q);
        double b0, b1, b2, a0, a1, a2;
        if (type == 0) { // Low-pass.
            b0 = (1 - c) / 2, b1 = 1 - c, b2 = (1 - c) / 2;
            a0 = 1 + alpha, a1 = -2 * c, a2 = 1 - alpha;
        } else if (type == 1) { // High-pass.
            b0 = (1 + c) / 2, b1 = -(1 + c), b2 = (1 + c) / 2;
            a0 = 1 + alpha, a1 = -2 * c, a2 = 1 - alpha;
        } else { // High shelf, S = 1.
            const double a = std::pow(10., db / 40.), beta = std::sqrt(2 * a) * std::sin(w);
            b0 = a * ((a + 1) + (a - 1) * c + beta);
            b1 = -2 * a * ((a - 1) + (a + 1) * c);
            b2 = a * ((a + 1) + (a - 1) * c - beta);
            a0 = (a + 1) - (a - 1) * c + beta;
            a1 = 2 * ((a - 1) - (a + 1) * c);
            a2 = (a + 1) - (a - 1) * c - beta;
        }
        Biquad f;
        f.b0 = float(b0 / a0), f.b1 = float(b1 / a0), f.b2 = float(b2 / a0);
        f.a1 = float(a1 / a0), f.a2 = float(a2 / a0);
        return f;
    }
    static Biquad lowpass(double f, double rate) { return make(0, f, rate); }
    static Biquad highpass(double f, double rate) { return make(1, f, rate); }
    static Biquad highShelf(double f, double rate, double db) { return make(2, f, rate, db); }
};

// Stereo feedback delay network: eight damped lines mixed by a Hadamard matrix, after a
// pre-delay and two allpass diffusers per side.
class Reverb {
    static constexpr int lineCount = 8;
    std::array<std::vector<float>, lineCount> lines_;
    std::array<int, lineCount> length_{}, position_{};
    std::array<float, lineCount> gain_{}, low_{};
    struct Allpass {
        std::vector<float> buffer;
        int position = 0;
        float process(float x) {
            const float delayed = buffer[position];
            const float y = delayed - .6f * x;
            buffer[position] = x + .6f * y;
            position = (position + 1) % int(buffer.size());
            return y;
        }
    };
    std::array<Allpass, 4> diffusers_;
    std::vector<float> pre_;
    int prePosition_ = 0;
    float damping_ = .5f;
    int rate_, decayMs_ = 0;

  public:
    explicit Reverb(int rate) : rate_(rate) {
        // Mutually prime lengths of 30-62 ms at 48 kHz.
        static constexpr int base[lineCount]{1427, 1693, 1949, 2111, 2333, 2557, 2767, 2963};
        for (int n = 0; n < lineCount; ++n) {
            length_[n] = std::max(64, int(base[n] * double(rate) / 48000));
            lines_[n].assign(size_t(length_[n]), 0.f);
        }
        static constexpr int diffusion[4]{142, 379, 107, 277};
        for (int n = 0; n < 4; ++n)
            diffusers_[n].buffer.assign(size_t(std::max(8, diffusion[n] * rate / 48000)), 0.f);
        pre_.assign(size_t(std::max(1, rate * 18 / 1000)) * 2, 0.f);
        damping_ = 1.f - float(std::exp(-2 * 3.14159265358979323846 * 5200. / rate));
        configure(1800);
    }
    void configure(int decayMs) {
        if (decayMs == decayMs_) return;
        decayMs_ = decayMs;
        for (int n = 0; n < lineCount; ++n)
            gain_[n] = float(std::pow(10., -3. * length_[n] / (decayMs / 1000. * rate_)));
    }
    void reset() {
        for (auto &line : lines_) std::fill(line.begin(), line.end(), 0.f);
        for (auto &d : diffusers_) std::fill(d.buffer.begin(), d.buffer.end(), 0.f);
        std::fill(pre_.begin(), pre_.end(), 0.f);
        low_.fill(0);
    }
    void process(float inLeft, float inRight, float &outLeft, float &outRight) {
        float left = pre_[size_t(prePosition_)], right = pre_[size_t(prePosition_) + 1];
        pre_[size_t(prePosition_)] = inLeft, pre_[size_t(prePosition_) + 1] = inRight;
        prePosition_ = (prePosition_ + 2) % int(pre_.size());
        left = diffusers_[1].process(diffusers_[0].process(left));
        right = diffusers_[3].process(diffusers_[2].process(right));
        std::array<float, lineCount> x{};
        for (int n = 0; n < lineCount; ++n) {
            const float out = lines_[n][size_t(position_[n])];
            low_[n] += damping_ * (out - low_[n]);
            x[n] = low_[n] * gain_[n];
        }
        outLeft = (x[0] + x[2] + x[4] + x[6]) * .5f;
        outRight = (x[1] + x[3] + x[5] + x[7]) * .5f;
        for (int size = 1; size < lineCount; size *= 2) // Fast Walsh-Hadamard transform.
            for (int n = 0; n < lineCount; n += 2 * size)
                for (int k = n; k < n + size; ++k) {
                    const float a = x[k], b = x[k + size];
                    x[k] = a + b, x[k + size] = a - b;
                }
        constexpr float norm = .35355339f; // 1 / sqrt(8) keeps the matrix lossless.
        for (int n = 0; n < lineCount; ++n) {
            float value = x[n] * norm + (n % 2 ? right : left) * .5f;
            if (std::fabs(value) < 1e-15f) value = 0.f; // No denormals in a silent tail.
            lines_[n][size_t(position_[n])] = value;
            if (++position_[n] == length_[n]) position_[n] = 0;
        }
    }
};

// Tempo-synced stereo echo with crossed, darkened feedback.
class Echo {
    std::vector<float> buffer_;
    int position_ = 0, length_ = 2, rate_;
    std::array<float, 2> low_{};

  public:
    explicit Echo(int rate) : buffer_(size_t(rate) * 2, 0.f), rate_(rate) {} // Up to 1 s.
    void setTime(int ms) { length_ = std::clamp(ms * rate_ / 1000, 1, rate_ - 1) * 2; }
    void reset() { std::fill(buffer_.begin(), buffer_.end(), 0.f), low_.fill(0); }
    void process(float inLeft, float inRight, float &outLeft, float &outRight) {
        const int size = int(buffer_.size());
        const int read = (position_ - length_ + size) % size;
        outLeft = buffer_[size_t(read)], outRight = buffer_[size_t(read) + 1];
        low_[0] += .45f * (outLeft - low_[0]);
        low_[1] += .45f * (outRight - low_[1]);
        buffer_[size_t(position_)] = inLeft + low_[1] * .35f;
        buffer_[size_t(position_) + 1] = inRight + low_[0] * .35f;
        position_ = (position_ + 2) % size;
    }
};

// Stereo-linked look-ahead peak limiter at -1 dBFS. The gain for each sample is the lowest
// any sample in the next 64 needs, released slowly and then averaged over 32 samples, so
// it is already down when a peak arrives and changes without clicks.
class Limiter {
    static constexpr int lookahead = 64, smoothing = 32;
    std::array<float, lookahead * 2> delay_{};
    int position_ = 0;
    std::array<float, lookahead + 1> windowValue_{};
    std::array<uint64_t, lookahead + 1> windowIndex_{};
    int head_ = 0, count_ = 0;
    uint64_t index_ = 0;
    std::array<float, smoothing> recent_{};
    int recentPosition_ = 0;
    double recentSum_ = smoothing;
    float released_ = 1, release_;

  public:
    static constexpr float ceiling = .89f;
    explicit Limiter(int rate) : release_(1.f - float(std::exp(-1. / (rate * .15)))) { reset(); }
    void reset() {
        delay_.fill(0), recent_.fill(1), recentSum_ = smoothing, released_ = 1;
        head_ = count_ = 0, index_ = 0, position_ = 0, recentPosition_ = 0;
    }
    float gain() const { return float(recentSum_ / smoothing); }
    void process(float &left, float &right) {
        const float peak = std::max(std::fabs(left), std::fabs(right));
        const float need = peak > ceiling ? ceiling / peak : 1.f;
        // Sliding minimum over the look-ahead window (a monotonic queue).
        constexpr int size = lookahead + 1;
        while (count_ && windowValue_[size_t((head_ + count_ - 1) % size)] >= need) --count_;
        windowValue_[size_t((head_ + count_) % size)] = need;
        windowIndex_[size_t((head_ + count_) % size)] = index_;
        ++count_;
        while (windowIndex_[size_t(head_)] + lookahead < index_) head_ = (head_ + 1) % size, --count_;
        ++index_;
        const float hold = windowValue_[size_t(head_)];
        released_ = hold < released_ ? hold : released_ + (hold - released_) * release_;
        recentSum_ += released_ - recent_[size_t(recentPosition_)];
        recent_[size_t(recentPosition_)] = released_;
        recentPosition_ = (recentPosition_ + 1) % smoothing;
        const float gain = float(recentSum_ / smoothing);
        const float l = delay_[size_t(position_) * 2], r = delay_[size_t(position_) * 2 + 1];
        delay_[size_t(position_) * 2] = left, delay_[size_t(position_) * 2 + 1] = right;
        position_ = (position_ + 1) % lookahead;
        left = std::clamp(l * gain, -1.f, 1.f), right = std::clamp(r * gain, -1.f, 1.f);
    }
};
} // namespace zc
