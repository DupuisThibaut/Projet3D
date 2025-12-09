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

    void renderEditor(){
        if (ImGui::CollapsingHeader("Tag Component", ImGuiTreeNodeFlags_DefaultOpen)) {
            char tagBuffer[256];
            strncpy(tagBuffer, tag.c_str(), sizeof(tagBuffer));
            tagBuffer[sizeof(tagBuffer) - 1] = '\0';
            
            if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer))) {
                tag = std::string(tagBuffer);
            }
        }
    }
};

#endif // TAG_COMPONENT_H
