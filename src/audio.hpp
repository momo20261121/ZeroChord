#pragma once
#include "renderer.hpp"
#include <memory>
#include <string>
namespace zc {
class Audio : public Sink {
  public:
    explicit Audio(const std::string &soundfont);
    ~Audio();
    void send(const Command &) override;
    uint64_t callbacks() const;
    uint64_t overflows() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace zc
