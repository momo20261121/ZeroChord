#include "looper.hpp"
#include "presets.hpp"
#include "settings_io.hpp"
#include "sounds.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
namespace zc {
namespace {
constexpr uint32_t loopNoteIds = 0x40000000u; // Loop notes; the engine's own IDs stay below.
int priority(LoopEvent::Kind kind) {
    switch (kind) {
    case LoopEvent::Style: return 0;
    case LoopEvent::ChordOff:
    case LoopEvent::ChordOn: return 1;
    case LoopEvent::NoteOff: return 2;
    case LoopEvent::NoteOn: return 3;
    }
    return 3;
}
bool sameChord(const LoopChord &a, const LoopChord &b) {
    return a.chord == b.chord && a.chord.column == b.chord.column && a.mask == b.mask &&
           a.sharp == b.sharp && a.bassColumn == b.bassColumn;
}
// What the loop's engine needs to sound like the style: its own layers, plus the app's levels.
bool sameSound(const Settings &a, const Settings &b) {
    return static_cast<const PerformanceSettings &>(a) == static_cast<const PerformanceSettings &>(b) &&
           a.chordVolume == b.chordVolume && a.harpVolume == b.harpVolume &&
           a.masterVolume == b.masterVolume && a.eqDb == b.eqDb && a.echo == b.echo &&
           a.output == b.output && a.preset == b.preset;
}
} // namespace
const char *loopLayerName(int layer) {
    return layer == 0 ? "Front" : modeName(backingLayerOrder[size_t(std::clamp(layer, 1, 5) - 1)]);
}
std::vector<LoopEvent> cleanLoop(const std::vector<LoopEvent> &events, double length,
                                 int shortTicks, int snapTicks) {
    struct Segment {
        double start;
        bool on;
        LoopChord chord;
    };
    std::vector<Segment> segments;
    std::vector<LoopEvent> out;
    for (const auto &e : events) {
        if (e.kind == LoopEvent::ChordOn) segments.push_back({e.tick, true, e.chord});
        else if (e.kind == LoopEvent::ChordOff) {
            if (!segments.empty() && segments.back().on) segments.push_back({e.tick, false, {}});
        } else if (e.tick < length)
            out.push_back(e);
    }
    // Every decision uses the raw durations, so one chord's fate never shifts another's.
    const size_t count = segments.size();
    std::vector<double> duration(count);
    for (size_t n = 0; n < count; ++n)
        duration[n] = (n + 1 < count ? segments[n + 1].start : length) - segments[n].start;
    std::vector<bool> keep(count, true);
    // A chord struck just before the loop's end belongs to the next pass, which opens with its
    // own first chord.
    if (count > 1 && segments.back().on && duration.back() < shortTicks) keep.back() = false;
    for (size_t n = 0; n + 1 < count; ++n) {
        if (duration[n] >= shortTicks) continue;
        if (segments[n].on && segments[n + 1].on) {
            keep[n] = false; // Joins the next chord, which starts when this one did.
            segments[n + 1].start = segments[n].start;
        } else if (!segments[n].on && n > 0 && segments[n + 1].on) {
            keep[n] = false; // A short gap between chords: the earlier chord rings on.
        }
    }
    std::vector<Segment> kept;
    for (size_t n = 0; n < count; ++n) {
        if (!keep[n]) continue;
        auto segment = segments[n];
        if (snapTicks > 0) segment.start = std::round(segment.start / snapTicks) * snapTicks;
        if (segment.start >= length) continue; // The next pass starts with its own first chord.
        // Rounding keeps the order; changes landing together keep only the later one.
        while (!kept.empty() && kept.back().start >= segment.start) kept.pop_back();
        kept.push_back(segment);
    }
    // A gap that closed can leave the same chord twice in a row: it simply rings on.
    std::vector<Segment> merged;
    for (const auto &segment : kept) {
        if (!merged.empty() && merged.back().on == segment.on &&
            (!segment.on || sameChord(merged.back().chord, segment.chord)))
            continue;
        if (merged.empty() && !segment.on) continue; // Every pass starts in silence anyway.
        merged.push_back(segment);
    }
    for (const auto &segment : merged) {
        LoopEvent e;
        e.kind = segment.on ? LoopEvent::ChordOn : LoopEvent::ChordOff;
        e.tick = segment.start;
        e.chord = segment.chord;
        out.push_back(e);
    }
    std::stable_sort(out.begin(), out.end(), [](const LoopEvent &a, const LoopEvent &b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return priority(a.kind) < priority(b.kind);
    });
    return out;
}
void writeLoop(const LoopTake &take, std::ostream &out) {
    out << "zerochord-loop 1\nbars " << take.bars << "\nseed " << take.seed << "\nmix";
    for (int v : take.level) out << ' ' << v;
    for (int v : take.on) out << ' ' << v;
    out << '\n';
    for (const auto &style : take.styles) {
        out << "style\n";
        writeSettings(style, out);
        out << "end\n";
    }
    out.precision(17);
    for (const auto &e : take.events) {
        const auto &c = e.chord;
        out << "e " << int(e.kind) << ' ' << e.tick << ' ' << c.chord.root << ' ' << c.chord.bass
            << ' ' << c.chord.column << ' ' << int(c.chord.quality) << ' ' << c.mask << ' '
            << c.sharp << ' ' << c.bassColumn << ' ' << e.note << ' ' << e.pitch << ' '
            << e.durationMs << ' ' << int(e.part) << ' ' << e.gain << ' ' << int(e.source) << ' '
            << e.index << ' ' << e.shift << ' ' << e.style << '\n';
    }
}
bool readLoop(LoopTake &take, std::istream &in) {
    LoopTake t;
    std::string line;
    if (!std::getline(in, line) || line != "zerochord-loop 1") return false;
    while (std::getline(in, line)) {
        std::istringstream row(line);
        std::string name;
        row >> name;
        if (name == "bars") row >> t.bars;
        else if (name == "seed") row >> t.seed;
        else if (name == "mix") {
            for (int &v : t.level) row >> v;
            for (int &v : t.on) row >> v;
        } else if (name == "style") {
            std::stringstream block;
            while (std::getline(in, line) && line != "end") block << line << '\n';
            Settings s;
            readSettings(s, block);
            t.styles.push_back(s);
        } else if (name == "e") {
            LoopEvent e;
            int kind = 0, quality = 0, part = 0, source = 0, sharp = 0;
            auto &c = e.chord;
            row >> kind >> e.tick >> c.chord.root >> c.chord.bass >> c.chord.column >> quality >>
                c.mask >> sharp >> c.bassColumn >> e.note >> e.pitch >> e.durationMs >> part >>
                e.gain >> source >> e.index >> e.shift >> e.style;
            if (!row || kind < 0 || kind > LoopEvent::Style || quality < 0 || quality >= qualityCount ||
                part < 0 || part > int(Part::Bass) || source < 0 || source > int(LoopNoteSource::Midi))
                return false;
            e.kind = LoopEvent::Kind(kind);
            c.chord.quality = Quality(quality);
            c.chord.column = std::clamp(c.chord.column, 0, 6);
            c.bassColumn = std::clamp(c.bassColumn, -1, 6);
            c.sharp = sharp != 0;
            e.part = Part(part);
            e.source = LoopNoteSource(source);
            e.index = std::clamp(e.index, 0, allFrontKeyCount - 1);
            e.style = std::max(0, e.style);
            t.events.push_back(e);
        }
    }
    if (t.bars < 1 || t.bars > 64 || t.styles.empty()) return false;
    for (const auto &e : t.events)
        if (e.style >= int(t.styles.size())) return false;
    for (int &v : t.level) v = v < 0 ? -1 : std::min(v, 100);
    for (int &v : t.on) v = v < 0 ? -1 : v != 0;
    take = std::move(t);
    return true;
}
void Looper::SlotSink::send(const Command &command) {
    Command c = command;
    c.slot = 1;
    if (c.id) c.id |= 0x80000000u; // Loop voices never share an ID with live ones.
    out.send(c);
}
Looper::Looper(Instrument &live, Sink &sink) : live_(live), sink_(sink), engine_(sink_) {}
void Looper::load() {
    if (path.empty()) return;
    std::ifstream in(path);
    if (!in) return;
    LoopTake take;
    if (!readLoop(take, in)) return;
    take_ = std::move(take);
    state_ = LoopState::Holding;
}
void Looper::save() const {
    if (path.empty()) return;
    std::error_code error;
    if (state_ == LoopState::Empty || take_.bars == 0) {
        std::filesystem::remove(path, error);
        return;
    }
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, error);
    {
        std::ofstream out(path + ".tmp");
        writeLoop(take_, out);
        if (!out) return;
    }
    std::filesystem::rename(path + ".tmp", path, error);
}
double Looper::tickAt(uint64_t at) const {
    return position_ + (double(at) - double(clock_)) * live_.settings.bpm / 15000.;
}
void Looper::tap(uint64_t pressedAt, uint64_t now) {
    now_ = now;
    switch (state_) {
    case LoopState::Empty: state_ = LoopState::Armed; break;
    case LoopState::Armed: state_ = LoopState::Empty; break;
    case LoopState::Recording:
        tick(now);
        if (state_ == LoopState::Recording) finish(std::min(tickAt(pressedAt), position_));
        break;
    case LoopState::Holding: countIn(now); break;
    case LoopState::Counting:
    case LoopState::Playing: stop(); break;
    }
}
void Looper::discard() {
    if (state_ != LoopState::Armed && state_ != LoopState::Recording) return;
    take_ = {};
    recording_.clear();
    state_ = LoopState::Empty;
}
void Looper::clear() {
    stop();
    take_ = {};
    events_.clear();
    recording_.clear();
    state_ = LoopState::Empty;
    save();
}
void Looper::stop() {
    if (state_ != LoopState::Playing && state_ != LoopState::Counting) return;
    sounding_.clear();
    engine_.panic(); // Ends the loop's voices; live playing carries on.
    state_ = LoopState::Holding;
    position_ = 0;
}
void Looper::begin(uint64_t at, bool byChord) {
    take_ = {};
    take_.styles.push_back(live_.settings);
    heardStyle_ = live_.settings;
    take_.seed = uint32_t(at * 2654435761u) | 1u;
    recording_.clear();
    nextNote_ = 0;
    position_ = 0;
    clock_ = at;
    state_ = LoopState::Recording;
    // A chord already ringing when a note starts the take opens the loop. Either way the
    // live backing restarts on beat 1, as the loop's will.
    if (live_.active && !byChord) {
        LoopEvent e;
        e.chord = {live_.chord, live_.selectedMask(), live_.selectedSharp(), live_.bassColumn()};
        take_.events.push_back(e);
    }
    if (live_.active) live_.restartTransport(at);
}
void Looper::add(LoopEvent e) {
    if (!take_.events.empty()) e.tick = std::max(e.tick, take_.events.back().tick);
    e.tick = std::max(0., e.tick);
    take_.events.push_back(e);
}
void Looper::finish(double end) {
    // Enter a moment late still means the bar just finished: a last bar barely begun (within
    // the clean-up window) is dropped; any more of it, or an early press, makes a whole bar.
    const auto &s = live_.settings;
    const int limit = std::max(1, s.loopBars);
    const int window = std::max(loopGridTicks[size_t(s.loopShort)], loopGridTicks[size_t(s.loopSnap)]);
    const double whole = std::floor(end / 16. + 1e-9);
    const double spare = end - whole * 16.;
    take_.bars = std::clamp(int(whole) + (spare > window + 1e-9 ? 1 : 0), 1, limit);
    for (const auto &[voice, note] : recording_) { // Notes still held end with the take.
        LoopEvent e;
        e.kind = LoopEvent::NoteOff;
        e.note = note;
        e.tick = std::min(end, take_.length());
        add(e);
    }
    recording_.clear();
    state_ = LoopState::Holding;
    position_ = 0;
    save();
    live_.panic(); // The take is over: what was playing stops with it.
}
void Looper::chordChanged(bool active, const LoopChord &chord, uint64_t at) {
    if (state_ == LoopState::Armed && active) begin(at, true);
    if (state_ != LoopState::Recording) return;
    LoopEvent e;
    e.kind = active ? LoopEvent::ChordOn : LoopEvent::ChordOff;
    e.tick = tickAt(at);
    e.chord = chord;
    add(e);
}
void Looper::noteOn(uint32_t voice, LoopEvent note, uint64_t at) {
    if (state_ == LoopState::Armed) begin(at, false);
    if (state_ != LoopState::Recording) return;
    note.kind = LoopEvent::NoteOn;
    note.tick = tickAt(at);
    note.note = ++nextNote_;
    if (!note.durationMs) recording_[voice] = note.note;
    add(note);
}
void Looper::noteOff(uint32_t voice, uint64_t at) {
    auto it = recording_.find(voice);
    if (state_ != LoopState::Recording || it == recording_.end()) return;
    LoopEvent e;
    e.kind = LoopEvent::NoteOff;
    e.note = it->second;
    e.tick = tickAt(at);
    recording_.erase(it);
    add(e);
}
void Looper::allNotesOff(uint64_t at) {
    while (!recording_.empty()) noteOff(recording_.begin()->first, at);
}
void Looper::styleChanged(const Settings &settings, uint64_t at) {
    if (state_ != LoopState::Recording) return;
    if (static_cast<const PerformanceSettings &>(settings) ==
        static_cast<const PerformanceSettings &>(heardStyle_))
        return;
    heardStyle_ = settings;
    LoopEvent e;
    e.kind = LoopEvent::Style;
    e.tick = tickAt(at);
    e.style = int(take_.styles.size());
    take_.styles.push_back(settings);
    add(e);
}
Settings Looper::baseSettings() const {
    Settings s = live_.settings;
    if (s.loopStyle == 0 && !take_.styles.empty())
        applyPresetSettings(s, take_.styles[size_t(std::clamp(style_, 0, int(take_.styles.size()) - 1))]);
    s.bpm = live_.settings.bpm; // The loop always follows the current tempo.
    return s;
}
Settings Looper::effective(int style) const {
    Settings s = live_.settings;
    if (s.loopStyle == 0 && !take_.styles.empty())
        applyPresetSettings(s, take_.styles[size_t(std::clamp(style, 0, int(take_.styles.size()) - 1))]);
    if (s.loopStyle == 0 && !take_.styles.empty()) s.preset = take_.styles[0].preset;
    s.bpm = live_.settings.bpm;
    if (take_.level[0] >= 0) s.frontMix = take_.level[0];
    for (int layer = 1; layer < loopLayerCount; ++layer) {
        const auto mode = backingLayerOrder[size_t(layer - 1)];
        if (take_.level[size_t(layer)] >= 0) s.mix(mode) = take_.level[size_t(layer)];
        if (take_.on[size_t(layer)] >= 0) {
            const unsigned bit = backingLayers(mode);
            s.setEnabledLayers(take_.on[size_t(layer)] ? s.enabledLayers() | bit
                                                       : s.enabledLayers() & ~bit);
        }
    }
    s.clamp();
    return s;
}
bool Looper::layerOn(int layer) const {
    if (take_.on[size_t(layer)] >= 0) return take_.on[size_t(layer)] != 0;
    return layer == 0 || baseSettings().enabled(backingLayerOrder[size_t(layer - 1)]);
}
int Looper::layerLevel(int layer) const {
    if (take_.level[size_t(layer)] >= 0) return take_.level[size_t(layer)];
    const auto s = baseSettings();
    return layer == 0 ? s.frontMix : s.mix(backingLayerOrder[size_t(layer - 1)]);
}
bool Looper::layerEdited(int layer) const {
    return take_.on[size_t(layer)] >= 0 || take_.level[size_t(layer)] >= 0;
}
void Looper::setLayer(int layer, bool on, int level) {
    if (layer < 0 || layer >= loopLayerCount || take_.bars == 0) return;
    const auto base = baseSettings();
    const bool baseOn = layer == 0 || base.enabled(backingLayerOrder[size_t(layer - 1)]);
    const int baseLevel = layer == 0 ? base.frontMix : base.mix(backingLayerOrder[size_t(layer - 1)]);
    level = std::clamp(level, 0, 100);
    take_.on[size_t(layer)] = on == baseOn ? -1 : on;
    take_.level[size_t(layer)] = level == baseLevel ? -1 : level;
    if (!on && layer == 0) releaseNotes();
    sync(true);
    save();
}
void Looper::restoreLayer(int layer) {
    if (layer < 0 || layer >= loopLayerCount) return;
    take_.on[size_t(layer)] = take_.level[size_t(layer)] = -1;
    sync(true);
    save();
}
bool Looper::missingSounds() const {
    if (take_.styles.empty()) return false;
    const auto &s = take_.styles[0];
    for (const auto *id : {&s.chordSound, &s.padSound, &s.frontSound, &s.arpSound, &s.drumSound,
                           &s.bassSound})
        if (!id->empty() && !soundLibrary().find(*id)) return true;
    return false;
}
Chord Looper::follow(const LoopChord &c) const {
    const auto &s = live_.settings;
    if (s.loopStyle == 0) return c.chord;
    const int sharp = c.sharp ? 1 : 0, column = std::clamp(c.chord.column, 0, 6);
    Chord out{s.chordRoot(column) + sharp, -1, column, s.quality(c.mask, column)};
    if (c.bassColumn >= 0) out.bass = s.chordRoot(c.bassColumn) + sharp;
    return out;
}
int Looper::pitchFor(const LoopEvent &e) const {
    const auto &s = live_.settings;
    if (s.loopStyle == 0) return e.pitch;
    int pitch = -1;
    switch (e.source) {
    case LoopNoteSource::ChordKey:
    case LoopNoteSource::ScaleKey: {
        Settings front = s;
        front.frontMode = e.source == LoopNoteSource::ScaleKey;
        pitch = frontNotes(follow(e.chord), front)[size_t(std::clamp(e.index, 0, 6))] + e.shift;
        break;
    }
    case LoopNoteSource::AllFront: {
        const int note = allFrontNotes(s)[size_t(std::clamp(e.index, 0, allFrontKeyCount - 1))];
        pitch = note < 0 ? -1 : note + e.shift;
        break;
    }
    case LoopNoteSource::Midi: {
        // The nearest move from the take's key to the current one.
        const int from = take_.styles.empty() ? s.tonic : take_.styles[0].tonic;
        const int delta = ((s.tonic - from) % 12 + 18) % 12 - 6;
        pitch = e.pitch + delta;
        break;
    }
    }
    return pitch < 0 ? -1 : std::min(pitch, 127);
}
void Looper::releaseNotes() {
    for (const auto &[note, voice] : sounding_) sink_.send(Command{CommandType::Off, voice});
    sounding_.clear();
}
void Looper::countIn(uint64_t now) {
    const int bars = live_.settings.loopCount;
    if (take_.bars <= 0) return;
    if (bars <= 0) {
        startPlayback(now);
        return;
    }
    state_ = LoopState::Counting;
    position_ = nextClick_ = -16. * bars;
    clock_ = now;
    tick(now);
}
void Looper::startPlayback(uint64_t now) {
    // A live chord already ringing keeps the loop's beat from its first bar.
    if (take_.bars <= 0) return;
    const auto &s = live_.settings;
    events_ = cleanLoop(take_.events, take_.length(), loopGridTicks[size_t(s.loopShort)],
                        loopGridTicks[size_t(s.loopSnap)]);
    state_ = LoopState::Playing;
    position_ = 0;
    clock_ = now;
    style_ = 0;
    next_ = 0;
    carry_ = size_t(-1);
    sounding_.clear();
    engine_.panic();
    engine_.settings = effective(0);
    engine_.configure();
    engine_.reseed(take_.seed);
    engine_.restartTransport(now);
    if (live_.active) live_.restartTransport(now);
    nextSync_ = now + 50;
    tick(now);
}
void Looper::sync(bool force) {
    if (!playing()) return;
    if (!force && now_ < nextSync_) return;
    nextSync_ = now_ + 50;
    const auto s = effective(style_);
    if (!sameSound(s, engine_.settings)) engine_.adopt(s);
}
void Looper::apply(const LoopEvent &e) {
    switch (e.kind) {
    case LoopEvent::ChordOn: engine_.playChord(follow(e.chord), now_); break;
    case LoopEvent::ChordOff: engine_.releaseChord(); break;
    case LoopEvent::Style:
        if (live_.settings.loopStyle == 0) {
            style_ = e.style;
            sync(true);
        }
        break;
    case LoopEvent::NoteOff: {
        auto it = sounding_.find(e.note);
        if (it == sounding_.end()) break;
        sink_.send(Command{CommandType::Off, it->second});
        sounding_.erase(it);
        break;
    }
    case LoopEvent::NoteOn: {
        if (!layerOn(0)) break;
        const int pitch = pitchFor(e);
        if (pitch < 0) break;
        Command c{CommandType::On, loopNoteIds | (++nextVoice_ & 0x3fffffffu), pitch, e.durationMs,
                  e.part};
        c.gain = e.gain;
        c.live = e.source == LoopNoteSource::Midi;
        sink_.send(c);
        if (!e.durationMs) sounding_[e.note] = c.id;
        break;
    }
    }
}
void Looper::wrap() {
    releaseNotes();
    if (style_ != 0) {
        style_ = 0;
        sync(true);
    }
    // A chord that opens the loop and is already ringing carries over without a new attack;
    // anything else stops, as every pass starts the way the take did.
    carry_ = events_.size();
    for (size_t n = 0; n < events_.size() && events_[n].tick <= 0; ++n)
        if (events_[n].kind == LoopEvent::ChordOn) carry_ = n;
    if (carry_ < events_.size()) {
        const Chord first = follow(events_[carry_].chord);
        if (!(engine_.active && first == engine_.chord && first.column == engine_.chord.column))
            carry_ = events_.size();
    }
    if (carry_ == events_.size()) engine_.releaseChord();
    engine_.reseed(take_.seed);
    engine_.restartTransport(now_);
    next_ = 0;
}
void Looper::seek() {
    next_ = 0;
    carry_ = size_t(-1);
    const LoopEvent *chord = nullptr;
    while (next_ < events_.size() && events_[next_].tick <= position_) {
        const auto &e = events_[next_];
        if (e.kind == LoopEvent::ChordOn || e.kind == LoopEvent::ChordOff) chord = &e;
        ++next_;
    }
    if (!chord || chord->kind == LoopEvent::ChordOff) engine_.releaseChord();
    else if (!engine_.active || !(follow(chord->chord) == engine_.chord))
        engine_.playChord(follow(chord->chord), now_);
}
void Looper::refresh() {
    if (!playing()) return;
    const auto &s = live_.settings;
    events_ = cleanLoop(take_.events, take_.length(), loopGridTicks[size_t(s.loopShort)],
                        loopGridTicks[size_t(s.loopSnap)]);
    releaseNotes();
    seek();
    sync(true);
}
void Looper::tick(uint64_t now) {
    now_ = now;
    if (state_ == LoopState::Recording) {
        position_ = tickAt(now);
        clock_ = now;
        const double limit = std::max(1, live_.settings.loopBars) * 16.;
        if (position_ >= limit) finish(limit); // The limit acts as Enter.
        return;
    }
    if (state_ == LoopState::Counting) {
        position_ = tickAt(now);
        clock_ = now;
        // A side stick on each beat, beat 1 accented, from the built-in kit at a fixed level.
        while (nextClick_ < 0 && nextClick_ <= position_) {
            const bool downbeat = std::fmod(nextClick_ + 1024., 16.) == 0;
            Command c{CommandType::On, loopNoteIds | (++nextVoice_ & 0x3fffffffu), 37, 120, Part::Drums};
            c.gain = downbeat ? 1.f : .6f;
            c.click = true;
            sink_.send(c);
            nextClick_ += 4;
        }
        if (position_ >= 0) startPlayback(now); // Beat 1.
        return;
    }
    if (state_ != LoopState::Playing) return;
    position_ = tickAt(now);
    clock_ = now;
    const double length = take_.length();
    for (;;) {
        while (next_ < events_.size() && events_[next_].tick <= position_ &&
               events_[next_].tick < length) {
            if (next_ != carry_) apply(events_[next_]);
            ++next_;
        }
        if (position_ < length) break;
        position_ -= length;
        wrap();
    }
    engine_.settings.bpm = live_.settings.bpm;
    sync(false);
    engine_.tick(now);
}
} // namespace zc
