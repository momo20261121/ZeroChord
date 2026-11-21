#define TSF_IMPLEMENTATION
#include <tsf.h>
#include "renderer.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace zc {
struct Renderer::Impl {
    tsf* synth=nullptr;
    int rate;
    Settings settings;
    struct Voice { uint32_t id=0; int pitch=0; uint64_t off=0, age=0; bool held=false; Part part=Part::Harp; };
    std::array<Voice,64> voices{};
    uint64_t frame=0;
    std::vector<float> delay;
    size_t delayPos=0;
    bool muted=true;
    explicit Impl(int r):rate(r),delay(size_t(r*0.23)*2,0.f) {}
    ~Impl() { if(synth) tsf_close(synth); }
};
Renderer::Renderer(const std::string& path,int rate):impl_(new Impl(rate)) {
    auto& s=*impl_; s.synth=tsf_load_filename(path.c_str());
    if(!s.synth) throw std::runtime_error("Cannot load sound bank: "+path);
    tsf_set_output(s.synth,TSF_STEREO_INTERLEAVED,rate,-17.f);
    tsf_set_max_voices(s.synth,96);
    for(int i=0;i<64;i++) { tsf_channel_set_presetnumber(s.synth,i,0,0); tsf_channel_set_volume(s.synth,i,1.f); }
}
Renderer::~Renderer()=default;
void Renderer::send(const Command& c) {
    auto& s=*impl_;
    if(c.type==CommandType::Panic) {
        for(int i=0;i<64;i++) { tsf_channel_sounds_off_all(s.synth,i); s.voices[i]={}; }
        s.muted=true; std::fill(s.delay.begin(),s.delay.end(),0.f); return;
    }
    if(c.type==CommandType::Configure) { s.settings=c.settings; return; }
    if(c.type==CommandType::Off) {
        for(int i=0;i<64;i++) if(s.voices[i].id==c.id && s.voices[i].held) { tsf_channel_note_off(s.synth,i,s.voices[i].pitch); s.voices[i].held=false; }
        return;
    }
    int slot=0;
    s.muted=false;
    for(int i=1;i<64;i++) if(s.voices[i].age<s.voices[slot].age) slot=i;
    tsf_channel_sounds_off_all(s.synth,slot);
    const auto& preset=presets[s.settings.preset];
    tsf_channel_set_presetnumber(s.synth,slot,c.part==Part::Chord?preset.chordProgram:preset.harpProgram,0);
    tsf_channel_set_volume(s.synth,slot,(c.part==Part::Chord?s.settings.chordVolume:s.settings.harpVolume)/100.f);
    tsf_channel_note_on(s.synth,slot,c.pitch,c.part==Part::Chord?0.68f:0.9f);
    s.voices[slot]={c.id,c.pitch,c.durationMs?s.frame+uint64_t(c.durationMs)*s.rate/1000:0,s.frame+1,true,c.part};
}
void Renderer::render(float* stereo,int frames) {
    auto& s=*impl_;
    for(int start=0;start<frames;start+=64) {
        int n=std::min(64,frames-start);
        for(int i=0;i<64;i++) if(s.voices[i].held && s.voices[i].off && s.voices[i].off<=s.frame) { tsf_channel_note_off(s.synth,i,s.voices[i].pitch); s.voices[i].held=false; }
        tsf_render_float(s.synth,stereo+start*2,n,0); s.frame+=n;
    }
    const float wet=s.settings.echo/100.f;
    if(s.muted) { std::fill(stereo,stereo+frames*2,0.f); return; }
    for(int i=0;i<frames*2;i++) {
        float dry=stereo[i],echo=s.delay[s.delayPos];
        s.delay[s.delayPos]=dry+echo*0.25f;
        s.delayPos=(s.delayPos+1)%s.delay.size();
        stereo[i]=std::clamp(dry+echo*wet,-1.f,1.f);
    }
}
}
