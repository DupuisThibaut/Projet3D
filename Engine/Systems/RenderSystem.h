#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H
#include <unordered_map>

struct Frustum {
    glm::vec4 planes[6];
};

Frustum extractFrustum(const glm::mat4& vpMatrix) {
    Frustum frustum;
    // left
    frustum.planes[0] = glm::vec4(
        vpMatrix[0][3] + vpMatrix[0][0],
        vpMatrix[1][3] + vpMatrix[1][0],
        vpMatrix[2][3] + vpMatrix[2][0],
        vpMatrix[3][3] + vpMatrix[3][0]
    );
    // right
    frustum.planes[1] = glm::vec4(
        vpMatrix[0][3] - vpMatrix[0][0],
        vpMatrix[1][3] - vpMatrix[1][0],
        vpMatrix[2][3] - vpMatrix[2][0],
        vpMatrix[3][3] - vpMatrix[3][0]
    );
    // bottom
    frustum.planes[2] = glm::vec4(
        vpMatrix[0][3] + vpMatrix[0][1],
        vpMatrix[1][3] + vpMatrix[1][1],
        vpMatrix[2][3] + vpMatrix[2][1],
        vpMatrix[3][3] + vpMatrix[3][1]
    );
    // top
    frustum.planes[3] = glm::vec4(
        vpMatrix[0][3] - vpMatrix[0][1],
        vpMatrix[1][3] - vpMatrix[1][1],
        vpMatrix[2][3] - vpMatrix[2][1],
        vpMatrix[3][3] - vpMatrix[3][1]
    );
    // near
    frustum.planes[4] = glm::vec4(
        vpMatrix[0][3] + vpMatrix[0][2],
        vpMatrix[1][3] + vpMatrix[1][2],
        vpMatrix[2][3] + vpMatrix[2][2],
        vpMatrix[3][3] + vpMatrix[3][2]
    );
    // far
    frustum.planes[5] = glm::vec4(
        vpMatrix[0][3] - vpMatrix[0][2],
        vpMatrix[1][3] - vpMatrix[1][2],
        vpMatrix[2][3] - vpMatrix[2][2],
        vpMatrix[3][3] - vpMatrix[3][2]
    );
    // Normalisation des plans
    for (int i = 0; i < 6; i++) {
        float length = glm::length(glm::vec3(frustum.planes[i]));
        frustum.planes[i] /= length;
    }
    return frustum;
}


class RenderSystem {
public:
    EntityManager* entityManager;
    GLuint shaderProgram;
    RenderSystem(EntityManager* em, GLuint shaderProg) : entityManager(em), shaderProgram(shaderProg) {}
    void update(const std::vector<Entity>& entities) {
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
        Frustum frustum = extractFrustum(proj * view);
        for (const Entity& entity : entities) {
            //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            auto meshIt = entityManager->GetComponents<MeshComponent>().find(entity.id);
            auto transformIt = entityManager->GetComponents<TransformComponent>().find(entity.id);
            auto materialIt = entityManager->GetComponents<MaterialComponent>().find(entity.id);
            if (meshIt != entityManager->GetComponents<MeshComponent>().end() && transformIt != entityManager->GetComponents<TransformComponent>().end() && materialIt != entityManager->GetComponents<MaterialComponent>().end()) {
                const MeshComponent& mesh = meshIt->second;
                const TransformComponent& transform = transformIt->second;
                const MaterialComponent& material = materialIt->second;

                // if(isSphereInFrustum(frustum, mesh.boundingSphereFrustrumCulling.center + glm::vec3(transform.worldMatrix[3]), mesh.boundingSphereFrustrumCulling.radius * std::max({transform.scale.x, transform.scale.y, transform.scale.z})) == false) {
                //     continue; 
                // }
                glm::mat4 modelMatrix = entityManager->GetComponent<TransformComponent>(entity.id).worldMatrix;
                glm::mat4 model = modelMatrix;

                material.bind(shaderProgram);

                GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
                glBindVertexArray(mesh.VAO);
                glDrawElements(GL_TRIANGLES, mesh.vertexCount, GL_UNSIGNED_SHORT, 0);


            }
        }
    }

    bool isSphereInFrustum(const Frustum& frustum, const glm::vec3& center, float radius) {
        for (int i = 0; i < 6; i++) {
            const glm::vec4& plane = frustum.planes[i];
            if (plane.x * center.x + plane.y * center.y + plane.z * center.z + plane.w <= -radius) {
                return false;
            }
        }
        return true;
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

