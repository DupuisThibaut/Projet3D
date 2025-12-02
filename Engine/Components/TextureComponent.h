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

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId, const std::string& gameFolder) {
        if(entityData.contains("texture")){
            if( entityData["texture"].contains("path")){
                std::string texturePath = gameFolder + "/" + entityData["texture"]["path"].get<std::string>();
                path=texturePath;
                isTexture=true;
            }else{
                texte=entityData["texture"]["texte"];
                isTexture=false;
            }
            if( entityData["texture"].contains("numero")){
                numero=entityData["texture"]["numero"];
                // std::cout<<"posx : "<<numero<<std::endl;
            }
            if( entityData["texture"].contains("positionX")){
                positionX=entityData["texture"]["positionX"];
                // std::cout<<"posx : "<<positionX<<std::endl;
            }
            if( entityData["texture"].contains("positionY")){
                positionY=entityData["texture"]["positionY"];
                // std::cout<<"posx : "<<positionY<<std::endl;
            }
            if( entityData["texture"].contains("width")){
                width=entityData["texture"]["width"];
                // std::cout<<"posx : "<<width<<std::endl;
            }
            if( entityData["texture"].contains("height")){
                height=entityData["texture"]["height"];
                // std::cout<<"posx : "<<height<<std::endl;
            }
            if( entityData["texture"].contains("taille")){
                taille=entityData["texture"]["taille"];
                // std::cout<<"posx : "<<taille<<std::endl;
            }
            if( entityData["texture"].contains("police")){
                std::string policePath = gameFolder + "/" + entityData["texture"]["police"].get<std::string>();
                police=policePath;
                // std::cout<<"posx : "<<police<<std::endl;
            }
        }
    }
};

#endif // TEXTURE_COMPONENT_H
