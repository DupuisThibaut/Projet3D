#ifndef INPUTEVENT_H
#define INPUTEVENT_H
#include <string>
#include <vector>

struct InputEvent {
    float dt;
    std::vector<std::string> buttons;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;
    bool mouseMoved = false;
    double scroll = 0.0;
};

#endif // INPUTEVENT_H
