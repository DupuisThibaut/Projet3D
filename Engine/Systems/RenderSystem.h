#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H
#include <unordered_map>


class RenderSystem {
public:
    EntityManager* entityManager;
    GLuint shaderProgram;
    Dispatcher* dispatcher;
    GLint nbTextures=0;
    GLuint textures[16];
    GLfloat infoTextures[64];
    GLint idTextures[16];
    RenderSystem(EntityManager* em, GLuint shaderProg, Dispatcher* disp, const std::vector<Entity>& entities) : entityManager(em), shaderProgram(shaderProg), dispatcher(disp) {
        dispatcher->subscribe([this](const InputEvent& event) {
            this->onInput(event);
            return false; // Ne pas arrêter la propagation
        });
        for(auto& e : entities){
            if(entityManager->HasComponent<TextureComponent>(e.id)){
                auto& text = entityManager->GetComponent<TextureComponent>(e.id);
                glUseProgram(shaderProgram);
                infoTextures[nbTextures*4]=text.positionX;
                infoTextures[nbTextures*4+1]=text.positionY;
                infoTextures[nbTextures*4+2]=text.width;
                infoTextures[nbTextures*4+3]=text.height;
                glGenTextures(1, &textures[nbTextures]);
                glBindTexture(GL_TEXTURE_2D, textures[nbTextures]);
                int w,h,c;
                unsigned char* img=stbi_load(text.path.c_str(),&w,&h,&c,4);
                glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,img);
                glGenerateMipmap(GL_TEXTURE_2D);
                stbi_image_free(img);
                idTextures[nbTextures]=nbTextures;
                nbTextures++;
            }
        }
    }

    void debugPrintScene() {
        if (!entityManager) { std::cerr << "[RenderSystem] no entityManager\n"; return; }
        auto &meshes = entityManager->GetComponents<MeshComponent>();
        std::cerr << "[RenderSystem] Mesh count = " << meshes.size() << "\n";
        for (const auto &kv : meshes) {
            uint32_t id = kv.first;
            std::cerr << "  Mesh entity " << id;
            std::cerr << " mesh="     << (entityManager->HasComponent<MeshComponent>(id)     ? "yes" : "no");
            if(entityManager->HasComponent<MeshComponent>(id)) {
                const MeshComponent& mesh = entityManager->GetComponent<MeshComponent>(id);
                std::cerr << " (vao=" << mesh.VAO << " vtx=" << mesh.vertexCount << ")";
            }
            std::cerr << " transform=" << (entityManager->HasComponent<TransformComponent>(id) ? "yes" : "no");
            std::cerr << " material="  << (entityManager->HasComponent<MaterialComponent>(id)  ? "yes" : "no");
            std::cerr << "\n";
        }
    }
    void update(const std::vector<Entity>& entities) {
        //debugPrintScene();

        glUseProgram(shaderProgram);
        for (int i=0;i<nbTextures;i++) {
            glActiveTexture(GL_TEXTURE0+i);
            glBindTexture(GL_TEXTURE_2D,textures[i]);
        }
        glUniform1iv(glGetUniformLocation(shaderProgram,"textures"),nbTextures,idTextures);
        glUniform4fv(glGetUniformLocation(shaderProgram,"info"),nbTextures,infoTextures);
        glUniform1iv(glGetUniformLocation(shaderProgram,"id"),nbTextures,idTextures);
        glUniform1i(glGetUniformLocation(shaderProgram,"nb"),nbTextures);
        glm::mat4 view;
        glm::mat4 proj;
        for(auto& camera : entityManager->GetComponents<CameraComponent>()){
            
            if (camera.second.isActive == false) continue;
            view = camera.second.getViewMatrix(glm::vec3(entityManager->GetComponent<TransformComponent>(camera.first).worldMatrix[3]));
            proj = camera.second.getProjectionMatrix();
            GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
            if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
            GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
            if (projLoc >= 0) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &proj[0][0]);
            glm::mat4 vp = proj * view;
        }
        for (const Entity& entity : entities) {
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            auto meshIt = entityManager->GetComponents<MeshComponent>().find(entity.id);
            auto transformIt = entityManager->GetComponents<TransformComponent>().find(entity.id);
            auto materialIt = entityManager->GetComponents<MaterialComponent>().find(entity.id);
            if (meshIt != entityManager->GetComponents<MeshComponent>().end() && transformIt != entityManager->GetComponents<TransformComponent>().end() && materialIt != entityManager->GetComponents<MaterialComponent>().end()) {
                const MeshComponent& mesh = meshIt->second;
                const TransformComponent& transform = transformIt->second;
                const MaterialComponent& material = materialIt->second;

                glm::mat4 modelMatrix = entityManager->GetComponent<TransformComponent>(entity.id).worldMatrix;
                glm::mat4 model = modelMatrix;

                material.bind(shaderProgram);

                GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
                glBindVertexArray(mesh.VAO);
                GLint isAnimLoc = glGetUniformLocation(shaderProgram, "isAnimated");
                glUniform1i(isAnimLoc, 0);
                if (entityManager->HasComponent<AnimationComponent>(entity.id)) {
                    auto& animComp = entityManager->GetComponent<AnimationComponent>(entity.id);
                    if (entityManager->GetComponent<AnimationComponent>(entity.id).isPlaying) {
                        glUniform1i(isAnimLoc, 1);
                    }
                    // SSBO with final bone matrices (binding must match shader)
                    if (animComp.bonesSSBO) {
                        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, animComp.bonesSSBO); // binding=3
                        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, animComp.bones.size() * sizeof(glm::mat4), animComp.finalBoneMatrices.data());
                        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
                    }

                    // Bone IDs attribute (location 5)
                    if (animComp.boneIDVBO) {
                        glBindBuffer(GL_ARRAY_BUFFER, animComp.boneIDVBO);
                        glEnableVertexAttribArray(5);
                        glVertexAttribIPointer(5, 4, GL_INT, sizeof(glm::ivec4), (void*)0);
                    }

                    // Bone Weights attribute (location 6)
                    if (animComp.boneWeightVBO) {
                        glBindBuffer(GL_ARRAY_BUFFER, animComp.boneWeightVBO);
                        glEnableVertexAttribArray(6);
                        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
                    }

                    // Unbind array buffer to avoid leaking state
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                }
                glDrawElements(GL_TRIANGLES, mesh.vertexCount, GL_UNSIGNED_SHORT, 0);
            }
        }
    }

    void onInput(const InputEvent& event) {
        if(event.buttons.empty()) return;
        for(const auto& button : event.buttons) {
            if(button == "R") {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            } else if(button == "F") {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
        }
    }
};

#endif // ENTITY_H

