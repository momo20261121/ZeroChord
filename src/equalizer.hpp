#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace zc {
// RBJ low/high shelves (S=1) and a broad mid peak (Q=0.7071):
// https://www.w3.org/TR/audio-eq-cookbook/
// Separate stereo state, no allocations in the render callback, exact flat bypass.
class Equalizer {
    struct Band {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        std::array<double, 2> z1{}, z2{};
        bool active = false;
        double process(double x, int channel) {
            if (!active) return x;
            double y = b0 * x + z1[channel];
            z1[channel] = b1 * x - a1 * y + z2[channel];
            z2[channel] = b2 * x - a2 * y;
            return y;
        }
    };
    std::array<Band, 3> bands_;
    std::array<int, 3> gains_{};
    int rate_ = 0;

  public:
    void reset() {
        for (auto &band : bands_) { band.z1.fill(0); band.z2.fill(0); }
    }
    void configure(std::array<int, 3> gains, int rate) {
        for (auto &gain : gains) gain = std::clamp(gain, -12, 12);
        if (gains == gains_ && rate == rate_) return;
        gains_ = gains;
        rate_ = rate;
        constexpr double pi = 3.14159265358979323846;
        constexpr double frequencies[]{250, 1000, 4000};
        for (int n = 0; n < 3; n++) {
            auto &band = bands_[n];
            band = {};
            band.active = gains[n] != 0;
            if (!band.active) continue;
            const double a = std::pow(10., gains[n] / 40.);
            const double w = 2 * pi * std::min(frequencies[n], rate * .45) / rate;
            const double c = std::cos(w), alpha = std::sin(w) / std::sqrt(2.);
            const double beta = 2 * std::sqrt(a) * alpha;
            double b0, b1, b2, a0, a1, a2;
            if (n == 0) {
                b0 = a * ((a + 1) - (a - 1) * c + beta);
                b1 = 2 * a * ((a - 1) - (a + 1) * c);
                b2 = a * ((a + 1) - (a - 1) * c - beta);
                a0 = (a + 1) + (a - 1) * c + beta;
                a1 = -2 * ((a - 1) + (a + 1) * c);
                a2 = (a + 1) + (a - 1) * c - beta;
            } else if (n == 1) {
                b0 = 1 + alpha * a; b1 = -2 * c; b2 = 1 - alpha * a;
                a0 = 1 + alpha / a; a1 = -2 * c; a2 = 1 - alpha / a;
            } else {
                b0 = a * ((a + 1) + (a - 1) * c + beta);
                b1 = -2 * a * ((a - 1) + (a + 1) * c);
                b2 = a * ((a + 1) + (a - 1) * c - beta);
                a0 = (a + 1) - (a - 1) * c + beta;
                a1 = 2 * ((a - 1) - (a + 1) * c);
                a2 = (a + 1) - (a - 1) * c - beta;
            }
            band.b0 = b0 / a0; band.b1 = b1 / a0; band.b2 = b2 / a0;
            band.a1 = a1 / a0; band.a2 = a2 / a0;
        }
    }
    float process(float sample, int channel) {
        double value = sample;
        for (auto &band : bands_) value = band.process(value, channel);
        return float(value);
    }
};
} // namespace zc
