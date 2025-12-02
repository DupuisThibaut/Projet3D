#ifndef TAG_COMPONENT_H
#define TAG_COMPONENT_H

#include <string>

struct TagComponent {
    std::string tag;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId){
        if(entityData["entities"][entityId].contains("tag")){
            tag = entityData["entities"][entityId]["tag"].get<std::string>();
        }
    }
};

#endif // TAG_COMPONENT_H
