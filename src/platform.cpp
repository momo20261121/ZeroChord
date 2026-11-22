#include "platform.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#if ZEROCHORD_DESKTOP
#include <SDL.h>
#else
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace zc {
struct Platform::Impl {
    bool headless;
    lv_display_t *display = nullptr;
    std::array<uint32_t, 320 * 170> pixels{};
    std::array<uint32_t, 320 * 34> drawBuffer{};
#if ZEROCHORD_DESKTOP
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;
#else
    int fb = -1, input = -1;
    fb_fix_screeninfo fix{};
    fb_var_screeninfo var{};
    uint8_t *mapped = nullptr;
    bool dropping = false;
    static char translate(unsigned code) {
        static constexpr unsigned keys[] = {
            KEY_1,  KEY_2,    KEY_3,    KEY_4,     KEY_5,        KEY_6,   KEY_7,   KEY_8,
            KEY_9,  KEY_0,    KEY_Q,    KEY_W,     KEY_E,        KEY_R,   KEY_T,   KEY_Y,
            KEY_U,  KEY_I,    KEY_O,    KEY_P,     KEY_A,        KEY_S,   KEY_D,   KEY_F,
            KEY_G,  KEY_H,    KEY_J,    KEY_K,     KEY_L,        KEY_Z,   KEY_X,   KEY_C,
            KEY_V,  KEY_B,    KEY_N,    KEY_M,     KEY_SPACE,    KEY_TAB, KEY_ESC, KEY_ENTER,
            KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_BACKSPACE};
        static const char values[] =
            "1234567890qwertyuiopasdfghjklzxcvbnm \t\x1b\n\x11\x12\x13\x14\b";
        for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
            if (keys[i] == code)
                return values[i];
        return 0;
    }
    void openInput() {
        const char *override = std::getenv("ZEROCHORD_INPUT_DEVICE");
        if (override)
            input = open(override, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        else
            input = open("/dev/input/by-path/platform-3f804000.i2c-event",
                         O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (input < 0 && !override)
            for (int n = 0; n < 32; n++) {
                auto path = "/dev/input/event" + std::to_string(n);
                int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
                if (fd < 0)
                    continue;
                std::array<unsigned char, (KEY_MAX + 8) / 8> bits{};
                ioctl(fd, EVIOCGBIT(EV_KEY, bits.size()), bits.data());
                auto has = [&](int k) { return bits[k / 8] & (1 << (k % 8)); };
                if (has(KEY_A) && has(KEY_Q) && has(KEY_SPACE)) {
                    input = fd;
                    break;
                }
                close(fd);
            }
        if (input < 0)
            throw std::runtime_error("Cannot open keyboard input");
    }
#endif
    explicit Impl(bool h) : headless(h) {}
    ~Impl() {
        if (display)
            lv_display_delete(display);
#if ZEROCHORD_DESKTOP
        if (texture)
            SDL_DestroyTexture(texture);
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        if (!headless)
            SDL_Quit();
#else
        if (mapped)
            munmap(mapped, fix.smem_len);
        if (fb >= 0)
            close(fb);
        if (input >= 0)
            close(input);
#endif
    }
    static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data) {
        auto &s = *static_cast<Impl *>(lv_display_get_user_data(display));
        auto *source = reinterpret_cast<uint32_t *>(data);
        for (int y = area->y1; y <= area->y2; y++)
            for (int x = area->x1; x <= area->x2; x++) {
                uint32_t color = *source++;
                s.pixels[size_t(y) * 320 + x] = color;
#if !ZEROCHORD_DESKTOP
                if (s.mapped) {
                    auto channel = [&](uint32_t c, const fb_bitfield &b) {
                        return ((c >> (8 - b.length)) & ((1u << b.length) - 1)) << b.offset;
                    };
                    uint32_t packed = channel((color >> 16) & 255, s.var.red) |
                                      channel((color >> 8) & 255, s.var.green) |
                                      channel(color & 255, s.var.blue);
                    size_t offset = size_t(y + s.var.yoffset) * s.fix.line_length +
                                    size_t(x + s.var.xoffset) * (s.var.bits_per_pixel / 8);
                    std::memcpy(s.mapped + offset, &packed, s.var.bits_per_pixel / 8);
                }
#endif
            }
#if ZEROCHORD_DESKTOP
        if (s.texture && lv_display_flush_is_last(display)) {
            SDL_UpdateTexture(s.texture, nullptr, s.pixels.data(), 320 * 4);
            SDL_RenderClear(s.renderer);
            SDL_RenderCopy(s.renderer, s.texture, nullptr, nullptr);
            SDL_RenderPresent(s.renderer);
        }
#endif
        lv_display_flush_ready(display);
    }
};
Platform::Platform(bool headless) : impl_(new Impl(headless)) {
    auto &s = *impl_;
    if (!headless) {
#if ZEROCHORD_DESKTOP
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
            throw std::runtime_error(SDL_GetError());
        s.window = SDL_CreateWindow("ZeroChord - desktop preview", SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED, 960, 510, SDL_WINDOW_SHOWN);
        if (!s.window)
            throw std::runtime_error(SDL_GetError());
        s.renderer = SDL_CreateRenderer(s.window, -1, SDL_RENDERER_SOFTWARE);
        if (!s.renderer)
            throw std::runtime_error(SDL_GetError());
        s.texture = SDL_CreateTexture(s.renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING, 320, 170);
        if (!s.texture)
            throw std::runtime_error(SDL_GetError());
#else
        std::vector<std::string> candidates;
        if (const char *specified = std::getenv("LV_LINUX_FBDEV_DEVICE"))
            candidates.push_back(specified);
        else {
            std::ifstream proc("/proc/fb");
            std::string line;
            while (std::getline(proc, line))
                if (line.find("st7789") != std::string::npos ||
                    line.find("panel-mipi") != std::string::npos)
                    candidates.push_back("/dev/fb" + line.substr(0, line.find(' ')));
            for (int i = 0; i < 8; i++)
                candidates.push_back("/dev/fb" + std::to_string(i));
        }
        for (const auto &path : candidates) {
            int fd = open(path.c_str(), O_RDWR | O_CLOEXEC);
            if (fd < 0)
                continue;
            fb_var_screeninfo var{};
            fb_fix_screeninfo fix{};
            bool valid = ioctl(fd, FBIOGET_VSCREENINFO, &var) == 0 &&
                         ioctl(fd, FBIOGET_FSCREENINFO, &fix) == 0;
            if (valid && var.xres == 320 && var.yres == 170 &&
                (var.bits_per_pixel == 16 || var.bits_per_pixel == 32) && var.red.length <= 8 &&
                var.green.length <= 8 && var.blue.length <= 8) {
                s.fb = fd;
                s.var = var;
                s.fix = fix;
                std::cerr << "Display: " << path << " 320x170 " << var.bits_per_pixel << "bpp\n";
                break;
            }
            close(fd);
        }
        if (s.fb < 0)
            throw std::runtime_error("No compatible 320x170 framebuffer");
        // Match LVGL's Linux fbdev startup: a previous app may have blanked it.
        // Unsupported unblank ioctls are nonfatal (some fbdev drivers omit it).
        if (ioctl(s.fb, FBIOBLANK, FB_BLANK_UNBLANK) != 0)
            std::cerr << "Framebuffer unblank: " << std::strerror(errno) << '\n';
        void *mapping = mmap(nullptr, s.fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, s.fb, 0);
        if (mapping == MAP_FAILED)
            throw std::runtime_error("Cannot map framebuffer");
        s.mapped = static_cast<uint8_t *>(mapping);
        s.openInput();
#endif
    }
    s.display = lv_display_create(320, 170);
    lv_display_set_color_format(s.display, LV_COLOR_FORMAT_XRGB8888);
    lv_display_set_user_data(s.display, &s);
    lv_display_set_buffers(s.display, s.drawBuffer.data(), nullptr, sizeof(s.drawBuffer),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s.display, Impl::flush);
}
Platform::~Platform() = default;
bool Platform::poll(const std::function<void(char, bool)> &key, const std::function<void()> &lost) {
    auto &s = *impl_;
    if (s.headless)
        return true;
#if ZEROCHORD_DESKTOP
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            return false;
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            lost();
        if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && !e.key.repeat) {
            auto sc = e.key.keysym.scancode;
            char k = 0;
            if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
                k = char('a' + sc - SDL_SCANCODE_A);
            if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
                k = char('1' + sc - SDL_SCANCODE_1);
            if (sc == SDL_SCANCODE_0)
                k = '0';
            if (sc == SDL_SCANCODE_SPACE)
                k = ' ';
            if (sc == SDL_SCANCODE_TAB)
                k = '\t';
            if (sc == SDL_SCANCODE_ESCAPE)
                k = 27;
            if (sc == SDL_SCANCODE_BACKSPACE)
                k = '\b';
            if (sc == SDL_SCANCODE_RETURN)
                k = '\n';
            if (sc == SDL_SCANCODE_UP)
                k = 17;
            if (sc == SDL_SCANCODE_DOWN)
                k = 18;
            if (sc == SDL_SCANCODE_LEFT)
                k = 19;
            if (sc == SDL_SCANCODE_RIGHT)
                k = 20;
            if (k)
                key(k, e.type == SDL_KEYDOWN);
        }
    }
#else
    input_event events[32];
    ssize_t bytes;
    while ((bytes = read(s.input, events, sizeof(events))) > 0)
        for (size_t n = 0; n < size_t(bytes) / sizeof(input_event); n++) {
            auto e = events[n];
            if (e.type == EV_SYN && e.code == SYN_DROPPED) {
                s.dropping = true;
                lost();
                continue;
            }
            if (s.dropping) {
                if (e.type == EV_SYN && e.code == SYN_REPORT)
                    s.dropping = false;
                continue;
            }
            if (e.type == EV_KEY && e.value != 2)
                if (char k = Impl::translate(e.code))
                    key(k, e.value == 1);
        }
    if (bytes == 0 || (bytes < 0 && errno != EAGAIN && errno != EINTR)) {
        lost();
        throw std::runtime_error("Keyboard disconnected");
    }
#endif
    return true;
}
void Platform::capture(const std::string &path) const {
    std::ofstream out(path, std::ios::binary);
    out << "P6\n320 170\n255\n";
    for (uint32_t c : impl_->pixels) {
        out.put(char((c >> 16) & 255));
        out.put(char((c >> 8) & 255));
        out.put(char(c & 255));
    }
    if (!out)
        throw std::runtime_error("Cannot save screenshot: " + path);
}
} // namespace zc
