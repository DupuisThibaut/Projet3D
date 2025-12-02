#ifndef SCRIPT_COMPONENT_H
#define SCRIPT_COMPONENT_H
#include "InputEvent.h"
#include <string>

struct ScriptComponent {
    virtual void onInput(const InputEvent& event) = 0;
    virtual void onUpdate(float deltaTime) = 0;

    std::string scriptName;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId) {
        if(entityData["entities"][entityId].contains("script")){
            if(entityData["entities"][entityId]["script"].contains("type")){
                if(entityData["entities"][entityId]["script"]["type"] == "C++" && entityData["entities"][entityId]["script"].contains("name")){
                scriptName = entityData["entities"][entityId]["script"]["name"].get<std::string>();
                }
            }
        }
    }
};

#endif // SCRIPT_COMPONENT_H
