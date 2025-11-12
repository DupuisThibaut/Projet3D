#ifndef LIGHTSYSTEM_H
#define LIGHTSYSTEM_H
#include <unordered_map>

class LightSystem {

public:
    EntityManager* entityManager;
    GLint shaderProgram;

    LightSystem(EntityManager* em, GLint shaderProg) : entityManager(em), shaderProgram(shaderProg) {}
    void update() {
        const auto& lights = entityManager->GetComponents<LightComponent>();
        if(lights.empty()) return;
        for (const auto& kv : lights) {
            const LightComponent& light = kv.second;
            auto itT = entityManager->GetComponents<TransformComponent>().find(kv.first);
            if (itT == entityManager->GetComponents<TransformComponent>().end()) continue;
            const TransformComponent& t = itT->second;
            auto itM = entityManager->GetComponents<MaterialComponent>().find(kv.first);
            if (itM == entityManager->GetComponents<MaterialComponent>().end()) continue;
            const MaterialComponent& mat = itM->second;

            GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
            if (lightPosLoc >= 0) glUniform3fv(lightPosLoc, 1, &t.worldMatrix[3][0]);

            GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
            if (lightColorLoc >= 0) glUniform3fv(lightColorLoc, 1, &mat.color[0]);
            
            GLint lightIntensityLoc = glGetUniformLocation(shaderProgram, "lightIntensity");
            if (lightIntensityLoc >= 0) glUniform1f(lightIntensityLoc, light.intensity);
            
            break; 
        }
    }
};

#endif // LIGHTSYSTEM_H