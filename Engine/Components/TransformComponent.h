#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <glm/glm.hpp>

struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 localMatrix = glm::mat4(1.0f);
    glm::mat4 worldMatrix = glm::mat4(1.0f);

    uint32_t parent = UINT32_MAX;
    std::vector<uint32_t> children;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId){
        if(entityData["entities"][entityId].contains("transform")){
            position = glm::vec3(entityData["entities"][entityId]["transform"]["position"][0],
                                entityData["entities"][entityId]["transform"]["position"][1],
                                entityData["entities"][entityId]["transform"]["position"][2]);
            rotation = glm::vec3(entityData["entities"][entityId]["transform"]["rotation"][0],
                                entityData["entities"][entityId]["transform"]["rotation"][1],
                                entityData["entities"][entityId]["transform"]["rotation"][2]);
            scale = glm::vec3(entityData["entities"][entityId]["transform"]["scale"][0],
                                entityData["entities"][entityId]["transform"]["scale"][1],
                                entityData["entities"][entityId]["transform"]["scale"][2]);
            if(entityData["entities"][entityId]["transform"].contains("parent")){
                parent = entityData["entities"][entityId]["transform"]["parent"];
            }
            if(entityData["entities"][entityId]["transform"].contains("children")){
                for (const auto& childId : entityData["entities"][entityId]["transform"]["children"]) {
                    children.push_back(childId);
                }
            }
        }
        
    }

    void renderEditor(){
        if(ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Transform Component");
            ImGui::Separator();
            ImGui::DragFloat3("Position", &position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &rotation.x, 1.0f);
            ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.001f, 100.0f);
            if (ImGui::Button("Reset Transform")) {
                position = glm::vec3(0.0f);
                rotation = glm::vec3(0.0f);
                scale = glm::vec3(1.0f);
            }
        }
    }

    json toJson(){
        nlohmann::json j;
        j["position"] = {position.x, position.y, position.z};
        j["rotation"] = {rotation.x, rotation.y, rotation.z};
        j["scale"] = {scale.x, scale.y, scale.z};
        if(parent != UINT32_MAX){
            j["parent"] = parent;
        }
        j["children"] = children;
        return j;
    }
};

#endif // TRANSFORM_COMPONENT_H
