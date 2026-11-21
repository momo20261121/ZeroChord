#pragma once
#include "instrument.hpp"
#include <string>
namespace zc {
class UI {
public:
    explicit UI(Instrument& instrument):instrument_(instrument) {}
    bool settingsOpen=false;
    void key(char key,bool down);
    void draw();
private:
    Instrument& instrument_;
    int selection_=0;
    std::string previous_;
    void adjust(int direction);
};
}
