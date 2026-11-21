#include "instrument.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#define CHECK(x) do { if(!(x)) { std::cerr<<__LINE__<<": " #x "\n"; std::exit(1); } } while(0)
using namespace zc;
struct Capture : Sink { std::vector<Command> events; void send(const Command& c) override { events.push_back(c); } };
int main() {
    for(int tonic=0;tonic<12;tonic++) for(int scale=0;scale<2;scale++) {
        CHECK(rootFor(tonic,scale,6)==48+tonic);
        for(int col=0;col<7;col++) for(unsigned mask=1;mask<8;mask++) {
            Chord c{rootFor(tonic,scale,col),-1,col,qualityFor(mask)}; auto harp=harpNotes(c);
            for(int i=1;i<7;i++) CHECK(harp[i]>harp[i-1]);
        }
    }
    Capture sink; Instrument i(sink);
    i.key('8',true,1); CHECK(chordName(i.chord)=="C");
    auto count=sink.events.size(); i.key('8',true,2); CHECK(sink.events.size()==count);
    i.key('j',true,3); CHECK(chordName(i.chord)=="Cmaj7");
    i.key('u',true,4); CHECK(chordName(i.chord)=="Caug");
    i.key('j',false,5); i.key('u',false,6); CHECK(chordName(i.chord)=="Caug");
    i.key('8',false,7); CHECK(i.active);
    i.key('1',true,8); i.key('q',true,9); CHECK(chordName(i.chord)=="Ebm");
    i.key('1',false,10); CHECK(chordName(i.chord)=="Ebm");
    i.panic(); i.key('8',true,20); i.key('5',true,21); CHECK(chordName(i.chord)=="C/G");
    CHECK(chordNotes(i.chord)[0]<chordNotes(i.chord)[1]);
    i.panic(); i.key('8',true,30); i.key('8',false,31);
    sink.events.clear(); i.key('l',true,40); i.tick(40); i.tick(250);
    std::vector<int> up; for(auto c:sink.events) if(c.type==CommandType::On) up.push_back(c.pitch);
    CHECK(up.size()==7); CHECK(std::is_sorted(up.begin(),up.end()));
    sink.events.clear(); i.key('k',true,300); i.tick(300); i.tick(510);
    std::vector<int> down; for(auto c:sink.events) if(c.type==CommandType::On) down.push_back(c.pitch);
    CHECK(down.size()==7); for(int n=0;n<7;n++) CHECK(down[n]==up[6-n]);
    i.key('l',false,520); i.key('l',true,530); i.panic(); sink.events.clear(); i.tick(1000); CHECK(sink.events.empty());
    i.hold=false; i.key('8',true,1100); i.key('8',false,1101); CHECK(!i.active);
    i.hold=true; i.key('8',true,1200); i.key('o',true,1201); sink.events.clear(); i.tick(1201); CHECK(sink.events.size()==1);
    i.tick(1202); CHECK(sink.events.size()==1); i.key(' ',true,1300); CHECK(!i.active&&!i.arp);
    std::cout<<"PASS: 12 tonics, two scales, seven qualities; combinations, release, slash, strums, repeat, hold, arp and panic\n";
}
