#ifndef LIGHT_COMPONENT_H
#define LIGHT_COMPONENT_H

#include <glm/glm.hpp>

struct LightComponent {
    float intensity;

    bool update=false;
    int nb=-1;
};

#endif // LIGHT_COMPONENT_H
