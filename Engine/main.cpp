// Include standard headers
#include <GL/glew.h>

#if defined(__APPLE__) || defined(MACOSX)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <unistd.h>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

// Include GLEW

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>


using namespace glm;

#include <common/shader.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <common/texture.hpp>
#include <common/text2D.hpp>
#include "common/miniaudio.h"
#include "common/Mesh.h"

#include "Assets/Primitives/Plane.h"
#include "Assets/Primitives/Sphere.h"
#include "Assets/Primitives/Box.h"
#include "Assets/Primitives/Cylinder.h"
#include "Assets/Primitives/Cone.h"
#include "Assets/Primitives/Capsule.h"


// Entities
#include "Entities/Entity.h"

// Components
#include "Components/TransformComponent.h"
#include "Components/MeshComponent.h"
#include "Components/MaterialComponent.h"
#include "Components/CameraComponent.h"
#include "Components/LightComponent.h"
#include "Components/ControllerComponent.h"
#include "Components/MyAudioComponent.h"
#include "Components/ScriptComponent.h"
// Systems
#include "Systems/EntityManager.h"
#include "Systems/Dispatcher.h"
#include "Systems/RenderSystem.h"
#include "Systems/LightSystem.h"
#include "Systems/ControllerSystem.h"
#include "Systems/AudioSystem.h"
#include "Systems/TransformSystem.h"
#include "Systems/ScriptSystem.h"
#include "Systems/RayTracerSystem.h"

// Scripts
#include "Scripts/CameraController.h"

#include <common/json.hpp>
using json = nlohmann::json;

// settings
unsigned int SCR_WIDTH = 512;
unsigned int SCR_HEIGHT = 512;

constexpr unsigned int MIN_WINDOW_WIDTH = 512;
constexpr unsigned int MIN_WINDOW_HEIGHT = 512;
std::string gameFolder;
std::string scenePath;
std::string mode;
std::string mode2;
json sceneData;


#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
#include <common/stb_image.h>
#include <unordered_map>
#endif

std::vector<Entity> entities;

// std::unordered_map<uint32_t, MeshComponent> meshComponents;
// std::unordered_map<uint32_t, TransformComponent> transformComponents;
// std::unordered_map<uint32_t, MaterialComponent> materialComponents;
// std::unordered_map<uint32_t, CameraComponent> cameraComponents;
// std::unordered_map<uint32_t, LightComponent> lightComponents;
// std::unordered_map<uint32_t, ControllerComponent> controllerComponents;
// std::unordered_map<uint32_t, AudioComponent> audioComponents;
// std::unordered_map<uint32_t, LuaScriptComponent> luaScriptComponents;

EntityManager entityManager;
AudioSystem audioSystem(&entityManager);
ScriptSystem scriptSystem;
Dispatcher dispatcher;
ControllerSystem input = ControllerSystem(dispatcher);
RayTracerSystem* gRayTracerSystem = nullptr;


void loadScene(){
std::ifstream sceneFile(scenePath);
    if (!sceneFile.is_open()) {
        std::cerr << "❌ Failed to open scene file: " << scenePath << std::endl;
        return;
    }

    sceneData = json::parse(sceneFile);

    for(const auto& entityData: sceneData["entities"]){
        Entity e{entityData["id"]};
        entityManager.CreateEntity(e.id);
        entities.push_back(e);
        std::cout << "Created entity with ID: " << e.id << std::endl;
        if(entityData.contains("transform")){
            TransformComponent t;
            t.position = glm::vec3(entityData["transform"]["position"][0],
                                entityData["transform"]["position"][1],
                                entityData["transform"]["position"][2]);
            t.rotation = glm::vec3(entityData["transform"]["rotation"][0],
                                entityData["transform"]["rotation"][1],
                                entityData["transform"]["rotation"][2]);
            t.scale = glm::vec3(entityData["transform"]["scale"][0],
                                entityData["transform"]["scale"][1],
                                entityData["transform"]["scale"][2]);
            if(entityData["transform"].contains("parent")){
                t.parent = entityData["transform"]["parent"];
            }
            if(entityData["transform"].contains("children")){
                for (const auto& childId : entityData["transform"]["children"]) {
                    t.children.push_back(childId);
                }
            }
            //transformComponents[e.id] = t;
            entityManager.AddComponent<TransformComponent>(e.id, t);
        }
        if(entityData.contains("mesh")){
            MeshComponent m;
            if (entityData["mesh"]["type"] == "primitive") {
                if (entityData["mesh"]["mesh_type"] == "PLANE") {
                    glm::vec3 normal(0.0f, 1.0f, 0.0f);
                    if (entityData["mesh"].contains("normal")) {
                        m.normal = glm::vec3(entityData["mesh"]["normal"][0],
                                           entityData["mesh"]["normal"][1],
                                           entityData["mesh"]["normal"][2]);
                    }
                    float width = 1.0f;
                    int subdivisions = 1;
                    if (entityData["mesh"].contains("subdivisions")) {
                        m.subdivisions = entityData["mesh"]["subdivisions"];
                    }
                    m.loadPrimitive("PLANE");
                }
                else if (entityData["mesh"]["mesh_type"] == "SPHERE") {
                    if(entityData["mesh"].contains("subdivisions")) {
                        m.subdivisions = entityData["mesh"]["subdivisions"];
                    }
                    m.loadPrimitive("SPHERE", entityManager.GetComponent<TransformComponent>(e.id).position);
                }
                else if (entityData["mesh"]["mesh_type"] == "BOX") {
                    m.loadPrimitive("BOX", entityManager.GetComponent<TransformComponent>(e.id).position);
                }
                else if (entityData["mesh"]["mesh_type"] == "CYLINDER") {
                    if (entityData["mesh"].contains("subdivisions")) {
                        m.subdivisions = entityData["mesh"]["subdivisions"];
                    }
                    if (entityData["mesh"].contains("width")) {
                        m.width = entityData["mesh"]["width"];
                    }
                    if (entityData["mesh"].contains("height")) {
                        m.height = entityData["mesh"]["height"];
                    }
                    m.loadPrimitive("CYLINDER", entityManager.GetComponent<TransformComponent>(e.id).position);
                }
                else if (entityData["mesh"]["mesh_type"] == "CONE") {
                    if (entityData["mesh"].contains("subdivisions")) {
                        m.subdivisions = entityData["mesh"]["subdivisions"];
                    }
                    if (entityData["mesh"].contains("width")) {
                        m.width = entityData["mesh"]["width"];
                    }
                    if (entityData["mesh"].contains("height")) {
                        m.height = entityData["mesh"]["height"];
                    }
                    m.loadPrimitive("CONE", entityManager.GetComponent<TransformComponent>(e.id).position);
                }
                else if (entityData["mesh"]["mesh_type"] == "CAPSULE") {
                    if (entityData["mesh"].contains("subdivisions")) {
                        m.subdivisions = entityData["mesh"]["subdivisions"];
                    }
                    if (entityData["mesh"].contains("width")) {
                        m.width = entityData["mesh"]["width"];
                    }
                    if (entityData["mesh"].contains("height")) {
                        m.height = entityData["mesh"]["height"];
                    }
                    m.loadPrimitive("CAPSULE", entityManager.GetComponent<TransformComponent>(e.id).position);
                }
            } else if (entityData["mesh"]["type"] == "file") {
                std::string meshPath = gameFolder + "/" + entityData["mesh"]["path"].get<std::string>();
                m.load_OFF(meshPath);
            }
            //meshComponents[e.id] = m;
            entityManager.AddComponent<MeshComponent>(e.id,m);
        }
        if(entityData.contains("material")){
            MaterialComponent mat;
            if( entityData["material"].contains("type")){
                if ( entityData["material"]["type"] == "texture") {
                    std::string texturePath = gameFolder + "/" + entityData["material"]["path"].get<std::string>();
                    mat.setTexture(texturePath);
                    if (!mat.loadTexture()) {
                        std::cerr << "❌ Failed to load texture for entity ID " << e.id << std::endl;
                    }
                } else if (entityData["material"]["type"] == "color") {
                    glm::vec3 color = glm::vec3(entityData["material"]["color"][0],
                                                entityData["material"]["color"][1],
                                                entityData["material"]["color"][2]);
                    glm::vec3 ambient=glm::vec3(entityData["material"]["ambient"][0],
                                                entityData["material"]["ambient"][1],
                                                entityData["material"]["ambient"][2]);
                    std::cout<<"ambient_material x : "<<ambient[0]<<" ambient_material y : "<<ambient[1]<<" ambient_material z : "<<ambient[2]<<std::endl;
                    glm::vec3 diffuse=glm::vec3(entityData["material"]["diffuse"][0],
                                                entityData["material"]["diffuse"][1],
                                                entityData["material"]["diffuse"][2]);
                    glm::vec3 specular=glm::vec3(entityData["material"]["specular"][0],
                                                entityData["material"]["specular"][1],
                                                entityData["material"]["specular"][2]);
                    float shininess=entityData["material"]["shininess"];
                    mat.setColor(color,ambient,diffuse,specular,shininess);
                    if( entityData["material"].contains("path")){
                        std::string texturePath = gameFolder + "/" + entityData["material"]["path"].get<std::string>();
                        mat.setTexture(texturePath);
                    }
                    if( entityData["material"].contains("reflection")){
                        mat.particularite=1;
                    }
                } else {
                // Default material
                mat.setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
            }
            } else {
                // Default material
                mat.setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
            }
            // materialComponents[e.id] = mat;
            entityManager.AddComponent<MaterialComponent>(e.id,mat);
        }
        if (entityData.contains("camera")) {
            CameraComponent camData;
            camData.id = entityData["camera"]["idCam"];
            glm::vec3 pos(0.0f);
            glm::vec3 lookAt(0.0f, 0.0f, -1.0f);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            if (entityData["camera"].contains("target"))
                lookAt = glm::vec3(entityData["camera"]["target"][0], entityData["camera"]["target"][1], entityData["camera"]["target"][2]);
            if (entityData["camera"].contains("up"))
                up = glm::vec3(entityData["camera"]["up"][0], entityData["camera"]["up"][1], entityData["camera"]["up"][2]);

            camData.up = up;
            // convert absolute lookAt position -> normalized direction expected by CameraComponent
            glm::vec3 dir = glm::normalize(lookAt - pos);
            if (glm::length(dir) > 1e-6f) {
                camData.target = dir;
                camData.yaw   = glm::degrees(atan2(dir.z, dir.x));
                camData.pitch = glm::degrees(asin(glm::clamp(dir.y, -1.0f, 1.0f)));
            }
            if (entityData["camera"].contains("fov")) camData.fov = entityData["camera"]["fov"];
            if (entityData["camera"].contains("near_plane")) camData.nearPlane = entityData["camera"]["near_plane"];
            if (entityData["camera"].contains("far_plane")) camData.farPlane = entityData["camera"]["far_plane"];
            camData.aspectRatio = (float)SCR_WIDTH / SCR_HEIGHT;
            camData.updateDirection();
            //cameraComponents[e.id]= camData;
            entityManager.AddComponent<CameraComponent>(e.id,camData);
        }
        if(entityData.contains("light")){
            LightComponent light;
            light.intensity = entityData["light"]["intensity"];
            //lightComponents[e.id] = light;
            entityManager.AddComponent<LightComponent>(e.id,light);
        }
        if(entityData.contains("controller")){
            ControllerComponent controller;
            controller.moveSpeed = entityData["controller"]["speed"];
            //controllerComponents[e.id] = controller;
            entityManager.AddComponent<ControllerComponent>(e.id,controller);
        }
        if(entityData.contains("audio")){
            MyAudioComponent audio;
            if(entityData["audio"].contains("type")){
                std::string typeStr = entityData["audio"]["type"].get<std::string>();
                // lowercase for robust comparison
                std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), [](unsigned char c){ return std::tolower(c); });
                if(typeStr == "music")      audio.type = AudioType::MUSIC;
                else if(typeStr == "sfx")   audio.type = AudioType::SFX;
                else if(typeStr == "spatial") audio.type = AudioType::SPATIAL;
                else audio.type = AudioType::NONE;
            }
            if(entityData["audio"].contains("path")){
                audio.audioFilePath = gameFolder + "/" + entityData["audio"]["path"].get<std::string>();
            }
            if(entityData["audio"].contains("volume")){
                audio.volume = entityData["audio"]["volume"].get<float>();
            }
            audio.loop = entityData["audio"].value("loop", false);
            audio.playOnStart = entityData["audio"].value("play_on_start", false);
            audio.isPlaying = audio.playOnStart;

            // verify file exists before attempting to load
            if (!audio.audioFilePath.empty() && !std::filesystem::exists(audio.audioFilePath)) {
                std::cerr << "Audio file not found: " << audio.audioFilePath << std::endl;
            } else {
                //audioComponents[e.id] = audio;
                audioSystem.addAudio(e.id, audio);
                entityManager.AddComponent<MyAudioComponent>(e.id,audio);
                std::cout << "Registered audio for entity ID " << e.id << ": " << audio.audioFilePath << std::endl;
            }
        }
        if(entityData.contains("script")){
            std::string scriptType = entityData["script"]["type"].get<std::string>();
            if(scriptType == "C++"){
                if(entityData["script"]["path"] == "CameraController"){
                    CameraController* controller = new CameraController();
                    controller->transform = &entityManager.GetComponent<TransformComponent>(e.id);
                    controller->camera = &entityManager.GetComponent<CameraComponent>(e.id);
                    input.subscribe(controller);
                }
            } else if (scriptType == "Lua") {
                LuaScriptComponent luaScript;
                luaScript.luaScriptPath = gameFolder + "/" + entityData["script"]["path"].get<std::string>();
                entityManager.AddComponent<LuaScriptComponent>(e.id, luaScript);
                scriptSystem.registerLuaScript(e.id, &entityManager.GetComponent<LuaScriptComponent>(e.id));
                
                scriptSystem.registerEntityManager(&entityManager);
                scriptSystem.initScript(entityManager.GetComponent<LuaScriptComponent>(e.id), e.id);
            }
        }
    }
}

void hotReload(){
    std::ifstream sceneFile(scenePath);
    if (!sceneFile.is_open()) {
        std::cerr << "❌ Failed to open scene file: " << scenePath << std::endl;
        return;
    }
    json tmpSceneData = json::parse(sceneFile);
    if(tmpSceneData != sceneData){
        sceneData = tmpSceneData;
        entities.clear();
        // meshComponents.clear();
        // transformComponents.clear();
        // materialComponents.clear();
        // cameraComponents.clear();
        // lightComponents.clear();
        // controllerComponents.clear();
        // audioComponents.clear();
        // audioSystem.clear();
        // luaScriptComponents.clear();
        entityManager = EntityManager();

        loadScene();
        bool anyActiveCamera = false;
        for (const auto &kv : entityManager.GetComponents<CameraComponent>()){
            if (kv.second.isActive) {
                anyActiveCamera = true;
                break;
            }
        }
        if(!anyActiveCamera){
            uint32_t camEntityId = 1000;
            for (const auto &kv : entityManager.GetComponents<CameraComponent>()) if (kv.first < camEntityId) camEntityId = kv.first;
            entityManager.GetComponent<CameraComponent>(camEntityId).isActive = true;
        }
    }
}

void loadBenchmarkScene(int numCubes = 500) {
    entities.clear();
    // transformComponents.clear();
    // meshComponents.clear();
    // materialComponents.clear();
    // cameraComponents.clear();
    // luaScriptComponents.clear();

    float spacing = 5.0f;
    int gridSize = static_cast<int>(std::ceil(std::sqrt(numCubes)));
    int i = 0;
    for (i = 0; i < numCubes; ++i) {
        Entity e{(uint32_t)i};
        entities.push_back(e);
        entityManager.CreateEntity(e.id);

        // Transform
        TransformComponent t;
        int row = i / gridSize;
        int col = i % gridSize;
        t.position = glm::vec3(col * spacing, row * spacing, 0.0f);
        t.rotation = glm::vec3(0.0f);
        t.scale = glm::vec3(0.5f);
        entityManager.AddComponent<TransformComponent>(e.id, t);

        // Mesh
        MeshComponent m;
        m.subdivisions = 1;
        //m.loadPrimitive("BOX", t.position);
        m.load_OFF("../Jeu/mesh/suzanne.off");
        entityManager.AddComponent<MeshComponent>(e.id, m);

        // Material
        MaterialComponent mat;
        // mat.setColor(glm::vec3(0.5f + (i % 10) * 0.05f, 0.5f, 0.5f));
        mat.setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
        entityManager.AddComponent<MaterialComponent>(e.id, mat);

        //script
        // LuaScriptComponent luaScript;
        // luaScript.luaScriptPath = "../Jeu/Scripts/ChangeColor.lua";
        // entityManager.AddComponent<LuaScriptComponent>(e.id, luaScript);
        // scriptSystem.registerLuaScript(e.id, &entityManager.GetComponent<LuaScriptComponent>(e.id));
        // scriptSystem.registerEntityManager(&entityManager);
        // scriptSystem.initScript(entityManager.GetComponent<LuaScriptComponent>(e.id), e.id);
    }

    CameraComponent cam;
    int camId = numCubes; // un ID unique après les cubes
    entityManager.CreateEntity(camId);
    entities.push_back(Entity{uint32_t(camId)});

    cam.target   = glm::vec3(gridSize * spacing / 2, gridSize * spacing / 2, 0.0f);
    cam.up       = glm::vec3(0.0f, 1.0f, 0.0f);
    cam.aspectRatio = static_cast<float>(SCR_WIDTH) / SCR_HEIGHT;
    cam.fov      = 45.0f;
    cam.nearPlane = 0.1f;
    cam.farPlane  = 1000.0f;
    cam.isActive = true;
    cam.updateDirection();
    entityManager.AddComponent<CameraComponent>(camId, cam);
    TransformComponent camTransform;
    camTransform.position = glm::vec3(gridSize * spacing / 2, gridSize * spacing / 2, 10.0f * spacing);
    camTransform.rotation = glm::vec3(0.0f);
    camTransform.scale = glm::vec3(1.0f);

    entityManager.AddComponent<TransformComponent>(camId, camTransform);

    LightComponent light;
    int lightId = numCubes + 1;
    entityManager.CreateEntity(lightId);
    entities.push_back(Entity{uint32_t(lightId)});
    light.intensity = 1.0f;
    entityManager.AddComponent<LightComponent>(lightId, light);
    TransformComponent lightTransform;
    lightTransform.position = glm::vec3(gridSize * spacing / 2, gridSize * spacing / 2, 5.0f * spacing);
    lightTransform.rotation = glm::vec3(0.0f);
    lightTransform.scale = glm::vec3(1.0f);
    entityManager.AddComponent<TransformComponent>(lightId, lightTransform);
    MaterialComponent lightMat;
    lightMat.setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
    // lightMat.setColor(glm::vec3(1.0f, 1.0f, 1.0f));
    entityManager.AddComponent<MaterialComponent>(lightId, lightMat);
    std::cout << "--- Benchmark scene with " << numCubes << " cubes loaded. ---" << std::endl;
}



// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
int nbFrames = 0;

float benchmarkTimer = 0.0f; // temps cumulé depuis le dernier chargement
float benchmarkInterval = 15.0f; // 15 secondes
int sizeBenchmark = 50;
float moyenneFPS = 0.0f;
bool benchmarkStarted = false;


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
        entityManager = EntityManager(); // reset de l'EntityManager
        loadBenchmarkScene(sizeBenchmark*=1.5);
        std::cout << "--- Benchmark scene with " << sizeBenchmark << " cubes loaded. ---" << std::endl;
        benchmarkTimer = 0.0f; // reset du timer
        moyenneFPS = 0.0f;         // reset de la moyenne
        lastFrame = glfwGetTime(); // reset du lastFrame pour éviter un gros deltaTime
        benchmarkStarted = true;
    }
}

// Fonction pour afficher le compteur de FPS
double affiche(GLFWwindow *window,double lastTime){
    double currentTime = glfwGetTime();
    nbFrames++;
    if ( currentTime - lastTime >= 1.0 ){ 
        double fps = double(nbFrames);
        std::string title = "Engine - FPS : " + std::to_string(fps);
        glfwSetWindowTitle(window, title.c_str());
        nbFrames = 0;
        lastTime += 1.0;
    }
    return lastTime;
}

static bool hotReloadKey = false;
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS && !hotReloadKey){
        hotReloadKey = true;
        scenePath = gameFolder + "/scene.json";
        hotReload();
        gRayTracerSystem->changeScene(entities);
    }
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE && hotReloadKey){
        hotReloadKey = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    if(gRayTracerSystem){
        gRayTracerSystem->resize(width, height);
    }
}

int main( int argc, char* argv[] )
{
    if (argc < 3) {
        std::cerr << "❌ Usage: ./Engine <game_folder> <mode>" << std::endl;
        return -1;
    }

    gameFolder = argv[1];
    if(argc >= 3) mode = argv[2];
    if(argc >= 4) mode2 = argv[3];
    // scenePath = gameFolder + "/scene.json";
    scenePath = gameFolder + "/cornelBox.json";

    // Test Lua integration
    // 1. Créer un nouvel état Lua
    lua_State* L = luaL_newstate();

    if (!L) {
        std::cerr << "Impossible de créer l'état Lua" << std::endl;
        return 1;
    }

    // 2. Charger les librairies standard
    luaL_openlibs(L);

    // 3. Exécuter un petit script Lua
    const char* script = R"(
        print("Hello from Lua!")
        a = 42
        b = 58
        print("a + b =", a + b)
    )";

    if (luaL_dostring(L, script) != LUA_OK) {
        std::cerr << "Erreur Lua: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1); // retirer l'erreur de la stack
    }

    // 4. Fermer l'état Lua
    lua_close(L);


    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }
    if (!glewIsSupported("GL_ARB_texture_non_power_of_two")) {
        std::cerr << "Your hardware does not support Non-Power-Of-Two textures." << std::endl;
    }

    // glfwWindowHint(GLFW_SAMPLES, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    int count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    // const GLFWvidmode* modeVid = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int x, y;
    glfwGetMonitorPos(monitors[0], &x, &y);
    std::cout<<"POSITION X : "<<x<<" Y : "<<y<<std::endl;
    const GLFWvidmode* modeVid = glfwGetVideoMode(monitors[0]);
    if(modeVid){
        SCR_WIDTH = std::max(MIN_WINDOW_WIDTH, static_cast<unsigned int>(modeVid->width * 0.75));
        SCR_HEIGHT = std::max(MIN_WINDOW_HEIGHT, static_cast<unsigned int>(modeVid->height * 0.75));
    } else {
        SCR_WIDTH = MIN_WINDOW_WIDTH;
        SCR_HEIGHT = MIN_WINDOW_HEIGHT;
    }

    // Open a window and create its OpenGL context
    window = glfwCreateWindow( SCR_WIDTH, SCR_HEIGHT, "Engine - GLFW", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwShowWindow(window);

    

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // ...existing code...
    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    //  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, SCR_WIDTH/2, SCR_HEIGHT/2);

    // Dark blue background
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    GLuint programID = LoadShaders( "Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl" );


    // For speed computation
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    // === 1. Définition des sommets du carré (2D centré sur l'origine) ===
    float squareVertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };

    // Indices pour former deux triangles
    unsigned short squareIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // === 2. Création du VAO/VBO ===
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // VBO : positions
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), squareVertices, GL_STATIC_DRAW);

    // EBO : indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW);

    // Layout (location = 0 dans le shader)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);


    input.onCreate(window);
    std::cout << "--- Loading scene... ---" << std::endl;
    if (mode == "-b" || mode2 == "-b") {
        loadBenchmarkScene(sizeBenchmark);
        std::cout << "--- Benchmark scene with " << sizeBenchmark << " cubes loaded. ---" << std::endl;
        lastFrame = glfwGetTime();
        benchmarkStarted = true;
    } else {
        loadScene();
    }
    std::cout << "--- Scene loaded. ---" << std::endl;

    bool anyActiveCamera = false;
    for (const auto &kv : entityManager.GetComponents<CameraComponent>()){
        if (kv.second.isActive) {
            anyActiveCamera = true;
            break;
        }
    }
    if(!anyActiveCamera){
        uint32_t camEntityId = UINT32_MAX;
        for (const auto &kv : entityManager.GetComponents<CameraComponent>()) if (kv.first < camEntityId) camEntityId = kv.first;
        entityManager.GetComponent<CameraComponent>(camEntityId).isActive = true;
    }
    std::cout << "--- Initialization... ---" << std::endl;
    RenderSystem renderSystem(&entityManager, programID, &dispatcher);
    scriptSystem.registerDispatcher(&dispatcher);
    scriptSystem.registerEntities(&entities);
    LightSystem lightSystem(&entityManager, programID);
    lightSystem.update();
    TransformSystem transformSystem(&entityManager);
    transformSystem.update();
    RayTracerSystem rayTracerSystem(&entityManager);
    gRayTracerSystem = &rayTracerSystem;
    rayTracerSystem.resize(SCR_WIDTH, SCR_HEIGHT);
    if(!rayTracerSystem.initialize()){
        std::cerr << "Failed to initialize RayTracerSystem." << std::endl;
    }
    rayTracerSystem.onCreate(entities);
    scriptSystem.registerEntityManager(&entityManager);
    input.setScriptSystem(&scriptSystem);
    input.setRenderSystem(&renderSystem);
    std::cout << "--- Systems initialized. ---" << std::endl;

    glfwSetWindowPos(window, x + 100, y + 100);

    do{
        lastTime = affiche(window,lastTime);
        // Measure speed
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);
        input.update(window,deltaTime);
        if(mode == "-b" || mode2 == "-b"){
            updateBenchmark(deltaTime);
        }
        
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        {
            const auto &camMap = entityManager.GetComponents<CameraComponent>();
            const auto &transMap = entityManager.GetComponents<TransformComponent>();
            uint32_t activeCamId = UINT32_MAX;
            for (const auto &kv : camMap) if (kv.second.isActive) { activeCamId = kv.first; break; }
            if (activeCamId != UINT32_MAX) {
                glm::vec3 listenerPos(0.0f);
                auto tIt = transMap.find(activeCamId);
                if (tIt != transMap.end()) {
                    listenerPos = tIt->second.position;
                } else {
                    listenerPos = glm::vec3(0.0f);
                }
                const auto &cam = camMap.at(activeCamId);
                glm::vec3 listenerForward = cam.target;
                glm::vec3 listenerUp = cam.up;
                audioSystem.updateListener(listenerPos, listenerForward, listenerUp);
            }
        }
        glUseProgram(programID);
        audioSystem.update();
        lightSystem.update();
        scriptSystem.onUpdate(deltaTime);
        transformSystem.update();
#if defined(__APPLE__) || defined(MACOSX)
        renderSystem.update(entities);
#else
        if(mode == "-r" || mode2 == "-r"){
            rayTracerSystem.update(entities);
        } else {
            renderSystem.update(entities);
        }
#endif

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );

    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VAO);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}
