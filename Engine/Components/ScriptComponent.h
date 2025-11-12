#ifndef SCRIPT_COMPONENT_H
#define SCRIPT_COMPONENT_H
#include "InputEvent.h"
#include <string>

struct ScriptComponent {
    virtual void onInput(const InputEvent& event) = 0;
    virtual void onUpdate(float deltaTime) = 0;

    std::string scriptName;
};

#endif // SCRIPT_COMPONENT_H
