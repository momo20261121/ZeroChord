#pragma once
#include <functional>
#include <memory>
#include <string>
#include <lvgl.h>
namespace zc {
class Platform {
public:
    explicit Platform(bool headless=false);
    ~Platform();
    bool poll(const std::function<void(char,bool)>& key,const std::function<void()>& lost);
    void capture(const std::string& path) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
