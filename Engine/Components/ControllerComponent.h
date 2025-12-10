#ifndef CONTROLLER_COMPONENT_H
#define CONTROLLER_COMPONENT_H

#include <unordered_map>
#include <string>

struct ControllerComponent {
    float moveSpeed = 5.0f;
    float zoomSpeed = 0.5f;
    float sensitivity = 0.1f;

    bool rightMouseDown = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;

    TransformComponent* transform = nullptr;
    CameraComponent* camera = nullptr;


    void loadFromFile(const nlohmann::json& entityData, const uint32_t entityId) {
        if(entityData["entities"][entityId].contains("controller")){
            if (entityData["entities"][entityId]["controller"].contains("speed")) this->moveSpeed = entityData["entities"][entityId]["controller"]["speed"];
            if (entityData["entities"][entityId]["controller"].contains("zoom_speed")) this->zoomSpeed = entityData["entities"][entityId]["controller"]["zoom_speed"];
            if (entityData["entities"][entityId]["controller"].contains("sensitivity")) this->sensitivity = entityData["entities"][entityId]["controller"]["sensitivity"];
        }
    }

    void renderEditor(){
            ImGui::Text("Controller Component");
            ImGui::Separator();
            ImGui::DragFloat("Move Speed", &moveSpeed, 0.1f);
            ImGui::DragFloat("Zoom Speed", &zoomSpeed, 0.01f);
            ImGui::DragFloat("Sensitivity", &sensitivity, 0.01f);
    }

    json toJson() {
        nlohmann::json j;
        j["speed"] = moveSpeed;
        j["zoom_speed"] = zoomSpeed;
        j["sensitivity"] = sensitivity;
        return j;
    }
};


#endif // CONTROLLER_COMPONENT_H