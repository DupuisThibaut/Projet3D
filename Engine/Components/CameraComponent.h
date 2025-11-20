#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

struct CameraComponent {
    int id;
    glm::vec3 target = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    float fov = 45.0f;
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float yaw = -90.0f;
    float pitch = 0.0f;
    bool isActive = false;

    glm::mat4 getViewMatrix(const glm::vec3& position) const {
        return glm::lookAt(position,position + target, up);
    }

    glm::mat4 getProjectionMatrix() const {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    void updateDirection(){
        float y = glm::radians(yaw);
        float p = glm::radians(pitch);
        target.x = cos(y) * cos(p);
        target.y = sin(p);
        target.z = sin(y) * cos(p);
        target = glm::normalize(target);
    }

    void loadFromFile(const nlohmann::json& entityData, const uint32_t entityId, unsigned int width, unsigned int height) {
        if(entityData["entities"][entityId].contains("camera")){
            this->id = entityData["entities"][entityId]["camera"]["idCam"];
            if (entityData["entities"][entityId]["camera"].contains("fov")) this->fov = entityData["entities"][entityId]["camera"]["fov"];
            if (entityData["entities"][entityId]["camera"].contains("near_plane")) this->nearPlane = entityData["entities"][entityId]["camera"]["near_plane"];
            if (entityData["entities"][entityId]["camera"].contains("far_plane")) this->farPlane = entityData["entities"][entityId]["camera"]["far_plane"];
            this->aspectRatio = static_cast<float>(width) / static_cast<float>(height);
            glm::vec3 pos(0.0f);
            glm::vec3 lookAt(0.0f, 0.0f, -1.0f);
            if (entityData["entities"][entityId]["camera"].contains("target")){
                lookAt = glm::vec3(entityData["entities"][entityId]["camera"]["target"][0], entityData["entities"][entityId]["camera"]["target"][1], entityData["entities"][entityId]["camera"]["target"][2]);
            }
            if (entityData["entities"][entityId]["camera"].contains("up")){
                this->up = glm::vec3(entityData["entities"][entityId]["camera"]["up"][0], entityData["entities"][entityId]["camera"]["up"][1], entityData["entities"][entityId]["camera"]["up"][2]);
            }
            glm::vec3 dir = glm::normalize(lookAt - pos);
            if (glm::length(dir) > 1e-6f) {
                this->target = dir;
                this->yaw   = glm::degrees(atan2(dir.z, dir.x));
                this->pitch = glm::degrees(asin(glm::clamp(dir.y, -1.0f, 1.0f)));
            }
            this->updateDirection();
        }
        
    }
};

#endif // CAMERA_COMPONENT_H
