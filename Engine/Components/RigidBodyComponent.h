#ifndef RIGIDBODY_COMPONENT_H
#define RIGIDBODY_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

struct RigidBodyComponent {
    glm::vec3 gravity = glm::vec3(0.0,-9.81,0.0);
    float friction = 0.95;
    float bounce = 0.7;
    float mass = 1.0;
    glm::vec3 acceleration = glm::vec3(0,0,0);
    glm::vec3 velocity = glm::vec3(0,0,0);
};

#endif // RIGIDBODY_COMPONENT_H
