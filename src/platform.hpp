#pragma once
#include <functional>
#include <lvgl.h>
#include <memory>
#include <optional>
#include <string>
namespace zc {
class Platform {
  public:
    explicit Platform(bool headless = false);
    ~Platform();
    bool poll(const std::function<void(char, bool)> &key, const std::function<void()> &lost);
    void capture(const std::string &path) const;
    std::optional<int> systemVolume() const;
    bool setSystemVolume(int percent) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace zc
