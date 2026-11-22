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

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace zc
