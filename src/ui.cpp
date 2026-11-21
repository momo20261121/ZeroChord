#include "ui.hpp"
#include <lvgl.h>
#include <sstream>
namespace zc {
namespace {
constexpr uint32_t bg=0x111923,panel=0x202d3a,text=0xe9edf0,muted=0x90a1b2,gold=0xffce75,cyan=0x73d9d0;
lv_obj_t* box(int x,int y,int w,int h,uint32_t color) {
    auto* o=lv_obj_create(lv_screen_active()); lv_obj_remove_style_all(o); lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,h); lv_obj_set_style_bg_color(o,lv_color_hex(color),0); lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0); lv_obj_set_style_radius(o,4,0); lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE); return o;
}
void label(int x,int y,int w,const std::string& value,uint32_t color=text,const lv_font_t* font=&lv_font_montserrat_10) {
    auto* o=lv_label_create(lv_screen_active()); lv_obj_remove_style_all(o); lv_obj_set_pos(o,x,y); lv_obj_set_width(o,w); lv_obj_set_style_text_font(o,font,0); lv_obj_set_style_text_color(o,lv_color_hex(color),0); lv_label_set_long_mode(o,LV_LABEL_LONG_CLIP); lv_label_set_text(o,value.c_str());
}
}
void UI::adjust(int d) {
    auto& s=instrument_.settings;
    switch(selection_) {
        case 0:s.tonic=(s.tonic+d+12)%12;break;
        case 1:s.scale=1-s.scale;break;
        case 2:s.preset=(s.preset+d+4)%4;break;
        case 3:s.chordVolume+=d*5;break;
        case 4:s.harpVolume+=d*5;break;
        case 5:s.echo+=d*5;break;
        case 6:s.strumMs+=d*30;break;
        case 7:s.bpm+=d*5;break;
        case 8:s.pattern=(s.pattern+d+3)%3;break;
    }
    instrument_.configure();
}
void UI::key(char key,bool down) {
    if(!down) return;
    if(key=='\t') { settingsOpen=!settingsOpen; instrument_.panic(); return; }
    if(key==27&&settingsOpen) { settingsOpen=false; instrument_.panic(); return; }
    if(!settingsOpen) return;
    if(key=='f'||key==17) selection_=(selection_+8)%9;
    if(key=='x'||key==18) selection_=(selection_+1)%9;
    if(key=='z'||key==19) adjust(-1);
    if(key=='c'||key==20||key=='\n') adjust(1);
}
void UI::draw() {
    const auto& i=instrument_; const auto& s=i.settings;
    std::ostringstream signature; signature<<settingsOpen<<','<<selection_<<','<<s.tonic<<','<<s.scale<<','<<s.preset<<','<<s.chordVolume<<','<<s.harpVolume<<','<<s.echo<<','<<s.strumMs<<','<<s.bpm<<','<<s.pattern<<','<<i.active<<','<<chordName(i.chord)<<','<<i.hold<<','<<i.arp<<','<<i.harpFlash;
    if(signature.str()==previous_) return; previous_=signature.str();
    lv_obj_clean(lv_screen_active()); lv_obj_remove_flag(lv_screen_active(),LV_OBJ_FLAG_SCROLLABLE); lv_obj_set_style_bg_color(lv_screen_active(),lv_color_hex(bg),0); lv_obj_set_style_bg_opa(lv_screen_active(),LV_OPA_COVER,0);
    label(8,5,142,settingsOpen?"ZeroChord / Setup":"ZeroChord",gold,&lv_font_montserrat_14);
    label(188,7,124,noteName(s.tonic)+(s.scale?" minor":" major")+"  "+presets[s.preset].name,muted);
    if(settingsOpen) {
        static const char* names[]={"Key","Scale","Sound","Chord volume","Harp volume","Echo","Strum sweep","Tempo","Arp pattern"};
        static const char* patterns[]={"Up","Down","Up / down"};
        const std::string values[]={noteName(s.tonic),s.scale?"Natural minor":"Major",presets[s.preset].name,std::to_string(s.chordVolume)+"%",std::to_string(s.harpVolume)+"%",std::to_string(s.echo)+"%",std::to_string(s.strumMs)+" ms",std::to_string(s.bpm)+" BPM",patterns[s.pattern]};
        int start=(selection_/5)*5;
        for(int n=start;n<9&&n<start+5;n++) {
            int y=29+(n-start)*22; bool selected=n==selection_;
            if(selected) box(6,y-3,308,21,panel);
            label(12,y,160,std::string(selected?"> ":"  ")+names[n],selected?gold:muted,&lv_font_montserrat_12);
            label(186,y,125,values[n],selected?text:muted,&lv_font_montserrat_12);
        }
        label(8,146,306,"F/X select   Z/C change   Tab/Esc back",muted);
        label(8,159,306,"Settings saved on exit  |  Hold Esc: home",muted);
        return;
    }
    box(8,29,84,48,panel); label(15,35,74,i.active?chordName(i.chord):"Ready",i.active?gold:text,&lv_font_montserrat_14);
    label(15,58,74,i.active?std::string(degreeName(i.chord.column))+" chord":"Select chord",muted);
    static const char* rows[]={"2345678","QWERTYU","ASDFGHJ"};
    for(int col=0;col<7;col++) {
        int x=100+col*30; bool selected=i.active&&col==i.chord.column;
        label(x+5,25,28,degreeName(col),selected?gold:muted);
        for(int row=0;row<3;row++) {
            int y=39+row*17;
            box(x,y,28,15,selected?0x3e3a30:panel);
            label(x+3,y+2,25,std::string(1,rows[row][col])+" "+(row==0?noteName(rootFor(s.tonic,s.scale,col)):(row==1?"m":"7")),selected?gold:text);
        }
    }
    label(8,90,305,std::string("I hold ")+(i.hold?"ON":"off")+"   O arp "+(i.arp?"ON":"off")+"   P "+std::to_string(s.bpm)+" BPM",muted);
    auto pitches=harpNotes(i.chord); const char* harp="ZXCVBNM";
    for(int n=0;n<7;n++) {
        int x=8+n*44; bool flash=i.harpFlash==n; box(x,107,40,29,flash?cyan:panel);
        label(x+5,110,30,std::string(1,harp[n])+" "+(i.active?noteName(pitches[n]):"-"),flash?bg:cyan);
        label(x+5,123,30,i.active?std::to_string(pitches[n]/12-1):"",flash?bg:muted);
    }
    label(8,142,306,"K down / L up   9/0 sound   1 sharp",text);
    label(8,156,306,"Tab setup   Space stop   Hold Esc: home",muted);
}
}
