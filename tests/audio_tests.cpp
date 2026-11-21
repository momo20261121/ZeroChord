#include "renderer.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace zc;
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    try {
        Renderer r(argv[1]); std::vector<float> samples(48000*2); float peak=0;
        for(int preset=0;preset<4;preset++) {
            Command config; config.type=CommandType::Configure; config.settings.preset=preset; r.send(config);
            r.send({CommandType::On,1,60,0,Part::Chord}); r.send({CommandType::On,2,60,400,Part::Harp});
            r.render(samples.data(),48000);
            double energy=0; for(float x:samples) { if(!std::isfinite(x)) throw std::runtime_error("nonfinite audio"); energy+=x*x; peak=std::max(peak,std::abs(x)); }
            if(energy<0.0001) throw std::runtime_error("silent preset");
            r.send({CommandType::Off,2}); r.render(samples.data(),48000);
            energy=0; for(float x:samples) energy+=x*x;
            if(energy<0.0001) throw std::runtime_error("releasing overlapping harp killed chord");
            r.send(Command{}); r.render(samples.data(),48000);
            for(float x:samples) if(x!=0) throw std::runtime_error("panic left audible samples");
        }
        std::cout<<"PASS: four presets, finite nonzero audio, independent overlapping notes, panic; peak="<<peak<<"\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<"\n"; return 1; }
}
