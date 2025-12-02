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
};

#endif // TRANSFORM_COMPONENT_H
