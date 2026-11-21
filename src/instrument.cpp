#include "instrument.hpp"
#include <algorithm>
#include <cstring>

namespace zc {
Instrument::Instrument(Sink& sink) : sink_(sink) {}
unsigned Instrument::mask(int col) const {
    static const char* rows[] = {"2345678","qwertyu","asdfghj"};
    unsigned out=0; for(int r=0;r<3;r++) if(pressed_[int(rows[r][col])]) out |= 1u<<r; return out;
}
bool Instrument::anyChordKey() const { for(int i=0;i<7;i++) if(mask(i)) return true; return false; }
void Instrument::stopChord() { for(auto id:chordVoices_) sink_.send(Command{CommandType::Off,id}); chordVoices_.clear(); }
void Instrument::soundChord() {
    stopChord(); if(arp || !active) return;
    for(int pitch:chordNotes(chord)) { auto id=++nextId_; chordVoices_.push_back(id); sink_.send(Command{CommandType::On,id,pitch,0,Part::Chord}); }
}
void Instrument::select(int column,uint64_t now) {
    if(baseColumn_<0 || !mask(baseColumn_)) baseColumn_=column;
    Chord next{rootFor(settings.tonic,settings.scale,baseColumn_) + (pressed_['1'] ? 1 : 0),-1,baseColumn_,qualityFor(mask(baseColumn_))};
    // First held column supplies the chord; the most recently pressed other column supplies bass.
    if(column != baseColumn_) next.bass=rootFor(settings.tonic,settings.scale,column) + (pressed_['1'] ? 1 : 0);
    else if(active && chord.bass>=0) {
        for(int i=0;i<7;i++) if(i!=baseColumn_ && mask(i)) next.bass=rootFor(settings.tonic,settings.scale,i)+(pressed_['1']?1:0);
    }
    chord=next; active=true; pending_.clear(); arpStep_=0; nextArp_=now; soundChord();
}
void Instrument::pluck(int pitch,int index,uint64_t now) {
    sink_.send(Command{CommandType::On,++nextId_,pitch,1600,Part::Harp}); harpFlash=index; harpFlashUntil=now+140;
}
void Instrument::key(char key,bool down,uint64_t now) {
    auto k=static_cast<unsigned char>(key); if(k>=128) return;
    if(pressed_[k]==down) return; pressed_[k]=down;
    int col=columnFor(key);
    if(!down) {
        if(col>=0 && !anyChordKey()) { baseColumn_=-1; if(!hold) { stopChord(); active=false; pending_.clear(); } }
        return;
    }
    if(col>=0) { select(col,now); return; }
    auto harp=std::string("zxcvbnm").find(key);
    if(harp!=std::string::npos && active) pluck(harpNotes(chord)[harp],int(harp),now);
    if((key=='k'||key=='l') && active) {
        pending_.clear(); auto notes=harpNotes(chord);
        for(int i=0;i<7;i++) { int n=key=='l'?i:6-i; pending_.push_back({now+uint64_t(i*settings.strumMs/6),notes[n],n}); }
    }
    if(key=='i') { hold=!hold; if(!hold&&!anyChordKey()) { stopChord(); active=false; pending_.clear(); } }
    if(key=='o') { arp=!arp; arpStep_=0; nextArp_=now; soundChord(); }
    if(key=='9'||key=='0') { settings.preset=(settings.preset+(key=='0'?1:3))%4; configure(); }
    if(key=='p') { if(lastTap_ && now>lastTap_ && now-lastTap_>=250 && now-lastTap_<=1500) settings.bpm=int(60000/(now-lastTap_)); lastTap_=now; }
    if(key==' ') panic();
}
void Instrument::tick(uint64_t now) {
    for(auto it=pending_.begin();it!=pending_.end();) {
        if(it->at<=now) { pluck(it->pitch,it->index,now); it=pending_.erase(it); } else ++it;
    }
    if(arp&&active&&now>=nextArp_) {
        auto notes=chordNotes(chord); int index=arpStep_%4;
        if(settings.pattern==1) index=3-index;
        if(settings.pattern==2) { int n=arpStep_%6; index=n<4?n:6-n; }
        sink_.send(Command{CommandType::On,++nextId_,notes[index],int(24000/settings.bpm),Part::Chord});
        arpStep_++; nextArp_=now+uint64_t(30000/settings.bpm);
    }
    if(now>=harpFlashUntil) harpFlash=-1;
}
void Instrument::clearKeys() { std::memset(pressed_,0,sizeof(pressed_)); baseColumn_=-1; }
void Instrument::panic() { sink_.send(Command{}); chordVoices_.clear(); pending_.clear(); active=false; arp=false; clearKeys(); harpFlash=-1; }
void Instrument::configure() {
    settings.clamp(); sink_.send(Command{}); chordVoices_.clear(); pending_.clear();
    Command command; command.type=CommandType::Configure; command.settings=settings; sink_.send(command); soundChord();
}
}
