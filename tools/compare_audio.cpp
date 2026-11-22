// Controlled, offline comparison; no device or player settings are modified.
#include "renderer.hpp"
#include "sounds.hpp"
#include <tsf.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace zc;
constexpr int rate = 48000, block = 480, seconds = 9;
using Samples = std::vector<float>;
void little(std::ostream &out, unsigned n, int bytes) {
    for (int i=0; i<bytes; ++i) out.put(char((n >> (8*i)) & 255));
}
void wave(const std::filesystem::path &path, const Samples &data, double gain) {
    std::ofstream out(path, std::ios::binary);
    out << "RIFF"; little(out, unsigned(36+data.size()*2), 4); out << "WAVEfmt ";
    little(out,16,4); little(out,1,2); little(out,2,2); little(out,rate,4);
    little(out,rate*4,4); little(out,4,2); little(out,16,2);
    out << "data"; little(out,unsigned(data.size()*2),4);
    for (float x : data) little(out, unsigned(int16_t(std::clamp(x*gain,-1.,1.)*32767)),2);
}
Samples phrase(const char *bank, int program, bool native, int echo) {
    Renderer renderer(bank);
    std::unique_ptr<tsf, decltype(&tsf_close)> reference(tsf_load_filename(bank), tsf_close);
    if (!reference) throw std::runtime_error("Cannot load reference bank");
    tsf_set_output(reference.get(), TSF_STEREO_INTERLEAVED, rate, -17.f);
    tsf_channel_set_presetnumber(reference.get(),0,program,0);
    tsf_channel_set_volume(reference.get(),0,1.f);
    Command config{CommandType::Configure};
    config.settings.harpProgram = program; config.settings.echo = echo;
    config.settings.mode = PlaybackMode::FrontOnly;
    renderer.send(config);
    Samples out(rate*seconds*2);
    const int pitches[]{48,60,64,67,72,60,64,67,72};
    unsigned id = 0;
    for (int tick=0; tick<seconds*100; ++tick) {
        if (tick%100==0 && tick<500) {
            int pitch=pitches[tick/100]; ++id;
            renderer.send({CommandType::On,id,pitch,0,Part::Harp});
            tsf_channel_note_on(reference.get(),0,pitch,.9f);
        }
        if (tick%100==70 && tick<500) {
            renderer.send({CommandType::Off,id});
            tsf_channel_note_off(reference.get(),0,pitches[tick/100]);
        }
        if (tick==500) {
            for (int n=5;n<9;++n) {
                renderer.send({CommandType::On,++id,pitches[n],0,Part::Harp});
                tsf_channel_note_on(reference.get(),0,pitches[n],.9f);
            }
        }
        if (tick==650) {
            for (unsigned n=6;n<=9;++n) renderer.send({CommandType::Off,n});
            tsf_channel_note_off_all(reference.get(),0);
        }
        float *dest=out.data()+tick*block*2;
        if (native) {
            // Match the renderer's TSF call size so envelope/filter updates align.
            for (int n=0; n<block; n+=64)
                tsf_render_float(reference.get(),dest+n*2,std::min(64,block-n),0);
        } else renderer.render(dest,block);
    }
    return out;
}
int main(int argc, char **argv) {
    if (argc!=4) { std::cerr << "Usage: zerochord-compare builtin.sf2 piano.sf2 output-directory\n"; return 2; }
    try {
        std::filesystem::path folder(argv[3]); std::filesystem::create_directories(folder);
        std::vector<std::pair<std::string,Samples>> clips;
        for (int program : {0,4,46,89}) {
            auto native=phrase(argv[1],program,true,0), dry=phrase(argv[1],program,false,0);
            double error=0;
            for (size_t n=0;n<dry.size();++n) error=std::max(error,double(std::abs(native[n]-dry[n])));
            std::cout << "Program " << program << " dry/native max error: " << error << '\n';
            if (error>1e-6) throw std::runtime_error("Dry renderer differs from native TSF reference");
        }
        clips.push_back({"01-built-in-piano-dry",phrase(argv[1],0,false,0)});
        clips.push_back({"02-fluid-grand-dry",phrase(argv[2],0,false,0)});
        clips.push_back({"03-electric-piano-dry",phrase(argv[1],4,false,0)});
        clips.push_back({"04-electric-piano-echo15",phrase(argv[1],4,false,15)});
        // Match RMS using attenuation only, with one shared target and peak headroom.
        double target=1.;
        for (const auto &clip:clips) {
            double sum=0,peak=0;
            for (float x:clip.second) { sum+=x*x; peak=std::max(peak,double(std::abs(x))); }
            double rms=std::sqrt(sum/clip.second.size());
            if (!std::isfinite(rms) || rms<1e-6 || peak>=1.) throw std::runtime_error("Silent/nonfinite/clipped comparison");
            target=std::min(target,std::min(rms,.8*rms/peak));
        }
        Samples combined;
        for (const auto &clip:clips) {
            double sum=0; for (float x:clip.second) sum+=x*x;
            double gain=target/std::sqrt(sum/clip.second.size());
            wave(folder/(clip.first+".wav"),clip.second,gain);
            for (float x:clip.second) combined.push_back(float(x*gain));
            combined.insert(combined.end(),rate,0.f); // Half-second stereo silence.
        }
        wave(folder/"comparison.wav",combined,1.);
        std::unique_ptr<tsf, decltype(&tsf_close)> piano(tsf_load_filename(argv[2]),tsf_close);
        std::cout << "FluidR3 source rates:";
        for (int hz:soundfontSampleRates(piano.get(),0,0)) std::cout << ' ' << hz;
        std::cout << " Hz\nWrote matched-RMS comparisons; same notes, native envelopes, no EQ.\n";
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
