// SceneManager.h
// Permet de gérer le chargement, changement et suppresion des scènes et le remplissage de l'EntityManager

#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cmath>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

class ScriptSystem;
class EntityManager;
struct Entity;
struct TransformComponent;
struct MeshComponent;
struct MaterialComponent;
struct CameraComponent;
struct LightComponent;
struct ControllerComponent;
struct MyAudioComponent;
struct TagComponent;
struct LayerComponent;
struct RigidBodyComponent;
struct LuaScriptComponent;

class SceneManager
{
private:
    /* data */
public:
    std::string currentScene;
    EntityManager* entityManager = nullptr;
    std::vector<Entity>* entities;
    std::string gameFolder;
    unsigned int width;
    unsigned int height;
    bool editorMode;
    bool Reloading = false;
    std::string pendingPath = "";

    json sceneData;
    SceneManager(){}
    SceneManager(EntityManager* entityManager, std::vector<Entity>* entities) : entityManager(entityManager), entities(entities) {}


    void requestChangeScene(const std::string& scenePath) {
        pendingPath = scenePath;
        Reloading = true;
        std::cerr << "SceneManager: Request change to -> " << pendingPath << std::endl;
    }

    void requestHotReload() {
        pendingPath = currentScene;
        Reloading = true;
        std::cerr << "SceneManager: Request Hot Reload -> " << pendingPath << std::endl;
    }

    void applyPendingScene(unsigned int w, unsigned int h, bool editorModeFlag) {
        if (!Reloading) return;
        std::cout << "--- Applying pending scene " << pendingPath << " ---" << std::endl;
        changeScene(pendingPath);
        Reloading = false;
        pendingPath.clear();
    }

    void loadScene(const std::string& scenePath, unsigned int width, unsigned int height, bool editorMode){
        std::cout << "SceneManager: Loading scene -> " << scenePath << std::endl;
        this->width = width;
        this->height = height;
        this->editorMode = editorMode;
        this->currentScene = scenePath;
        std::ifstream sceneFile(gameFolder + "/" + scenePath);
        if (!sceneFile.is_open()) {
            std::cerr << "Echec de l'ouverture du fichier : " << scenePath << std::endl;
            return;
        }
        try {
            sceneData = json::parse(sceneFile);
        } catch (const std::exception& e) {
            std::cerr << "JSON Error: " << e.what() << std::endl;
            return;
        }
        for(const auto& entityData: sceneData["entities"]){
            Entity e{entityData["id"]};
            entityManager->CreateEntity(e.id);
            entities->push_back(e);
            if(entityData.contains("transform")){
                TransformComponent t;
                t.loadFromFile(sceneData, e.id);
                entityManager->AddComponent<TransformComponent>(e.id, t);
            }
            if(entityData.contains("mesh")){
                MeshComponent m;
                m.loadFromFile(sceneData, e.id, gameFolder, entityManager->GetComponent<TransformComponent>(e.id).position);
                entityManager->AddComponent<MeshComponent>(e.id, m);
            }
            if(entityData.contains("material")){
                MaterialComponent mat;
                mat.loadFromFile(sceneData, e.id, gameFolder);
                entityManager->AddComponent<MaterialComponent>(e.id, mat);
            }
            if(entityData.contains("camera")){
                CameraComponent cam;
                cam.loadFromFile(sceneData, e.id, width, height);
                entityManager->AddComponent<CameraComponent>(e.id, cam);
            }
            if(entityData.contains("light")){
                LightComponent light;
                light.loadFromFile(sceneData, e.id);
                entityManager->AddComponent<LightComponent>(e.id, light);
            }
            if(entityData.contains("controller")){
                ControllerComponent controller;
                controller.loadFromFile(sceneData, e.id);
                entityManager->AddComponent<ControllerComponent>(e.id, controller);
            }
            if(entityData.contains("audio")){
                MyAudioComponent audio;
                audio.loadFromFile(sceneData, e.id, gameFolder);
                entityManager->AddComponent<MyAudioComponent>(e.id, audio);
            }
            if(entityData.contains("tag")){
                TagComponent tag;
                tag.loadFromFile(sceneData, e.id);
                entityManager->AddComponent<TagComponent>(e.id, tag);
            }
            if(entityData.contains("layer")){
                LayerComponent layer;
                layer.loadFromFile(sceneData, e.id);
                entityManager->AddComponent<LayerComponent>(e.id, layer);
            }
            if(entityData.contains("rigidbody")){
                RigidBodyComponent rigidbody;
                rigidbody.loadFromFile(sceneData, e.id);
                entityManager->AddComponent<RigidBodyComponent>(e.id, rigidbody);
            }
            if(entityData.contains("script")){
                LuaScriptComponent luaScript;
                luaScript.loadFromFile(sceneData, e.id, gameFolder);
                entityManager->AddComponent<LuaScriptComponent>(e.id, luaScript);
            }
            
        }
    }

    void resetScene(){
        if (!entityManager || !entities) return;
        entities->clear();
        *entityManager = EntityManager();
    }

    void changeScene(const std::string& scenePath){
        resetScene();
        std::cout << "--- Changing scene to " << scenePath << " ---" << std::endl;
        // Load new scene
        loadScene(scenePath,width,height,editorMode);
    }

    void hotReload(){
        resetScene();
        loadScene(currentScene,width,height,editorMode);
    }


    // Partie Essais Benchmarking

    float benchmarkTimer = 0.0f;
    float benchmarkInterval = 15.0f;
    int sizeBenchmark = 50;
    float moyenneFPS = 0.0f;
    bool benchmarkStarted = false;
    float lastFrame = 0.0f;

    void loadBenchmarkScene( unsigned int width, unsigned int height,int numCubes = 50) {
        this->width = width;
        this->height = height;
        entities->clear();
        *entityManager = EntityManager();
        float spacing = 5.0f;
        int gridSize = static_cast<int>(std::ceil(std::sqrt(numCubes)));
        int i = 0;
        for (i = 0; i < numCubes; ++i) {
            Entity e{(uint32_t)i};
            entities->push_back(e);
            entityManager->CreateEntity(e.id);

            // Transform
            TransformComponent t;
            int row = i / gridSize;
            int col = i % gridSize;
            t.position = glm::vec3(col * spacing, row * spacing, 0.0f);
            t.rotation = glm::vec3(0.0f);
            t.scale = glm::vec3(0.5f);
            entityManager->AddComponent<TransformComponent>(e.id, t);

            // Mesh
            MeshComponent m;
            m.subdivisions = 1;
            //m.loadPrimitive("BOX", t.position);
            m.load_OFF("../Jeu/mesh/suzanne.off");
            entityManager->AddComponent<MeshComponent>(e.id, m);

            // Material
            MaterialComponent mat;
            // mat.setColor(glm::vec3(0.5f + (i % 10) * 0.05f, 0.5f, 0.5f));
            mat.setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
            entityManager->AddComponent<MaterialComponent>(e.id, mat);
        }

        CameraComponent cam;
        int camId = numCubes; // un ID unique après les cubes
        entityManager->CreateEntity(camId);
        entities->push_back(Entity{uint32_t(camId)});

        cam.target   = glm::vec3(gridSize * spacing / 2, gridSize * spacing / 2, 0.0f);
        cam.up       = glm::vec3(0.0f, 1.0f, 0.0f);
        cam.aspectRatio = static_cast<float>(width) / height;
        cam.fov      = 45.0f;
        cam.nearPlane = 0.1f;
        cam.farPlane  = 1000.0f;
        cam.isActive = true;
        cam.updateDirection();
        entityManager->AddComponent<CameraComponent>(camId, cam);
        TransformComponent camTransform;
        camTransform.position = glm::vec3(gridSize * spacing / 2, gridSize * spacing / 2, 10.0f * spacing);
        camTransform.rotation = glm::vec3(0.0f);
        camTransform.scale = glm::vec3(1.0f);

        entityManager->AddComponent<TransformComponent>(camId, camTransform);

        LightComponent light;
        int lightId = numCubes + 1;
        entityManager->CreateEntity(lightId);
        entities->push_back(Entity{uint32_t(lightId)});
        light.intensity = 1.0f;
        entityManager->AddComponent<LightComponent>(lightId, light);
        TransformComponent lightTransform;
        lightTransform.position = glm::vec3(gridSize * spacing / 2, gridSize * spacing / 2, 5.0f * spacing);
        lightTransform.rotation = glm::vec3(0.0f);
        lightTransform.scale = glm::vec3(1.0f);
        entityManager->AddComponent<TransformComponent>(lightId, lightTransform);
        MaterialComponent lightMat;
        lightMat.setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
        // lightMat.setColor(glm::vec3(1.0f, 1.0f, 1.0f));
        entityManager->AddComponent<MaterialComponent>(lightId, lightMat);
        std::cout << "--- Benchmark scene with " << numCubes << " cubes loaded. ---" << std::endl;
    }


    void updateBenchmark(float deltaTime) {
        if(!benchmarkStarted) return;
        benchmarkTimer += deltaTime;   // accumule le temps
        moyenneFPS++;

        if (benchmarkTimer >= benchmarkInterval) {
            float avgFPS = moyenneFPS / benchmarkInterval;
            if(avgFPS < 24.0f){
                std::cout << "--- Benchmark interval ended. Average FPS: " << avgFPS << " --- for " << sizeBenchmark * 511 << " triangles." << std::endl;
                std::cout << "--- Benchmark terminé. ---" << std::endl;
                glfwSetWindowShouldClose(window, GL_TRUE);
                return;
            }
            std::cout << "--- Benchmark interval ended. Average FPS: " << avgFPS << " --- for " << sizeBenchmark * 511 << " triangles." << std::endl;
            benchmarkStarted = false;
            loadBenchmarkScene(width,height,sizeBenchmark*=1.5);
            std::cout << "--- Benchmark scene with " << sizeBenchmark << " cubes loaded. ---" << std::endl;
            benchmarkTimer = 0.0f; // reset du timer
            moyenneFPS = 0.0f;         // reset de la moyenne
            lastFrame = glfwGetTime(); // reset du lastFrame pour éviter un gros deltaTime
            benchmarkStarted = true;
        }
    }



};


#endif // SCENEMANAGER_H