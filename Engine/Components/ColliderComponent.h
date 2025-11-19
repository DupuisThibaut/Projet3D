#ifndef COLLIDER_COMPONENT_H
#define COLLIDER_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

enum class ColliderType {
    BOX,
    SPHERE,
    MESH
};

struct ColliderComponent {
    ColliderType type = ColliderType::SPHERE;
    bool isTrigger = false;
};

#endif // COLLIDER_COMPONENT_H
