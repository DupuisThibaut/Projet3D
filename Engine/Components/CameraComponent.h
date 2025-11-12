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
};

#endif // CAMERA_COMPONENT_H
