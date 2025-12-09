#ifndef RIGIDBODY_COMPONENT_H
#define RIGIDBODY_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

struct RigidBodyComponent {
    glm::vec3 gravity = glm::vec3(0.0,-9.81,0.0);
    float friction = 0.9999f;
    float bounce = 0.7;
    float mass = 1.0;
    glm::vec3 acceleration = glm::vec3(0,0,0);
    glm::vec3 velocity = glm::vec3(0,0,0);
    glm::vec3 oldPosition = glm::vec3(0,0,0);
    glm::vec3 oldVelocity = glm::vec3(0,0,0);
    bool firstFrame = true;

    glm::vec3 forces = glm::vec3(0,0,0);

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId){
        if(entityData["entities"][entityId].contains("rigidbody")){
            if(entityData["entities"][entityId]["rigidbody"].contains("mass")){
                mass = entityData["entities"][entityId]["rigidbody"]["mass"].get<float>();
            }
            if(entityData["entities"][entityId]["rigidbody"].contains("gravity")){
                gravity = glm::vec3(entityData["entities"][entityId]["rigidbody"]["gravity"][0],
                                      entityData["entities"][entityId]["rigidbody"]["gravity"][1],
                                      entityData["entities"][entityId]["rigidbody"]["gravity"][2]);
            }
            if(entityData["entities"][entityId]["rigidbody"].contains("friction")){
                friction = entityData["entities"][entityId]["rigidbody"]["friction"].get<float>();
            }
            if(entityData["entities"][entityId]["rigidbody"].contains("bounce")){
                bounce = entityData["entities"][entityId]["rigidbody"]["bounce"].get<float>();
            }
        }
    }

    void renderEditor(){
        if(ImGui::CollapsingHeader("RigidBody Component")){
            ImGui::Text("RigidBody Component:");
            ImGui::DragFloat3("Gravity", &gravity[0], 0.1f);
            ImGui::DragFloat("Friction", &friction, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Bounce", &bounce, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Mass", &mass, 0.1f, 0.0f, 1000.0f);
        }
    }
};

#endif // RIGIDBODY_COMPONENT_H
