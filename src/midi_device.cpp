#include "midi_device.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <vector>
namespace zc {
namespace {
// "midiC1D0" is card 1, device 0; -1 for any other name.
int cardOf(const std::string &file) {
    int card = -1, device = -1;
    char tail = 0;
    if (std::sscanf(file.c_str(), "midiC%dD%d%c", &card, &device, &tail) != 2) return -1;
    return card;
}
// The card's name from /proc/asound/cards: " 1 [Keystation ]: USB-Audio - Keystation 49".
std::string cardName(int card) {
    std::ifstream in("/proc/asound/cards");
    std::string line;
    while (std::getline(in, line)) {
        int number = -1;
        if (std::sscanf(line.c_str(), "%d", &number) != 1 || number != card) continue;
        const auto dash = line.find(" - ");
        if (dash == std::string::npos) break;
        auto name = line.substr(dash + 3);
        while (!name.empty() && name.back() == ' ') name.pop_back();
        if (!name.empty()) return name;
    }
    return "MIDI keyboard";
}
} // namespace
struct MidiInput::Impl {
    std::string directory = "/dev/snd";
    struct Port {
        std::string file, name;
        int fd = -1;
        MidiParser parser;
    };
    std::vector<Port> ports;
    void close(size_t n) {
        ::close(ports[n].fd);
        ports.erase(ports.begin() + long(n));
    }
};
MidiInput::MidiInput() : impl_(new Impl) {
    if (const char *dir = std::getenv("ZEROCHORD_MIDI_DIR"); dir && *dir) impl_->directory = dir;
}
MidiInput::~MidiInput() {
    while (!impl_->ports.empty()) impl_->close(0);
}
void MidiInput::scan() {
    auto &s = *impl_;
    DIR *dir = opendir(s.directory.c_str());
    if (!dir) return;
    while (auto *entry = readdir(dir)) {
        const std::string file = entry->d_name;
        const int card = cardOf(file);
        if (card < 0) continue;
        if (std::any_of(s.ports.begin(), s.ports.end(), [&](const auto &p) { return p.file == file; }))
            continue;
        // A port another program holds open fails with EBUSY; try again on the next scan.
        const int fd = open((s.directory + "/" + file).c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;
        Impl::Port port;
        port.file = file;
        port.name = cardName(card);
        port.fd = fd;
        s.ports.push_back(std::move(port));
    }
    closedir(dir);
    std::sort(s.ports.begin(), s.ports.end(),
              [](const auto &a, const auto &b) { return a.file < b.file; });
}
bool MidiInput::poll(const std::function<void(const MidiEvent &)> &out) {
    auto &s = *impl_;
    bool kept = true;
    uint8_t buffer[256];
    for (size_t n = 0; n < s.ports.size();) {
        auto &port = s.ports[n];
        bool lost = false;
        for (;;) {
            const ssize_t got = read(port.fd, buffer, sizeof buffer);
            if (got > 0) {
                port.parser.feed(buffer, size_t(got), out);
                continue;
            }
            // 0 is a FIFO with no writer yet; EAGAIN is no data. Anything else is an unplug.
            if (got < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) lost = true;
            break;
        }
        if (lost) {
            s.close(n);
            kept = false;
        } else ++n;
    }
    return kept;
}
std::string MidiInput::name() const {
    const auto &ports = impl_->ports;
    if (ports.empty()) return "";
    // One keyboard can have several ports; count other keyboards by name.
    std::vector<std::string> names;
    for (const auto &p : ports)
        if (std::find(names.begin(), names.end(), p.name) == names.end()) names.push_back(p.name);
    return names.size() == 1 ? names[0] : names[0] + " +" + std::to_string(names.size() - 1);
}
} // namespace zc
