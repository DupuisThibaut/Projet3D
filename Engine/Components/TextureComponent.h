#ifndef TEXTURE_COMPONENT_H
#define TEXTURE_COMPONENT_H

#include <glm/glm.hpp>
#include <string>

struct TextureComponent {
    std::string path;
    std::string texte;
    int numero;
    float positionX;
    float positionY;
    float width;
    float height;
    bool isTexture;
    int taille;
    std::string police;
};

#endif // TEXTURE_COMPONENT_H
