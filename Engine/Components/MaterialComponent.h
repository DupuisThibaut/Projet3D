#ifndef MATERIAL_COMPONENT_H
#define MATERIAL_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <iostream>
#include <common/stb_image.h>


struct MaterialComponent {
    enum class Type {
        None,
        Texture,
        Color
    };
    Type type = Type::Color;
    // Pour le cas des textures
    GLuint texture = 0;
    std::string texturePath;

    // Pour le cas des couleurs
    glm::vec3 color = glm::vec3(1.0f);

    glm::vec3 diffuse_material;
    glm::vec3 specular_material;
    glm::vec3 ambient_material;
    float shininess;

    void setTexture(const std::string& path) {
        type = Type::Texture;
        texturePath = path;

    }

    bool loadTexture() {
        if (type != Type::Texture || texturePath.empty()) return false;
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 4);
        if (!data) {
            std::cerr << "MaterialComponent: failed to load texture: " << texturePath << std::endl;
            return false;
        }
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
        return true;
    }

    void setColor(const glm::vec3& col, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& ambient, float shini) {
        type = Type::Color;
        color = col;
        diffuse_material=diffuse;
        specular_material=specular;
        ambient_material=ambient;
        shininess=shini;

    }

    void bind(GLuint shaderProgram) const {
        if (type == Type::Texture) {
            if (texture != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture);
                GLint texLoc = glGetUniformLocation(shaderProgram, "materialType");
                if (texLoc >= 0) glUniform1i(texLoc, 0);


            }
        } else if (type == Type::Color) {
            GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
            if (colorLoc >= 0) glUniform3fv(colorLoc, 1, &color[0]);
            GLint texLoc = glGetUniformLocation(shaderProgram, "materialType");
            if (texLoc >= 0) glUniform1i(texLoc, 1);
        } else {
            GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
            if (colorLoc >= 0) {
                glm::vec3 zero(0.0f);
                glUniform3fv(colorLoc, 1, &zero[0]);
                GLint texLoc = glGetUniformLocation(shaderProgram, "materialType");
                if (texLoc >= 0) glUniform1i(texLoc, 1);
            }
        }
    }

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId, const std::string& gameFolder) {
        if(entityData["entities"][entityId].contains("material")){
            if( entityData["entities"][entityId]["material"].contains("type")){
                if (entityData["entities"][entityId]["material"]["type"] == "texture") {
                    std::string texturePath = entityData["entities"][entityId]["material"]["path"].get<std::string>();
                    setTexture(gameFolder + "/" + texturePath);
                    if (!loadTexture()) {
                        std::cerr << "Erreur de changement de la texture pour entity ID : " << entityId << std::endl;
                    }
                } else if (entityData["entities"][entityId]["material"]["type"] == "color") {
                    glm::vec3 color = glm::vec3(entityData["entities"][entityId]["material"]["color"][0],
                                                entityData["entities"][entityId]["material"]["color"][1],
                                                entityData["entities"][entityId]["material"]["color"][2]);
                    glm::vec3 ambient=glm::vec3(entityData["entities"][entityId]["material"]["ambient"][0],
                                                entityData["entities"][entityId]["material"]["ambient"][1],
                                                entityData["entities"][entityId]["material"]["ambient"][2]);
                    glm::vec3 diffuse=glm::vec3(entityData["entities"][entityId]["material"]["diffuse"][0],
                                                entityData["entities"][entityId]["material"]["diffuse"][1],
                                                entityData["entities"][entityId]["material"]["diffuse"][2]);
                    glm::vec3 specular=glm::vec3(entityData["entities"][entityId]["material"]["specular"][0],
                                                 entityData["entities"][entityId]["material"]["specular"][1],
                                                 entityData["entities"][entityId]["material"]["specular"][2]);
                    float shininess=entityData["entities"][entityId]["material"]["shininess"];
                    setColor(color,ambient,diffuse,specular,shininess);
                } else {
                    // Default material
                    setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
                }
            }
        }
    }
};

#endif // MATERIAL_COMPONENT_H
