#pragma once
#include "music.hpp"
#include <cstdint>
#include <vector>

namespace zc {
enum class Part { Chord, Harp };
enum class CommandType { On, Off, Panic, Configure };
struct Command {
    CommandType type = CommandType::Panic;
    uint32_t id = 0;
    int pitch = 0, durationMs = 0;
    Part part = Part::Harp;
    Settings settings;
    Command(CommandType t=CommandType::Panic,uint32_t handle=0,int note=0,int duration=0,Part p=Part::Harp)
        :type(t),id(handle),pitch(note),durationMs(duration),part(p) {}
};
struct Sink { virtual ~Sink() = default; virtual void send(const Command&) = 0; };
class Instrument {
public:
    explicit Instrument(Sink& sink);
    Settings settings;
    Chord chord;
    bool active = false, hold = true, arp = false;
    int harpFlash = -1;
    uint64_t harpFlashUntil = 0;
    void key(char key, bool down, uint64_t now);
    void tick(uint64_t now);
    void panic();
    void configure();
    void clearKeys();
private:
    Sink& sink_;
    bool pressed_[128]{};
    int baseColumn_ = -1, arpStep_ = 0;
    uint32_t nextId_ = 0;
    uint64_t nextArp_ = 0, lastTap_ = 0;
    std::vector<uint32_t> chordVoices_;
    struct Pending { uint64_t at; int pitch, index; };
    std::vector<Pending> pending_;
    void select(int column, uint64_t now);
    void soundChord();
    void stopChord();
    void pluck(int pitch, int index, uint64_t now);
    unsigned mask(int column) const;
    bool anyChordKey() const;
};
}
