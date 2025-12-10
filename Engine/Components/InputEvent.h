#ifndef INPUTEVENT_H
#define INPUTEVENT_H
#include <string>
#include <vector>

enum class STATE {
    PRESSED,
    RELEASED,
    REPEAT
};

struct InputEvent {
    float dt;
    std::vector<std::pair<std::string, STATE>> buttons;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;
    bool mouseMoved = false;
    double scroll = 0.0;
};

#endif // INPUTEVENT_H
