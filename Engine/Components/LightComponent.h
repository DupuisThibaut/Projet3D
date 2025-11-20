#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include <glm/glm.hpp>

struct LightComponent {
    float intensity;

    void loadFromFile(const nlohmann::json& entityData, uint32_t id) {
        if(entityData["entities"][id].contains("light")){
            intensity = entityData["entities"][id]["light"]["intensity"];
        }
    }
};

#endif // LIGHT_COMPONENT_H
