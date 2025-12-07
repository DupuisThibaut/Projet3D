#ifndef PARTICULE_COMPONENT_H
#define PARTICULE_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cmath>

struct ParticuleComponent {
    int nb = 100;
    std::string path = "";
    std::vector<glm::vec3> pos;
    std::vector<glm::vec3> posAvant;
    std::vector<glm::vec3> velocity;
    std::vector<float> age;
    std::vector<float> ageMax;
    std::vector<float> rayon;
    std::vector<bool> firstFrame;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 offset = glm::vec3(0.0f);
    float bouncingFactor = 0.2f;

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
            if(entityData["particule"].contains("offset")){
                offset=glm::vec3(entityData["particule"]["offset"][0],
                                entityData["particule"]["offset"][1],
                                entityData["particule"]["offset"][2]);
            }
            for(int i=0;i<nb;i++){
                glm::vec3 randomOffset = glm::vec3(offset.x*rand() / static_cast<float>(RAND_MAX),
                                                   offset.y*rand() / static_cast<float>(RAND_MAX),
                                                   offset.z*rand() / static_cast<float>(RAND_MAX));
                glm::vec3 centeredOffset = randomOffset - offset * 0.5f;
                pos.push_back(position + centeredOffset);
                posAvant.push_back(position + centeredOffset);
                age.push_back(0.0f);
            }
            if(entityData["particule"].contains("rayon")){
                float r=entityData["particule"]["rayon"];
                for(int i=0;i<nb;i++){
                    rayon.push_back(r);
                }
            } else {
                for(int i=0;i<nb;i++){
                    rayon.push_back(rand()/float(RAND_MAX)*0.05f);
                }
            }
            if(entityData["particule"].contains("ageMax")){
                float a=entityData["particule"]["ageMax"];
                for(int i=0;i<nb;i++){
                    ageMax.push_back(a + (0.75f*a * rand() / float(RAND_MAX)));
                }
            } else {
                for(int i=0;i<nb;i++){
                    ageMax.push_back(125.0f + (100.0f * rand() / float(RAND_MAX)));
                }
            }
            if(entityData["particule"].contains("velocity")){
                glm::vec3 s=glm::vec3(entityData["particule"]["velocity"][0],
                                      entityData["particule"]["velocity"][1],
                                      entityData["particule"]["velocity"][2]);
                for(int i=0;i<nb;i++){
                    velocity.push_back(s);
                }
            } else {
                for(int i=0;i<nb;i++){
                    velocity.push_back(glm::vec3(0.0f,-9.81f,0.0f));
                }
            }
            if(entityData["particule"].contains("bouncing")){
                bouncingFactor=entityData["particule"]["bouncing"];
            }
            firstFrame = std::vector<bool>(nb, true);
        }
    }
    void init(int i){
        glm::vec3 randomOffset = glm::vec3(offset.x*rand() / static_cast<float>(RAND_MAX),
                                           offset.y*rand() / static_cast<float>(RAND_MAX),
                                           offset.z*rand() / static_cast<float>(RAND_MAX));
        glm::vec3 centeredOffset = randomOffset - offset * 0.5f;
        pos[i] = position + centeredOffset;
        posAvant[i] = position + centeredOffset;
        age[i] = 0.0f;
        firstFrame[i] = true;
    }
    

};
#endif // PARTICULE_COMPONENT_H