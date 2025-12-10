#ifndef LAYER_COMPONENT_H
#define LAYER_COMPONENT_H

#include <string>

struct LayerComponent {
    std::string name;
    int id;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId){
        if(entityData["entities"][entityId].contains("layer")){
            name = entityData["entities"][entityId]["layer"]["name"];
            id = entityData["entities"][entityId]["layer"]["id"];
        }
    }

    void renderEditor(){
            char layerNameBuffer[256];
            strncpy(layerNameBuffer, name.c_str(), sizeof(layerNameBuffer));
            layerNameBuffer[sizeof(layerNameBuffer) - 1] = '\0';

            if (ImGui::InputText("Layer Name", layerNameBuffer, sizeof(layerNameBuffer))) {
                name = std::string(layerNameBuffer);
            }
            ImGui::InputInt("Layer ID", &id);
    }

    json toJson(){
        nlohmann::json j;
        j["name"] = name;
        j["id"] = id;
        return j;
    }
};

#endif // LAYER_COMPONENT_H
