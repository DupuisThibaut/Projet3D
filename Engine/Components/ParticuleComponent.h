#ifndef PARTICULE_COMPONENT_H
#define PARTICULE_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

struct ParticuleComponent {
    int nb;
    std::string path;
    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> speed;
    std::vector<float> age;
    std::vector<float> ageMax;
    std::vector<float> rayon;
    glm::vec3 position;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId, const std::string& gameFolder){
        if(entityData.contains("particule")){
            nb=entityData["particule"]["nombre"];
            if( entityData["particule"].contains("path")){
                std::string texturePath = gameFolder + "/" + entityData["particule"]["path"].get<std::string>();
                path=texturePath;
            }
            if(entityData.contains("transform")){
                position=glm::vec3(entityData["transform"]["position"][0],
                                entityData["transform"]["position"][1],
                                entityData["transform"]["position"][2]);
            }
        }
    }

    

};
#endif // PARTICULE_COMPONENT_H