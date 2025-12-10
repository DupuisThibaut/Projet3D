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
    std::string texture = "";
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
    int particularite=0;
    std::string GameFolder;
    float ageMaxGlobal=1.0f;
    float rayonGlobal=0.05f;
    bool rayonAleatoire=true;
    bool ageMaxAleatoire=true;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId, const std::string& gameFolder){
        GameFolder = gameFolder;
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
                    rayonGlobal=r;
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
                    ageMaxGlobal=a;
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
            if(entityData["particule"].contains("texture")){
                std::string texturePath = gameFolder + "/" + entityData["particule"]["texture"].get<std::string>();
                texture=texturePath;
            }if(entityData["particule"].contains("reflection")){
                particularite=1;
            }if(entityData["particule"].contains("refraction")){
                particularite=2;
            }if(entityData["particule"].contains("metallicite")){
                particularite=2;
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
    
    void renderEditor(){
            ImGui::InputInt("Nombre de particules", &nb);
            ImGui::InputFloat3("Offset", &offset[0]);
            ImGui::InputFloat3("Position Emission", &position[0]);
            ImGui::Checkbox("Age Max Aléatoire", &ageMaxAleatoire);
            if (!ageMaxAleatoire) {
                ImGui::InputFloat("Age Max", &ageMaxGlobal);
            }

            ImGui::Checkbox("Rayon Aléatoire", &rayonAleatoire);
            if (!rayonAleatoire) {
                ImGui::InputFloat("Rayon", &rayonGlobal);
            }
            ImGui::InputFloat3("Velocity", &velocity[0][0]);
            ImGui::InputFloat("Bouncing Factor", &bouncingFactor);
            ImGui::InputInt("Particularite", &particularite);
            ImGui::Separator();
            ImGui::Text("Texture:");
            ImGui::BeginChild("ParticuleTextureDropZone", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::TextWrapped("%s", path.empty() ? "Drag & drop a texture file here" : path.c_str());
            ImGui::EndChild();
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                    const char* droppedPath = (const char*)payload->Data;
                    std::string ext = droppedPath;
                    ext = ext.substr(ext.find_last_of('.') + 1);
                    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" ||
                        ext == "PNG" || ext == "JPG" || ext == "JPEG" || ext == "BMP" || ext == "TGA") {
                        path = std::string(droppedPath);
                    }
                }
                ImGui::EndDragDropTarget();
            }
            char texturePathBuffer[256];
            strncpy(texturePathBuffer, path.c_str(), sizeof(texturePathBuffer));
            texturePathBuffer[sizeof(texturePathBuffer) - 1] = '\0';
            ImGui::InputText("Texture Path", texturePathBuffer, sizeof(texturePathBuffer));
            path = std::string(texturePathBuffer);
    }

    json toJson(){
        nlohmann::json j;
        j["nombre"] = nb;
        std::string relPath = path;
        size_t pos = path.find(GameFolder + "/");
        if (pos != std::string::npos) {
            relPath = path.substr(pos + GameFolder.length() + 1);
        } else {
            relPath = path;
        }
        j["path"] = relPath;
        j["offset"] = { offset.x, offset.y, offset.z };
        if(!rayonAleatoire) j["rayon"] = rayonGlobal;
        j["bouncing"] = bouncingFactor;
        if(!ageMaxAleatoire) j["ageMax"] = ageMaxGlobal;
        j["velocity"] = { velocity[0].x, velocity[0].y, velocity[0].z };
        if(!texture.empty()){
            std::string relTexPath = texture;
            size_t posTex = texture.find(GameFolder + "/");
            if (posTex != std::string::npos) {
                relTexPath = texture.substr(posTex + GameFolder.length() + 1);
            } else {
                relTexPath = texture;
            }
            j["texture"] = relTexPath;
        }
        if(particularite==1){
            j["reflection"] = true;
        }
        if(particularite==2){
            j["refraction"] = true;
        }
        if(particularite==3){
            j["metallicite"] = true;
        }
        return j;
    }

};
#endif // PARTICULE_COMPONENT_H