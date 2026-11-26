#pragma once
#include "instrument.hpp"
#include <memory>
#include <string>

namespace zc {
class Renderer {
  public:
    explicit Renderer(const std::string &soundfont, int rate = 48000);
    ~Renderer();
    void send(const Command &command);
    void render(float *stereo, int frames);
    // Level before the limiter since the last call: peak and mean square, for level tests.
    struct Meter {
        float peak = 0;
        double meanSquare = 0;
    };
    Meter meter();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace zc
