#pragma once
#include "../Components/ScriptComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/CameraComponent.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CameraController : public ScriptComponent {
    TransformComponent* transform;
    CameraComponent* camera;
    float speed = 5.0f;
    float sensitivity = 0.1f;

    void onInput(const InputEvent& event) override {
        glm::vec3 forward = glm::normalize(camera->target);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));
        double scrollY = event.scroll;

        for (auto& btn : event.buttons) {
            if (btn == "Forward") transform->position += forward * speed * event.dt;
            if (btn == "Backward") transform->position -= forward * speed * event.dt;
            if (btn == "Right") transform->position += right * speed * event.dt;
            if (btn == "Left") transform->position -= right * speed * event.dt;
            if(scrollY != 0.0f){
                if(btn == "RightMouse"){
                    sensitivity += scrollY * 0.01f;
                    if(sensitivity < 0.01f) sensitivity = 0.01f;
                    if (sensitivity > 1.0f) sensitivity = 1.0f;
                    scrollY = 0.0f;
                }
            }
        }

        if (event.mouseMoved) {
            camera->yaw   += event.mouseDeltaX * sensitivity;
            camera->pitch += event.mouseDeltaY * sensitivity;

            if (camera->pitch > 89.0f) camera->pitch = 89.0f;
            if (camera->pitch < -89.0f) camera->pitch = -89.0f;

            glm::vec3 direction;
            direction.x = cos(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
            direction.y = sin(glm::radians(camera->pitch));
            direction.z = sin(glm::radians(camera->yaw)) * cos(glm::radians(camera->pitch));
            camera->target = glm::normalize(direction);
        }

        if(scrollY != 0.0f){
            transform->position += glm::vec3(0.0f, 0.0f, -scrollY * 0.5f);
        }
    }

    void onUpdate(float deltaTime) override {
        // Ici tu peux gérer l’inertie, transitions, animations ou autres updates frame par frame
        // Si tout est event-driven, ça peut rester vide
    }
};
