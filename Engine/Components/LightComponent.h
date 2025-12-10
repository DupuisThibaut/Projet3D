#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include <glm/glm.hpp>

struct LightComponent {
    float intensity;

    bool update=false;
    int nb=-1;

    void loadFromFile(const nlohmann::json& entityData, uint32_t id) {
        if(entityData["entities"][id].contains("light")){
            intensity = entityData["entities"][id]["light"]["intensity"];
        }
    }

    void renderEditor(){
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f);
        }
    }

    json toJson() {
        nlohmann::json j;
        j["intensity"] = intensity;
        return j;
    }
};

#endif // LIGHT_COMPONENT_H
