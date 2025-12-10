#ifndef MATERIAL_COMPONENT_H
#define MATERIAL_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <iostream>
#include <common/stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>


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

    int particularite;

    void setTexture(const std::string& path) {
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

    bool loadTextureFromAssimp(const std::string& fbxPath) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(fbxPath,
            aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);

        if (!scene || !scene->HasMeshes()) {
            std::cerr << "MaterialComponent: failed to load FBX file: " << fbxPath << std::endl;
            return false;
        }

        const aiMesh* mesh = scene->mMeshes[0]; // Prend le premier mesh
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        aiString texPath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            std::string textureFile = texPath.C_Str();
            std::string folder = fbxPath.substr(0, fbxPath.find_last_of("/\\"));
            std::string fullTexPath = folder + "/" + textureFile;
            std::replace(fullTexPath.begin(), fullTexPath.end(), '\\', '/');
             std::cout << "Full texture path: " << fullTexPath << std::endl;
            setTexture(fullTexPath);
            return loadTexture();
        }
        std::cerr << "MaterialComponent: aucune texture diffuse trouvée dans le FBX." << std::endl;
        return false;
    }

    void setColor(const glm::vec3& col, const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular, float shini) {
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
            glBindTexture(GL_TEXTURE_2D, 0);
            GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
            if (colorLoc >= 0) glUniform3fv(colorLoc, 1, &color[0]);
            GLint texLoc = glGetUniformLocation(shaderProgram, "materialType");
            if (texLoc >= 0) glUniform1i(texLoc, 1);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
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
        bool isFBX = false;
        std::string meshPath;
        if(entityData.contains("mesh") && entityData["mesh"].contains("type") && entityData["mesh"]["type"] == "file") {
            meshPath = gameFolder + "/" + entityData["mesh"]["path"].get<std::string>();
            std::string ext = meshPath.substr(meshPath.find_last_of('.') + 1);
            if (ext == "fbx" || ext == "FBX") {
                isFBX = true;
            }
        }
        if(entityData.contains("material")){
            if( entityData["material"].contains("type")){
                if (entityData["material"]["type"] == "texture") {
                    bool t=false;
                    if(isFBX){
                        t = loadTextureFromAssimp(meshPath);
                        glm::vec3 ambient=glm::vec3(0.0,0.0,0.0);
                        glm::vec3 diffuse=glm::vec3(1.0,1.0,1.0);
                        glm::vec3 specular=glm::vec3(1.0,1.0,1.0);
                        float shininess=16.0f;
                        setColor(glm::vec3(1.0,1.0,1.0),ambient,diffuse,specular,shininess);
                    } 
                    if( entityData["material"].contains("path") && t == false){
                        std::string texturePath = entityData["material"]["path"].get<std::string>();
                        type = Type::Texture;
                        setTexture(gameFolder + "/" + texturePath);
                        if (!loadTexture()) {
                            std::cerr << "Erreur de changement de la texture pour entity ID : " << entityId << std::endl;
                        }
                        std::cout << "Texture chargée ID : " << texture<< std::endl;
                    }
                } else if (entityData["material"]["type"] == "color") {
                    glm::vec3 color = glm::vec3(entityData["material"]["color"][0],
                                                entityData["material"]["color"][1],
                                                entityData["material"]["color"][2]);
                    glm::vec3 ambient=glm::vec3(entityData["material"]["ambient"][0],
                                                entityData["material"]["ambient"][1],
                                                entityData["material"]["ambient"][2]);
                    // std::cout<<"ambient_material x : "<<ambient[0]<<" ambient_material y : "<<ambient[1]<<" ambient_material z : "<<ambient[2]<<std::endl;
                    glm::vec3 diffuse=glm::vec3(entityData["material"]["diffuse"][0],
                                                entityData["material"]["diffuse"][1],
                                                entityData["material"]["diffuse"][2]);
                    glm::vec3 specular=glm::vec3(entityData["material"]["specular"][0],
                                                entityData["material"]["specular"][1],
                                                entityData["material"]["specular"][2]);
                    float shininess=entityData["material"]["shininess"];
                    setColor(color,ambient,diffuse,specular,shininess);
                    if( entityData["material"].contains("path")){
                        std::string texturePath = gameFolder + "/" + entityData["material"]["path"].get<std::string>();
                        setTexture(texturePath);
                    }
                    if( entityData["material"].contains("reflection")){
                        particularite=1;
                    }
                    if( entityData["material"].contains("refraction")){
                        particularite=2;
                    }
                } else {
                    // Default material
                    setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
                }
            }
        }
    }

    void renderEditor(){
        if(ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* materialTypes[] = { "None", "Texture", "Color" };
            int currentType = static_cast<int>(type);
            if (ImGui::Combo("Material Type", &currentType, materialTypes, IM_ARRAYSIZE(materialTypes))) {
                type = static_cast<Type>(currentType);
            }
            ImGui::Separator();
                        if (type == Type::Texture) {
                ImGui::Text("Texture Settings");
                ImVec2 dropZoneSize(256, 128);
                ImGui::BeginChild("TextureDropZone", dropZoneSize, true, ImGuiWindowFlags_NoScrollbar);
                if (texture != 0) {
                    ImGui::Image((void*)(intptr_t)texture, ImVec2(dropZoneSize.x - 16, dropZoneSize.y - 16));
                } else {
                    ImGui::SetCursorPosY(dropZoneSize.y * 0.5f - 20);
                    ImGui::TextWrapped("Drag & Drop a texture file here\nor click to browse");
                }
                
                ImGui::EndChild();
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                        const char* droppedPath = (const char*)payload->Data;
                        texturePath = std::string(droppedPath);
                        setTexture(texturePath);
                        if (loadTexture()) {
                            std::cout << "Loaded texture via drag & drop: " << texturePath << std::endl;
                        } else {
                            std::cerr << "Failed to load texture: " << texturePath << std::endl;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                if (!texturePath.empty()) {
                    ImGui::Text("Path: %s", texturePath.c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Clear")) {
                        texture = 0;
                        texturePath.clear();
                    }
                }
                ImGui::Separator();
                ImGui::Text("Or enter path manually:");
                char texturePathBuffer[512];
                strncpy(texturePathBuffer, texturePath.c_str(), sizeof(texturePathBuffer));
                texturePathBuffer[sizeof(texturePathBuffer) - 1] = '\0';
                
                if (ImGui::InputText("##TexturePath", texturePathBuffer, sizeof(texturePathBuffer))) {
                    texturePath = std::string(texturePathBuffer);
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    if (!texturePath.empty()) {
                        setTexture(texturePath);
                        if (loadTexture()) {
                            std::cout << "Loaded texture: " << texturePath << std::endl;
                        }
                    }
                }
            }
            else if (type == Type::Color) {
                ImGui::Text("Color Settings");
                bool needsUpdate = false;
                needsUpdate |= ImGui::ColorEdit3("Color", &color.x);
                needsUpdate |= ImGui::ColorEdit3 ("Ambient", &ambient_material.x);
                needsUpdate |= ImGui::ColorEdit3 ("Diffuse", &diffuse_material.x);
                needsUpdate |= ImGui::ColorEdit3 ("Specular", &specular_material.x);
                needsUpdate |= ImGui::SliderFloat("Shininess", &shininess, 1.0f, 128.0f);
                if (needsUpdate) {
                    std::cout << "Material color updated" << std::endl;
                }
                if (ImGui::Button("Reset Material")) {
                    setColor(
                        glm::vec3(1.0f, 1.0f, 1.0f), 
                        glm::vec3(0.8f, 0.8f, 0.8f), 
                        glm::vec3(1.0f, 1.0f, 1.0f), 
                        glm::vec3(0.2f, 0.2f, 0.2f), 
                        32.0f                         
                    );
                    std::cout << "Material reset to default" << std::endl;
                }
                ImGui::Separator();
                ImGui::Text("Quick Presets:");
                if (ImGui::Button("Gold")) {
                    setColor(
                        glm::vec3(1.0f, 0.84f, 0.0f),    
                        glm::vec3(0.25f, 0.20f, 0.07f), 
                        glm::vec3(0.75f, 0.61f, 0.23f),  
                        glm::vec3(0.63f, 0.56f, 0.37f),  
                        51.2f                            
                    );
                    std::cout << "Applied gold material preset" << std::endl;
                }
                ImGui::SameLine();
                if (ImGui::Button("Silver")) {
                    setColor(
                        glm::vec3(0.75f, 0.75f, 0.75f), 
                        glm::vec3(0.19f, 0.19f, 0.19f), 
                        glm::vec3(0.51f, 0.51f, 0.51f),  
                        glm::vec3(0.51f, 0.51f, 0.51f),  
                        51.2f                             
                    );
                    std::cout << "Applied silver material preset" << std::endl;
                }
                ImGui::SameLine();
                if (ImGui::Button("Bronze")) {
                    setColor(
                        glm::vec3(0.71f, 0.40f, 0.11f),  
                        glm::vec3(0.21f, 0.13f, 0.05f), 
                        glm::vec3(0.71f, 0.43f, 0.18f), 
                        glm::vec3(0.39f, 0.27f, 0.17f), 
                        25.6f                             
                    );
                    std::cout << "Applied bronze material preset" << std::endl;
                }
                if (ImGui::Button("Emerald")) {
                    setColor(
                        glm::vec3(0.07f, 0.61f, 0.08f),  
                        glm::vec3(0.02f, 0.17f, 0.02f), 
                        glm::vec3(0.63f, 0.73f, 0.63f), 
                        glm::vec3(0.63f, 0.73f, 0.63f), 
                        76.8f                             
                    );
                    std::cout << "Applied emerald material preset" << std::endl;
                }
                if(ImGui::Button("Ruby")) {
                    setColor(
                        glm::vec3(0.61f, 0.04f, 0.04f), 
                        glm::vec3(0.17f, 0.01f, 0.01f),  
                        glm::vec3(0.73f, 0.06f, 0.06f),  
                        glm::vec3(0.73f, 0.06f, 0.06f),  
                        76.8f                           
                    );
                    std::cout << "Applied ruby material preset" << std::endl;
                }
                ImGui::Separator();
                if(particularite == 1){
                    ImGui::Text("Material Particularity: Reflection");
                }
                else if(particularite == 2){
                    ImGui::Text("Material Particularity: Refraction");
                }
                bool isReflection = (particularite == 1);
                bool isRefraction = (particularite == 2);
                
                if (ImGui::Checkbox("Reflection", &isReflection)) {
                    particularite = isReflection ? 1 : 0;
                    if (isReflection) isRefraction = false;
                }
                ImGui::SameLine();
                if (ImGui::Checkbox("Refraction", &isRefraction)) {
                    particularite = isRefraction ? 2 : 0;
                    if (isRefraction) isReflection = false;
                }
            }
        }
    }

    json toJson() {
        nlohmann::json j;
        if (type == Type::Texture) {
            j["type"] = "texture";
            j["path"] = texturePath;
        } else if (type == Type::Color) {
            j["type"] = "color";
            j["color"] = { color.r, color.g, color.b };
            j["ambient"] = { ambient_material.r, ambient_material.g, ambient_material.b };
            j["diffuse"] = { diffuse_material.r, diffuse_material.g, diffuse_material.b };
            j["specular"] = { specular_material.r, specular_material.g, specular_material.b };
            j["shininess"] = shininess;
            j["path"] = texturePath;
            if(particularite == 1){
                j["reflection"] = true;
            }
            else if(particularite == 2){
                j["refraction"] = true;
            }
        } else {
            j["type"] = "none";
        }
        return j;
    }
};

#endif // MATERIAL_COMPONENT_H
