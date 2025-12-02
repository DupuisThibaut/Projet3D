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
};

#endif // LAYER_COMPONENT_H
