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
};


#endif // CONTROLLER_COMPONENT_H