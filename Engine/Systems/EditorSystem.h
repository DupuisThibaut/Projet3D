#ifndef EDITORSYSTEM_H
#define EDITORSYSTEM_H

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "EntityManager.h"
#include "../Entities/Entity.h"
#include <iostream>
#include <filesystem>

#if defined(__APPLE__) || defined(__MACH__)
    #define PLATFORM_MACOS
#elif defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#endif

class EditorSystem {
public:
    EditorSystem(EntityManager* entityManager, std::vector<Entity>* entities, const std::string& ScenePath= "../Jeu/scene.json", const std::string& gameFolder = "../Jeu/")  {
        this->entityManager = entityManager;
        this->entities = entities;
        selectedEntityId = UINT32_MAX;
        showHierarchy = true;
        showInspector = true;
        showStats = true;
        showMenuBar = true;
        hierarchyWidth = 0.10f;
        inspectorWidth = 0.15f;
        windowWidth = 800.0f;
        windowHeight = 600.0f;
        menuBarHeight = 0.0f;
        GameFolder = gameFolder;
        scenePath = ScenePath;
    }
    ~EditorSystem(){}

    void saveLayout(){
        std::ofstream file("editor_layout.json");
        if (file.is_open()) {
            file << "{\n";
            file << "  \"hierarchyWidth\": " << hierarchyWidth << ",\n";
            file << "  \"inspectorWidth\": " << inspectorWidth << ",\n";
            file << "  \"theme\": " << currentTheme << "\n";
            file << "}\n";
            file.close();
            std::cout << "Layout saved" << std::endl;
        }
    }

    void loadLayout(){
        std::ifstream file("editor_layout.json");
        if(file.is_open()){
            std::string line;
            while(std::getline(file,line)){
                if (line.find("hierarchyWidth") != std::string::npos) {
                    size_t pos = line.find(": ");
                    if (pos != std::string::npos) {
                        hierarchyWidth = std::stof(line.substr(pos + 2));
                    }
                }
                if (line.find("inspectorWidth") != std::string::npos) {
                    size_t pos = line.find(": ");
                    if (pos != std::string::npos) {
                        inspectorWidth = std::stof(line.substr(pos + 2));
                    }
                }
                if (line.find("theme") != std::string::npos) {
                    size_t pos = line.find(": ");
                    if (pos != std::string::npos) {
                        currentTheme = std::stoi(line.substr(pos + 2));
                    }
                }
            }
            file.close();
            std::cout<< "Layout loaded" << std::endl;
        }
    }

    // Initialisation
    void initialize(GLFWwindow* window){
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        // Style
        scanSystemFonts();
        if(!availableFonts.empty()){
            loadCustomFont(availableFonts[0].second.c_str(), 16.0f);
        } else {
            loadCustomFont(nullptr, 16.0f);
        }
        loadLayout();
        applyTheme(currentTheme);
        
        std::cout << "EditorSystem initialized (ImGui " << IMGUI_VERSION << ")" << std::endl;
    }
    void shutdown(){
        saveLayout();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        std::cout << "EditorSystem shutdown" << std::endl;
    }

    void applyTheme(int themeIndex = 0){
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;
        
        switch(themeIndex){
            case 0: // Dark Theme (défaut amélioré)
                applyDarkTheme(style, colors);
                break;
            case 1: // Light Theme
                applyLightTheme(style, colors);
                break;
            case 2: // Unity-like Theme
                applyUnityTheme(style, colors);
                break;
            case 3: // Unreal-like Theme
                applyUnrealTheme(style, colors);
                break;
            case 4: // Dracula Theme
                applyDraculaTheme(style, colors);
                break;
            case 5: // Nord Theme
                applyNordTheme(style, colors);
                break;
        }
    }

    void processFontReload(){
        if (fontReloadRequested) {
            if (pendingFontPath.empty()) {
                loadCustomFont(nullptr, pendingFontSize);
            } else {
                loadCustomFont(pendingFontPath.c_str(), pendingFontSize);
            }
            fontReloadRequested = false;
        }
    }

    // Appelé au début de chaque frame
    void beginFrame(){
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        ImGuiIO& io = ImGui::GetIO();
        windowWidth = io.DisplaySize.x;
        windowHeight = io.DisplaySize.y;

        static bool printed = false;
        if (!printed) {
            std::cout << "=== Display Info ===" << std::endl;
            std::cout << "Window size (logical): " << windowWidth << "x" << windowHeight << std::endl;
            std::cout << "Framebuffer scale: " << io.DisplayFramebufferScale.x << "x" << io.DisplayFramebufferScale.y << std::endl;
            std::cout << "Framebuffer size (physical): " 
                    << (windowWidth * io.DisplayFramebufferScale.x) << "x" 
                    << (windowHeight * io.DisplayFramebufferScale.y) << std::endl;
            printed = true;
        }
    }
    
    // Rendu de l'interface
    void renderHierarchy(){
        if (!showHierarchy) return;
        ImGui::SetNextWindowPos(ImVec2(0, menuBarHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(hierarchyWidth * windowWidth, windowHeight-menuBarHeight), ImGuiCond_Always);
        ImGui::Begin("Hierarchy", &showHierarchy, 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus
        );
        ImGui::Text("Entities (%zu)", entities->size());
        ImGui::SameLine();
        float buttonWidth = 25.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonWidth - ImGui::GetStyle().FramePadding.x);

        if(ImGui::Button("+", ImVec2(buttonWidth, 0))){
            Entity newEntity;
            newEntity.id = entityManager->CreateEntity();
            TransformComponent transform;
            entityManager->AddComponent(newEntity.id, transform);
            entities->push_back(newEntity);
            selectedEntityId = newEntity.id;
        }
        if(ImGui::IsItemHovered()){
            ImGui::BeginTooltip();
            ImGui::Text("Add New Entity");
            ImGui::EndTooltip();
        }
        ImGui::Separator();
        for (const auto& entity : *entities) {
            renderEntityNode(entity);
        }
        ImGui::End();
    }
    void renderInspector(){
        if (!showInspector) return;
        float inspectorX = windowWidth - inspectorWidth * windowWidth;
        ImGui::SetNextWindowPos(ImVec2(inspectorX, menuBarHeight), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(inspectorWidth*windowWidth, windowHeight - menuBarHeight), ImGuiCond_Always);
        ImGui::Begin("Inspector", &showInspector, 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoCollapse
        );
        if (selectedEntityId != UINT32_MAX) {
            ImGui::Text("Entity ID: %u", selectedEntityId);
            ImGui::Separator();
            // Transform
            if (entityManager->HasComponent<TransformComponent>(selectedEntityId)) {
                renderTransformComponent(selectedEntityId);
            }
            // Camera
            if (entityManager->HasComponent<CameraComponent>(selectedEntityId)) {
                renderCameraComponent(selectedEntityId);
            }
            // Light
            if (entityManager->HasComponent<LightComponent>(selectedEntityId)) {
                renderLightComponent(selectedEntityId);
            }
            // Material
            if (entityManager->HasComponent<MaterialComponent>(selectedEntityId)) {
                renderMaterialComponent(selectedEntityId);
            }
            // Mesh
            if (entityManager->HasComponent<MeshComponent>(selectedEntityId)) {
                renderMeshComponent(selectedEntityId);
            }
            // Audio
            if (entityManager->HasComponent<MyAudioComponent>(selectedEntityId)) {
                renderAudioComponent(selectedEntityId);
            }
            // Script
            if (entityManager->HasComponent<LuaScriptComponent>(selectedEntityId)) {
                renderLuaScriptComponent(selectedEntityId);
            }
            // Tag
            if (entityManager->HasComponent<TagComponent>(selectedEntityId)) {
                renderTagComponent(selectedEntityId);
            }
            // Layer
            if (entityManager->HasComponent<LayerComponent>(selectedEntityId)) {
                renderLayerComponent(selectedEntityId);
            }
        } else {
            ImGui::TextDisabled("No entity selected");
        }
        float buttonWidth = 120.0f;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonWidth - ImGui::GetStyle().FramePadding.x);
        if(selectedEntityId != UINT32_MAX){
            if(ImGui::Button("Add Component")){
                ImGui::OpenPopup("AddComponentPopup");
            }
            if(ImGui::BeginPopup("AddComponentPopup")){
                ImGui::Text("Add Component");
                ImGui::Separator();
                if(!entityManager->HasComponent<TransformComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Transform Component")){
                        TransformComponent transform;
                        entityManager->AddComponent<TransformComponent>(selectedEntityId, transform);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if(!entityManager->HasComponent<MeshComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Mesh Component")){
                        MeshComponent mesh;
                        entityManager->AddComponent<MeshComponent>(selectedEntityId, mesh);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if(!entityManager->HasComponent<CameraComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Camera Component")){
                        CameraComponent camera;
                        entityManager->AddComponent<CameraComponent>(selectedEntityId, camera);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if(!entityManager->HasComponent<LightComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Light Component")){
                        LightComponent light;
                        entityManager->AddComponent<LightComponent>(selectedEntityId, light);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if(!entityManager->HasComponent<MaterialComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Material Component")){
                        MaterialComponent material;
                        entityManager->AddComponent<MaterialComponent>(selectedEntityId, material);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if(!entityManager->HasComponent<MyAudioComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Audio Component")){
                        MyAudioComponent audio;
                        entityManager->AddComponent<MyAudioComponent>(selectedEntityId, audio);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if(!entityManager->HasComponent<LuaScriptComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Script Component")){
                        LuaScriptComponent script;
                        entityManager->AddComponent<LuaScriptComponent>(selectedEntityId, script);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if(!entityManager->HasComponent<TagComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Tag Component")){
                        TagComponent tag;
                        entityManager->AddComponent<TagComponent>(selectedEntityId, tag);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if(!entityManager->HasComponent<LayerComponent>(selectedEntityId)){
                    if(ImGui::MenuItem("Layer Component")){
                        LayerComponent layer;
                        entityManager->AddComponent<LayerComponent>(selectedEntityId, layer);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }

        }
        ImGui::End();
    }
    void renderMenuBar(){
        if (!showMenuBar){
            menuBarHeight = 0.0f;
            return;
        }
        if (ImGui::BeginMainMenuBar()) {
            if(ImGui::BeginMenu("File")){
                if (ImGui::MenuItem("Save Scene", "CTRL+S")) {
                    saveScene();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(glfwGetCurrentContext(), GL_TRUE);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy);
                ImGui::MenuItem("Inspector", nullptr, &showInspector);
                ImGui::MenuItem("Stats", nullptr, &showStats);
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Theme")){
                if (ImGui::MenuItem("Dark", nullptr, currentTheme == 0)) {
                    currentTheme = 0;
                    applyTheme(0);
                }
                if (ImGui::MenuItem("Light", nullptr, currentTheme == 1)) {
                    currentTheme = 1;
                    applyTheme(1);
                }
                if (ImGui::MenuItem("Unity", nullptr, currentTheme == 2)) {
                    currentTheme = 2;
                    applyTheme(2);
                }
                if (ImGui::MenuItem("Unreal", nullptr, currentTheme == 3)) {
                    currentTheme = 3;
                    applyTheme(3);
                }
                if (ImGui::MenuItem("Dracula", nullptr, currentTheme == 4)) {
                    currentTheme = 4;
                    applyTheme(4);
                }
                if (ImGui::MenuItem("Nord", nullptr, currentTheme == 5)) {
                    currentTheme = 5;
                    applyTheme(5);
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu("Font")){
                if (ImGui::MenuItem("Default", nullptr, currentFontPath.empty())) {
                    fontReloadRequested = true;
                    pendingFontPath = "";
                    pendingFontSize = currentFontSize;
                }
                
                ImGui::Separator();
                ImGui::Text("System Fonts:");
                
                if (availableFonts.empty()) {
                    ImGui::TextDisabled("No fonts found");
                } else {
                    for (const auto& [name, path] : availableFonts) {
                        bool isSelected = (currentFontPath == path);
                        if (ImGui::MenuItem(name.c_str(), nullptr, isSelected)) {
                            fontReloadRequested = true;
                            pendingFontPath = path;
                            pendingFontSize = currentFontSize;
                        }
                    }
                }
                
                ImGui::Separator();
                ImGui::Text("Font Size:");
                if (ImGui::SliderFloat("##fontsize", &currentFontSize, 10.0f, 30.0f, "%.0f px")) {
                }
                if(ImGui::IsItemDeactivatedAfterEdit()){
                    fontReloadRequested = true;
                    pendingFontPath = currentFontPath.empty() ? "" : currentFontPath;
                    pendingFontSize = currentFontSize;
                }
                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Execute")){
                if (ImGui::MenuItem("Play Mode")) {
                }
                ImGui::EndMenu();
            }
            menuBarHeight = ImGui::GetWindowSize().y;
            ImGui::EndMainMenuBar();
        }
    }
    void renderStats(float deltaTime){
        if (!showStats) return;
        ImGui::SetNextWindowPos(ImVec2(windowWidth - 200, windowHeight - 80), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(190, 70), ImGuiCond_Always);
        ImGui::Begin("Stats", &showStats, 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoTitleBar
        );
        
        ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
        ImGui::Text("Frame time: %.2f ms", deltaTime * 1000.0f);
        ImGui::Text("Entities: %zu", entities->size());
        
        ImGui::End();
    }


    void renderLeftSplitter(){
        if (!showHierarchy) return;
        
        float splitterX = windowWidth * hierarchyWidth;
        
        ImGui::SetCursorScreenPos(ImVec2(splitterX - splitterWidth/2, menuBarHeight));
        ImGui::InvisibleButton("##LeftSplitter", ImVec2(splitterWidth, windowHeight - menuBarHeight));
        
        bool isHovered = ImGui::IsItemHovered();
        bool isActive = ImGui::IsItemActive();
        
        if (isHovered || isDraggingLeftSplitter) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        
        if (isActive && ImGui::IsMouseClicked(0)) {
            isDraggingLeftSplitter = true;
        }
        
        if (isDraggingLeftSplitter) {
            if (ImGui::IsMouseDown(0)) {
                ImGuiIO& io = ImGui::GetIO();
                float newWidth = io.MousePos.x / windowWidth;
                hierarchyWidth = std::max(minPanelWidth, std::min(newWidth, maxPanelWidth));
                
                // ✅ Affiche un tooltip avec le pourcentage
                ImGui::BeginTooltip();
                ImGui::Text("%.0f%%", hierarchyWidth * 100.0f);
                ImGui::EndTooltip();
            } else {
                isDraggingLeftSplitter = false;
            }
        }
        
        // Dessine la ligne du splitter
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        
        // ✅ Ligne principale
        ImU32 lineColor = isDraggingLeftSplitter 
            ? IM_COL32(100, 150, 255, 255)  // Bleu vif pendant drag
            : isHovered 
                ? IM_COL32(100, 150, 255, 180)  // Bleu transparent au hover
                : IM_COL32(60, 60, 60, 180);    // Gris par défaut
        
        float lineWidth = isDraggingLeftSplitter ? 3.0f : (isHovered ? 2.0f : 1.0f);
        
        drawList->AddLine(
            ImVec2(splitterX, menuBarHeight),
            ImVec2(splitterX, windowHeight),
            lineColor,
            lineWidth
        );
        
        // ✅ Zone de highlight au hover
        if (isHovered || isDraggingLeftSplitter) {
            drawList->AddRectFilled(
                ImVec2(splitterX - 2, menuBarHeight),
                ImVec2(splitterX + 2, windowHeight),
                IM_COL32(100, 150, 255, 50)
            );
        }
    }

    void renderRightSplitter(){
        if (!showInspector) return;
        
        float splitterX = windowWidth * (1.0f - inspectorWidth);
        
        ImGui::SetCursorScreenPos(ImVec2(splitterX - splitterWidth/2, menuBarHeight));
        ImGui::InvisibleButton("##RightSplitter", ImVec2(splitterWidth, windowHeight - menuBarHeight));
        
        bool isHovered = ImGui::IsItemHovered();
        bool isActive = ImGui::IsItemActive();
        
        if (isHovered || isDraggingRightSplitter) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        
        if (isActive && ImGui::IsMouseClicked(0)) {
            isDraggingRightSplitter = true;
        }
        
        if (isDraggingRightSplitter) {
            if (ImGui::IsMouseDown(0)) {
                ImGuiIO& io = ImGui::GetIO();
                float newWidth = (windowWidth - io.MousePos.x) / windowWidth;
                inspectorWidth = std::max(minPanelWidth, std::min(newWidth, maxPanelWidth));
                
                // ✅ Affiche un tooltip avec le pourcentage
                ImGui::BeginTooltip();
                ImGui::Text("%.0f%%", inspectorWidth* 100.0f);
                ImGui::EndTooltip();
            } else {
                isDraggingRightSplitter = false;
            }
        }
        
        // Dessine la ligne du splitter
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        
        // ✅ Ligne principale
        ImU32 lineColor = isDraggingRightSplitter 
            ? IM_COL32(100, 150, 255, 255)  // Bleu vif pendant drag
            : isHovered 
                ? IM_COL32(100, 150, 255, 180)  // Bleu transparent au hover
                : IM_COL32(60, 60, 60, 180);    // Gris par défaut
        
        float lineWidth = isDraggingRightSplitter ? 3.0f : (isHovered ? 2.0f : 1.0f);
        
        drawList->AddLine(
            ImVec2(splitterX, menuBarHeight),
            ImVec2(splitterX, windowHeight),
            lineColor,
            lineWidth
        );
        
        // ✅ Zone de highlight au hover
        if (isHovered || isDraggingRightSplitter) {
            drawList->AddRectFilled(
                ImVec2(splitterX - 2, menuBarHeight),
                ImVec2(splitterX + 2, windowHeight),
                IM_COL32(100, 150, 255, 50)
            );
        }
    }
    
    // Appelé à la fin de chaque frame
    void endFrame(){
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Configuration du viewport pour le rendu 3D
    void configureViewport(float& viewportX, float& viewportWidth, float& viewportHeight, unsigned int& width, unsigned int& height){
        float float_hierarchyWidth = showHierarchy ? (windowWidth * hierarchyWidth) : 0.0f;
        float float_inspectorWidth = showInspector ? (windowWidth * inspectorWidth) : 0.0f;
        
        viewportX = float_hierarchyWidth;
        viewportWidth = windowWidth - float_hierarchyWidth - float_inspectorWidth;
        viewportHeight = windowHeight - menuBarHeight;
        width = static_cast<unsigned int>(viewportWidth);
        height = static_cast<unsigned int>(viewportHeight);
        
        ImGuiIO& io = ImGui::GetIO();
        float retinaScale = io.DisplayFramebufferScale.x;
        
        int fbViewportX = static_cast<int>(viewportX * retinaScale);
        int fbViewportY = static_cast<int>(menuBarHeight * retinaScale);
        int fbViewportWidth = static_cast<int>(viewportWidth * retinaScale);
        int fbViewportHeight = static_cast<int>(viewportHeight * retinaScale);
        
        glEnable(GL_SCISSOR_TEST);
        glScissor(fbViewportX, fbViewportY, fbViewportWidth, fbViewportHeight);
        glViewport(fbViewportX, fbViewportY, fbViewportWidth, fbViewportHeight);
    }
    
    // Restaure le viewport complet après le rendu 3D
    void restoreViewport(){
        ImGuiIO& io = ImGui::GetIO();
        float retinaScale = io.DisplayFramebufferScale.x;
        
        int fbWidth = static_cast<int>(windowWidth * retinaScale);
        int fbHeight = static_cast<int>(windowHeight * retinaScale);
        
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, fbWidth, fbHeight);
    }

    // Getters
    uint32_t getSelectedEntityId() const { return selectedEntityId; }
    bool isHierarchyVisible() const { return showHierarchy; }
    bool isInspectorVisible() const { return showInspector; }
    float getHierarchyWidth() const { return hierarchyWidth; }
    float getInspectorWidth() const { return inspectorWidth; }

private:
    EntityManager* entityManager;
    std::vector<Entity>* entities;
    std::string GameFolder;
    std::string scenePath;
    
    // État de l'éditeur
    uint32_t selectedEntityId;
    bool showHierarchy;
    bool showInspector;
    bool showStats;
    bool showMenuBar;
    
    // Dimensions des panneaux
    float hierarchyWidth;
    float inspectorWidth;
    float windowWidth;
    float windowHeight;
    float menuBarHeight;

    // Splitter
    bool isDraggingLeftSplitter = false;
    bool isDraggingRightSplitter = false;
    const float splitterWidth =8.0f;
    const float minPanelWidth = 0.05f; // 5%
    const float maxPanelWidth = 0.5f;  // 50%

    // Gestion des fonts
    std::string currentFontPath = "";
    float currentFontSize = 16.0f;
    std::vector<std::pair<std::string, std::string>> availableFonts;

    bool fontReloadRequested = false;
    std::string pendingFontPath = "";
    float pendingFontSize = 16.0f;

    std::vector<std::string> getSystemFontPaths(){
        std::vector<std::string> fontDirs;
        
        #ifdef PLATFORM_MACOS
            fontDirs.push_back("/System/Library/Fonts/");
            fontDirs.push_back("/Library/Fonts/");
            fontDirs.push_back(std::string(getenv("HOME")) + "/Library/Fonts/");
        #endif
        
        #ifdef PLATFORM_WINDOWS
            // Récupère le chemin Windows
            char* windir = getenv("WINDIR");
            if (windir) {
                fontDirs.push_back(std::string(windir) + "\\Fonts\\");
            } else {
                fontDirs.push_back("C:\\Windows\\Fonts\\");
            }
        #endif
        
        #ifdef PLATFORM_LINUX
            fontDirs.push_back("/usr/share/fonts/");
            fontDirs.push_back("/usr/local/share/fonts/");
            fontDirs.push_back(std::string(getenv("HOME")) + "/.fonts/");
            fontDirs.push_back(std::string(getenv("HOME")) + "/.local/share/fonts/");
        #endif
        
        return fontDirs;
    }

    void scanSystemFonts(){
        availableFonts.clear();
        
        // Liste des polices à chercher (nom → fichiers possibles)
        std::vector<std::pair<std::string, std::vector<std::string>>> fontsToFind = {
            // Polices universelles
            {"Arial", {"Arial.ttf", "arial.ttf", "Arial.TTF"}},
            {"Times New Roman", {"Times New Roman.ttf", "times.ttf", "TimesNewRoman.ttf"}},
            {"Courier New", {"Courier New.ttf", "cour.ttf", "CourierNew.ttf"}},
            {"Verdana", {"Verdana.ttf", "verdana.ttf", "Verdana.TTF"}},
            
            #ifdef PLATFORM_MACOS
                {"San Francisco", {"SFNS.ttf", "SFNSDisplay.ttf"}},
                {"Helvetica", {"Helvetica.ttc", "Helvetica.ttf"}},
                {"Monaco", {"Monaco.dfont", "Monaco.ttf"}},
                {"Menlo", {"Menlo.ttc", "Menlo.ttf"}},
            #endif
            
            #ifdef PLATFORM_WINDOWS
                {"Segoe UI", {"segoeui.ttf", "SegoeUI.ttf"}},
                {"Consolas", {"consola.ttf", "Consolas.ttf"}},
                {"Calibri", {"calibri.ttf", "Calibri.ttf"}},
            #endif
            
            #ifdef PLATFORM_LINUX
                {"DejaVu Sans", {"DejaVuSans.ttf", "DejaVuSans-Regular.ttf"}},
                {"Ubuntu", {"Ubuntu-R.ttf", "Ubuntu-Regular.ttf"}},
                {"Liberation Sans", {"LiberationSans-Regular.ttf"}},
                {"Noto Sans", {"NotoSans-Regular.ttf"}},
            #endif
        };
        std::vector<std::string> fontDirs = getSystemFontPaths();
        
        // Cherche chaque police dans les dossiers système
        for (const auto& [fontName, fileNames] : fontsToFind) {
            for (const auto& dir : fontDirs) {
                for (const auto& fileName : fileNames) {
                    std::string fullPath = dir + fileName;
                    if (std::ifstream(fullPath).good()) {
                        availableFonts.push_back({fontName, fullPath});
                        std::cout << "Found font: " << fontName << " at " << fullPath << std::endl;
                        goto nextFont; // Police trouvée, passe à la suivante
                    }
                }
            }
            nextFont:;
        }
        
        std::cout << "Total fonts found: " << availableFonts.size() << std::endl;
    }


    void loadCustomFont(const char* fontPath = nullptr, float fontSize = 16.0f){
        ImGuiIO& io = ImGui::GetIO();
        
        // ✅ Vérifie que l'atlas n'est pas verrouillé
        if (io.Fonts->Locked) {
            std::cerr << "Cannot load font: ImFontAtlas is locked!" << std::endl;
            return;
        }
        
        if (!io.Fonts->Fonts.empty()) {
            io.Fonts->Clear();
        }
        
        if (fontPath == nullptr) {
            // Police par défaut
            io.Fonts->AddFontDefault();
            currentFontPath = "";
            std::cout << "Loaded default font (size: " << fontSize << ")" << std::endl;
        } else {
            // Vérifie si le fichier existe
            if (std::ifstream(fontPath).good()) {
                ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, fontSize);
                if (font != nullptr) {
                    currentFontPath = fontPath;
                    currentFontSize = fontSize;
                    std::cout << "Loaded font: " << fontPath << " (size: " << fontSize << ")" << std::endl;
                } else {
                    std::cerr << "Failed to load font: " << fontPath << std::endl;
                    io.Fonts->AddFontDefault();
                    currentFontPath = "";
                }
            } else {
                std::cerr << "Font file not found: " << fontPath << std::endl;
                io.Fonts->AddFontDefault();
                currentFontPath = "";
            }
        }
        
        // ✅ Reconstruit l'atlas de polices
        io.Fonts->Build();
        
        // ✅ Recrée la texture OpenGL
        ImGui_ImplOpenGL3_DestroyFontsTexture();
        ImGui_ImplOpenGL3_CreateFontsTexture();
    }
    
    // Méthodes privées d'aide
    void renderEntityNode(const Entity& entity){
        std::string label = "Entity " + std::to_string(entity.id);
        
        if (ImGui::Selectable(label.c_str(), selectedEntityId == entity.id)) {
            selectedEntityId = entity.id;
        }
    }

    void renderTransformComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& transform = const_cast<TransformComponent&>(
                entityManager->GetComponent<TransformComponent>(entityId)
            );
            ImGui::Text("Hierarchy");
            ImGui::Separator();        
            std::string parentLabel = "None";
            if (transform.parent != UINT32_MAX) {
                parentLabel = "Entity " + std::to_string(transform.parent);
            }
            // ImGui::Text("Parent: %s", parentLabel.c_str());
            // ImGui::SameLine();        
            // if (ImGui::Button("Change##parent")) {
            //     ImGui::OpenPopup("SelectParentPopup");
            // }        
            // if (ImGui::BeginPopup("SelectParentPopup")) {
            //     ImGui::Text("Select Parent Entity");
            //     ImGui::Separator();            
            //     if (ImGui::Selectable("None", transform.parentId == UINT32_MAX)) {
            //         setParent(entityId, UINT32_MAX);
            //         ImGui::CloseCurrentPopup();
            //     }            
            //     for (const auto& entity : *entities) {
            //         if (entity.id == entityId) continue;               
            //         if (isDescendant(entity.id, entityId)) continue;
            //         std::string label = "Entity " + std::to_string(entity.id);
            //         bool isSelected = (transform.parentId == entity.id);
                    
            //         if (ImGui::Selectable(label.c_str(), isSelected)) {
            //             setParent(entityId, entity.id);
            //             ImGui::CloseCurrentPopup();
            //         }
            //     }
                
            //     ImGui::EndPopup();
            // }        
            // if (!transform.childrenIds.empty()) {
            //     ImGui::Text("Children (%zu):", transform.childrenIds.size());
            //     ImGui::Indent();
            //     for (uint32_t childId : transform.childrenIds) {
            //         ImGui::Text("- Entity %u", childId);
            //         ImGui::SameLine();
            //         if (ImGui::SmallButton(("Select##" + std::to_string(childId)).c_str())) {
            //             selectedEntityId = childId;
            //         }
            //     }
            //     ImGui::Unindent();
            // }
            ImGui::Separator();
            ImGui::Text("Local Transform");
            ImGui::DragFloat3("Position", &transform.position.x, 0.1f);
            ImGui::DragFloat3("Rotation", &transform.rotation.x, 1.0f);
            ImGui::DragFloat3("Scale", &transform.scale.x, 0.01f, 0.001f, 100.0f);
            if (ImGui::Button("Reset Transform")) {
                transform.position = glm::vec3(0.0f);
                transform.rotation = glm::vec3(0.0f);
                transform.scale = glm::vec3(1.0f);
            }
        }
    }
    void renderCameraComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& camera = const_cast<CameraComponent&>(
                entityManager->GetComponent<CameraComponent>(entityId)
            );
            
            ImGui::Checkbox("Active", &camera.isActive);
            ImGui::DragFloat("FOV", &camera.fov, 0.1f, 1.0f, 179.0f);
            ImGui::DragFloat("Near Plane", &camera.nearPlane, 0.01f, 0.01f, 1000.0f);
            ImGui::DragFloat("Far Plane", &camera.farPlane, 1.0f, 1.0f, 10000.0f);
            ImGui::Text("Aspect Ratio: %.2f", camera.aspectRatio);
        }
    }
    void renderLightComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& light = const_cast<LightComponent&>(
                entityManager->GetComponent<LightComponent>(entityId)
            );
            
            ImGui::DragFloat("Intensity", &light.intensity, 0.1f, 0.0f, 100.0f);
        }
    }

    void renderMaterialComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& material = const_cast<MaterialComponent&>(
                entityManager->GetComponent<MaterialComponent>(entityId)
            );
            
            const char* materialTypes[] = { "None", "Texture", "Color" };
            int currentType = static_cast<int>(material.type);
            
            if (ImGui::Combo("Material Type", &currentType, materialTypes, IM_ARRAYSIZE(materialTypes))) {
                material.type = static_cast<MaterialComponent::Type>(currentType);
            }
            
            ImGui::Separator();

            if (material.type == MaterialComponent::Type::Texture) {
                ImGui::Text("Texture Settings");
                
                // Affiche le chemin de la texture actuelle
                if (!material.texturePath.empty()) {
                    ImGui::Text("Current: %s", material.texturePath.c_str());
                } else {
                    ImGui::TextDisabled("No texture loaded");
                }
                
                // Input pour le chemin de la texture
                char texturePathBuffer[256];
                strncpy(texturePathBuffer, getRelativePath(material.texturePath,GameFolder).c_str(), sizeof(texturePathBuffer));
                texturePathBuffer[sizeof(texturePathBuffer) - 1] = '\0';
                
                if (ImGui::InputText("Texture Path", texturePathBuffer, sizeof(texturePathBuffer))) {
                    material.texturePath = std::string(texturePathBuffer);
                }
                
                // Bouton pour charger la texture
                if (ImGui::Button("Load Texture")) {
                    if (!material.texturePath.empty()) {
                        material.setTexture(GameFolder + material.texturePath);
                        if (material.loadTexture()) {
                            std::cout << "Loaded texture: " << material.texturePath << std::endl;
                        } else {
                            std::cerr << "Failed to load texture: " << material.texturePath << std::endl;
                        }
                    } else {
                        std::cerr << "No texture path specified" << std::endl;
                    }
                }
                
                // Affiche l'ID de la texture OpenGL
                if (material.texture != 0) {
                    ImGui::Text("Texture ID: %u", material.texture);
                    ImGui::Text("Preview:");
                    ImVec2 previewSize(128, 128);
                    ImGui::Image((void*)(intptr_t)material.texture, previewSize);
                }
            }
            
            else if (material.type == MaterialComponent::Type::Color) {
                ImGui::Text("Color Settings");
                
                bool needsUpdate = false;
                
                needsUpdate |= ImGui::ColorEdit3("Color", &material.color.x);
                needsUpdate |= ImGui::ColorEdit3("Ambient", &material.ambient_material.x);
                needsUpdate |= ImGui::ColorEdit3("Diffuse", &material.diffuse_material.x);
                needsUpdate |= ImGui::ColorEdit3("Specular", &material.specular_material.x);
                needsUpdate |= ImGui::SliderFloat("Shininess", &material.shininess, 1.0f, 128.0f);
                
                // Bouton pour appliquer les modifications
                if (needsUpdate) {
                    std::cout << "Material color updated" << std::endl;
                }
                
                // Bouton pour réinitialiser
                if (ImGui::Button("Reset Material")) {
                    material.setColor(
                        glm::vec3(1.0f, 1.0f, 1.0f), // color
                        glm::vec3(0.8f, 0.8f, 0.8f), // diffuse
                        glm::vec3(1.0f, 1.0f, 1.0f), // specular
                        glm::vec3(0.2f, 0.2f, 0.2f), // ambient
                        32.0f                         // shininess
                    );
                    std::cout << "Material reset to default" << std::endl;
                }
            }

            else {
                ImGui::TextDisabled("No material applied");
                
                if (ImGui::Button("Set Default Color")) {
                    material.setColor(
                        glm::vec3(1.0f, 1.0f, 1.0f),
                        glm::vec3(0.8f, 0.8f, 0.8f),
                        glm::vec3(1.0f, 1.0f, 1.0f),
                        glm::vec3(0.2f, 0.2f, 0.2f),
                        32.0f
                    );
                    std::cout << "Applied default color material" << std::endl;
                }
            }
            
            ImGui::Separator();
            
            ImGui::Text("Quick Presets:");
            
            if (ImGui::Button("Gold")) {
                material.setColor(
                    glm::vec3(1.0f, 0.84f, 0.0f),    // color
                    glm::vec3(0.75f, 0.61f, 0.23f),  // diffuse
                    glm::vec3(0.63f, 0.56f, 0.37f),  // specular
                    glm::vec3(0.25f, 0.20f, 0.07f),  // ambient
                    51.2f                             // shininess
                );
                std::cout << "Applied gold material preset" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Silver")) {
                material.setColor(
                    glm::vec3(0.75f, 0.75f, 0.75f),  // color
                    glm::vec3(0.51f, 0.51f, 0.51f),  // diffuse
                    glm::vec3(0.51f, 0.51f, 0.51f),  // specular
                    glm::vec3(0.19f, 0.19f, 0.19f),  // ambient
                    51.2f                             // shininess
                );
                std::cout << "Applied silver material preset" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Bronze")) {
                material.setColor(
                    glm::vec3(0.71f, 0.40f, 0.11f),  // color
                    glm::vec3(0.71f, 0.43f, 0.18f),  // diffuse
                    glm::vec3(0.39f, 0.27f, 0.17f),  // specular
                    glm::vec3(0.21f, 0.13f, 0.05f),  // ambient
                    25.6f                             // shininess
                );
                std::cout << "Applied bronze material preset" << std::endl;
            }
            
            if (ImGui::Button("Emerald")) {
                material.setColor(
                    glm::vec3(0.07f, 0.61f, 0.08f),  // color
                    glm::vec3(0.63f, 0.73f, 0.63f),  // diffuse
                    glm::vec3(0.63f, 0.73f, 0.63f),  // specular
                    glm::vec3(0.02f, 0.17f, 0.02f),  // ambient
                    76.8f                             // shininess
                );
                std::cout << "Applied emerald material preset" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Ruby")) {
                material.setColor(
                    glm::vec3(0.61f, 0.04f, 0.04f),  // color
                    glm::vec3(0.73f, 0.03f, 0.03f),  // diffuse
                    glm::vec3(0.73f, 0.63f, 0.63f),  // specular
                    glm::vec3(0.17f, 0.01f, 0.01f),  // ambient
                    76.8f                             // shininess
                );
                std::cout << "Applied ruby material preset" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Plastic")) {
                material.setColor(
                    glm::vec3(0.8f, 0.8f, 0.8f),     // color
                    glm::vec3(0.55f, 0.55f, 0.55f),  // diffuse
                    glm::vec3(0.70f, 0.70f, 0.70f),  // specular
                    glm::vec3(0.0f, 0.0f, 0.0f),     // ambient
                    32.0f                             // shininess
                );
                std::cout << "Applied plastic material preset" << std::endl;
            }
        }
    }

    void renderAudioComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& audio = const_cast<MyAudioComponent&>(
                entityManager->GetComponent<MyAudioComponent>(entityId)
            );
            
            ImGui::SliderFloat("Volume", &audio.volume, 0.0f, 1.0f);
            ImGui::Checkbox("Loop", &audio.loop);
            ImGui::Checkbox("Play on Awake", &audio.playOnStart);
            ImGui::Checkbox("IsPlaying", &audio.isPlaying);
            char audioPathBuffer[256];
            strncpy(audioPathBuffer, audio.audioFilePath.c_str(), sizeof(audioPathBuffer));
            audioPathBuffer[sizeof(audioPathBuffer) - 1] = '\0';
            ImGui::InputText("Audio File", audioPathBuffer, sizeof(audioPathBuffer));
            audio.audioFilePath = std::string(audioPathBuffer);
        }
    }

    void renderLuaScriptComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Lua Script", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& script = const_cast<LuaScriptComponent&>(
                entityManager->GetComponent<LuaScriptComponent>(entityId)
            );
            
            char scriptPathBuffer[256];
            strncpy(scriptPathBuffer, script.luaScriptPath.c_str(), sizeof(scriptPathBuffer));
            scriptPathBuffer[sizeof(scriptPathBuffer) - 1] = '\0';
            
            if (ImGui::InputText("Script File", scriptPathBuffer, sizeof(scriptPathBuffer))) {
                script.luaScriptPath = std::string(scriptPathBuffer);
            }
        }
    }

    void renderMeshComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& mesh = const_cast<MeshComponent&>(
                entityManager->GetComponent<MeshComponent>(entityId)
            );
        
            const char* primitiveTypes[] = { "PLANE", "CUBE", "SPHERE", "CYLINDER", "MESH" };
            int currentType = static_cast<int>(mesh.type);
            
            if (ImGui::Combo("Type", &currentType, primitiveTypes, IM_ARRAYSIZE(primitiveTypes))) {
                mesh.type = static_cast<PrimitiveType>(currentType);
            }
            
            ImGui::Separator();
            
            bool needsRebuild = false;
            
            switch(mesh.type) {
                case PrimitiveType::PLANE:
                    ImGui::Text("Plane Settings");
                    needsRebuild |= ImGui::DragFloat("Width", &mesh.width, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Height", &mesh.height, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Subdivisions", &mesh.subdivisions, 1.0f, 1.0f, 200.0f);
                    needsRebuild |= ImGui::DragFloat3("Right Vector", &mesh.m_right_vector.x, 0.01f);
                    needsRebuild |= ImGui::DragFloat3("Up Vector", &mesh.m_up_vector.x, 0.01f);
                    break;
                    
                case PrimitiveType::SPHERE:
                    ImGui::Text("Sphere Settings");
                    needsRebuild |= ImGui::DragFloat("Radius", &mesh.rayon, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Subdivisions", &mesh.subdivisions, 1.0f, 4.0f, 100.0f);
                    break;
                    
                case PrimitiveType::CYLINDER:
                    ImGui::Text("Cylinder Settings");
                    needsRebuild |= ImGui::DragFloat("Radius", &mesh.width, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Height", &mesh.height, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Subdivisions", &mesh.subdivisions, 1.0f, 3.0f, 100.0f);
                    break;
                    
                case PrimitiveType::CUBE:
                    ImGui::Text("Cube (no parameters)");
                    break;
                    
                case PrimitiveType::MESH:
                    ImGui::Text("Custom Mesh");
                    
                    // Bouton pour charger un fichier OFF
                    static char offPathBuffer[256] = "";
                    ImGui::InputText("OFF File Path", offPathBuffer, sizeof(offPathBuffer));
                    
                    if (ImGui::Button("Load OFF File")) {
                        if (strlen(offPathBuffer) > 0) {
                            mesh.load_OFF(offPathBuffer);
                            std::cout << "Loaded OFF file: " << offPathBuffer << std::endl;
                        }
                    }
                    break;
            }
            
            ImGui::Separator();
            
            ImGui::Text("Quick Load:");
            
            if (ImGui::Button("Load Cube")) {
                mesh.loadPrimitive("BOX");
                mesh.type = PrimitiveType::CUBE;
                std::cout << "Loaded cube primitive" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Load Sphere")) {
                mesh.loadPrimitive("SPHERE", glm::vec3(1,0,0), glm::vec3(0,1,0), 1.0f);
                mesh.type = PrimitiveType::SPHERE;
                mesh.rayon = 1.0f;
                mesh.subdivisions = 20.0f;
                std::cout << "Loaded sphere primitive" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Load Plane")) {
                mesh.loadPrimitive("PLANE", glm::vec3(1,0,0), glm::vec3(0,1,0), 1.0f);
                mesh.type = PrimitiveType::PLANE;
                mesh.m_right_vector = glm::vec3(1,0,0);
                mesh.m_up_vector = glm::vec3(0,1,0);
                mesh.width = 1.0f;
                mesh.height = 1.0f;
                mesh.subdivisions = 10.0f;
                std::cout << "Loaded plane primitive" << std::endl;
            }
            
            if (ImGui::Button("Load Cylinder")) {
                mesh.loadPrimitive("CYLINDER");
                mesh.type = PrimitiveType::CYLINDER;
                mesh.width = 1.0f;
                mesh.height = 2.0f;
                mesh.subdivisions = 20.0f;
                std::cout << "Loaded cylinder primitive" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Load Cone")) {
                mesh.loadPrimitive("CONE");
                mesh.type = PrimitiveType::MESH;
                mesh.width = 1.0f;
                mesh.height = 2.0f;
                mesh.subdivisions = 20.0f;
                std::cout << "Loaded cone primitive" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Load Capsule")) {
                mesh.loadPrimitive("CAPSULE");
                mesh.type = PrimitiveType::MESH;
                mesh.width = 0.5f;
                mesh.height = 2.0f;
                mesh.subdivisions = 20.0f;
                std::cout << "Loaded capsule primitive" << std::endl;
            }
            
            ImGui::Separator();
            
            if (needsRebuild) {
                std::cout << "Rebuilding mesh with new parameters..." << std::endl;
                
                switch(mesh.type) {
                    case PrimitiveType::PLANE:
                        mesh.loadPrimitive("PLANE", mesh.m_right_vector, mesh.m_up_vector, 1.0f);
                        break;
                    case PrimitiveType::SPHERE:
                        mesh.loadPrimitive("SPHERE", glm::vec3(1,0,0), glm::vec3(0,1,0), mesh.rayon);
                        break;
                    case PrimitiveType::CYLINDER:
                        mesh.loadPrimitive("CYLINDER");
                        break;
                    default:
                        break;
                }
            }
            
            ImGui::Separator();
            
            ImGui::Text("Mesh Statistics:");
            ImGui::Text("Vertices: %zu", mesh.vertices.size());
            ImGui::Text("Indices: %zu", mesh.indices.size());
            ImGui::Text("Triangles: %zu", mesh.indices.size() / 3);
            ImGui::Text("VAO: %u", mesh.VAO);
            
            if (mesh.type != PrimitiveType::MESH) {
                ImGui::Text("Bounding Sphere Center: (%.2f, %.2f, %.2f)", 
                    mesh.boundingSphereFrustrumCulling.center.x,
                    mesh.boundingSphereFrustrumCulling.center.y,
                    mesh.boundingSphereFrustrumCulling.center.z);
                ImGui::Text("Bounding Sphere Radius: %.2f", mesh.boundingSphereFrustrumCulling.radius);
            }
        }
    }

    void renderLayerComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Layer", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& layer = const_cast<LayerComponent&>(
                entityManager->GetComponent<LayerComponent>(entityId)
            );
            char layerNameBuffer[256];
            strncpy(layerNameBuffer, layer.name.c_str(), sizeof(layerNameBuffer));
            layerNameBuffer[sizeof(layerNameBuffer) - 1] = '\0';
            
            if (ImGui::InputText("Layer Name", layerNameBuffer, sizeof(layerNameBuffer))) {
                layer.name = std::string(layerNameBuffer);
            }
            ImGui::InputInt("Layer ID", &layer.id);
        }
    }
    void renderTagComponent(uint32_t entityId){
        if (ImGui::CollapsingHeader("Tag", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& tag = const_cast<TagComponent&>(
                entityManager->GetComponent<TagComponent>(entityId)
            );
            char tagBuffer[256];
            strncpy(tagBuffer, tag.tag.c_str(), sizeof(tagBuffer));
            tagBuffer[sizeof(tagBuffer) - 1] = '\0';
            
            if (ImGui::InputText("Tag", tagBuffer, sizeof(tagBuffer))) {
                tag.tag = std::string(tagBuffer);
            }
        }
    }

    void saveScene(){
        json sceneData;
        sceneData["entities"] = json::array();
        
        for (const auto& entity : *entities) {
            json entityData;
            entityData["id"] = entity.id;
            
            // ════════════════════════════════════════════════════════════════
            //  TRANSFORM COMPONENT
            // ════════════════════════════════════════════════════════════════
            if (entityManager->HasComponent<TransformComponent>(entity.id)) {
                auto& transform = entityManager->GetComponent<TransformComponent>(entity.id);
                entityData["transform"] = {
                    {"position", {transform.position.x, transform.position.y, transform.position.z}},
                    {"rotation", {transform.rotation.x, transform.rotation.y, transform.rotation.z}},
                    {"scale", {transform.scale.x, transform.scale.y, transform.scale.z}},
                    {"parent", transform.parent},
                    {"children", transform.children}
                };
            }
            // ════════════════════════════════════════════════════════════════
            //  MESH COMPONENT
            // ════════════════════════════════════════════════════════════════
            if (entityManager->HasComponent<MeshComponent>(entity.id)) {
                auto& mesh = entityManager->GetComponent<MeshComponent>(entity.id);
                
                switch(mesh.type) {
                    case PrimitiveType::PLANE:
                        entityData["mesh"] = {
                            {"type", "primitive"},
                            {"mesh_type", "PLANE"},
                            {"subdivisions", mesh.subdivisions},
                            {"normal", {mesh.normal.x, mesh.normal.y, mesh.normal.z}},
                            {"width", mesh.width},
                            {"height", mesh.height}
                        };
                        break;
                        
                    case PrimitiveType::SPHERE:
                        entityData["mesh"] = {
                            {"type", "primitive"},
                            {"mesh_type", "SPHERE"},
                            {"subdivisions", mesh.subdivisions}
                        };
                        break;
                        
                    case PrimitiveType::CUBE:
                        entityData["mesh"] = {
                            {"type", "primitive"},
                            {"mesh_type", "BOX"}
                        };
                        break;
                        case PrimitiveType::CYLINDER:
                        entityData["mesh"] = {
                            {"type", "primitive"},
                            {"mesh_type", "CYLINDER"},
                            {"subdivisions", mesh.subdivisions},
                            {"width", mesh.width},
                            {"height", mesh.height}
                        };
                        break;
                        
                    case PrimitiveType::MESH:
                        // Sauvegarde le chemin du fichier OFF/OBJ
                        entityData["mesh"] = {
                            {"type", "file"},
                            {"path", getRelativePath(mesh.meshFilePath,GameFolder)}
                        };
                        break;
                }
            }
            // ════════════════════════════════════════════════════════════════
            //  MATERIAL COMPONENT
            // ════════════════════════════════════════════════════════════════
            if (entityManager->HasComponent<MaterialComponent>(entity.id)) {
                auto& material = entityManager->GetComponent<MaterialComponent>(entity.id);
                
                if (material.type == MaterialComponent::Type::Texture) {
                    entityData["material"] = {
                        {"type", "texture"},
                        {"path", getRelativePath(material.texturePath,GameFolder)}
                    };
                } else if (material.type == MaterialComponent::Type::Color) {
                    entityData["material"] = {
                        {"type", "color"},
                        {"color", {material.color.x, material.color.y, material.color.z}},
                        {"ambient", {material.ambient_material.x, material.ambient_material.y, material.ambient_material.z}},
                        {"diffuse", {material.diffuse_material.x, material.diffuse_material.y, material.diffuse_material.z}},
                        {"specular", {material.specular_material.x, material.specular_material.y, material.specular_material.z}},
                        {"shininess", material.shininess}
                    };
                }
            }
             // ════════════════════════════════════════════════════════════════
            //  CAMERA COMPONENT
            // ════════════════════════════════════════════════════════════════
            if (entityManager->HasComponent<CameraComponent>(entity.id)) {
                auto& camera = entityManager->GetComponent<CameraComponent>(entity.id);
                entityData["camera"] = {
                    {"idCam", camera.id},
                    {"target", {camera.target.x, camera.target.y, camera.target.z}},
                    {"up", {camera.up.x, camera.up.y, camera.up.z}},
                    {"fov", camera.fov},
                    {"near_plane", camera.nearPlane},
                    {"far_plane", camera.farPlane}
                };
            }
            // ════════════════════════════════════════════════════════════════
            //  LIGHT COMPONENT
            // ════════════════════════════════════════════════════════════════
            if (entityManager->HasComponent<LightComponent>(entity.id)) {
                auto& light = entityManager->GetComponent<LightComponent>(entity.id);
                entityData["light"] = {
                    {"intensity", light.intensity}
                };
            }
            
            // ════════════════════════════════════════════════════════════════
            //  AUDIO COMPONENT
            // ════════════════════════════════════════════════════════════════
            if (entityManager->HasComponent<MyAudioComponent>(entity.id)) {
                auto& audio = entityManager->GetComponent<MyAudioComponent>(entity.id);
                
                std::string typeStr = "NONE";
                switch(audio.type) {
                    case AudioType::MUSIC: typeStr = "MUSIC"; break;
                    case AudioType::SFX: typeStr = "SFX"; break;
                    case AudioType::SPATIAL: typeStr = "SPATIAL"; break;
                    default: typeStr = "NONE"; break;
                }
                
                entityData["audio"] = {
                    {"type", typeStr},
                    {"path", getRelativePath(audio.audioFilePath,GameFolder)},
                    {"volume", audio.volume},
                    {"loop", audio.loop},
                    {"play_on_start", audio.playOnStart},
                    {"isPlaying", audio.isPlaying}
                };
            }
            // ════════════════════════════════════════════════════════════════
            //  SCRIPT COMPONENT
            // ════════════════════════════════════════════════════════════════
            if (entityManager->HasComponent<LuaScriptComponent>(entity.id)) {
                auto& script = entityManager->GetComponent<LuaScriptComponent>(entity.id);
                entityData["script"] = {
                    {"type", "Lua"},
                    {"path", getRelativePath(script.luaScriptPath,GameFolder)}
                };
            }
            
            // ════════════════════════════════════════════════════════════════
            //  CONTROLLER COMPONENT
            // ════════════════════════════════════════════════════════════════
            if (entityManager->HasComponent<ControllerComponent>(entity.id)) {
                auto& controller = entityManager->GetComponent<ControllerComponent>(entity.id);
                entityData["controller"] = {
                    {"speed", controller.moveSpeed}
                };
            }
            
            sceneData["entities"].push_back(entityData);
        }
        // ════════════════════════════════════════════════════════════════
        //  ÉCRITURE DU FICHIER JSON
        // ════════════════════════════════════════════════════════════════
        std::ofstream sceneFile(scenePath);
        
        if (!sceneFile.is_open()) {
            std::cerr << "❌ Failed to open scene file for writing: " << scenePath << std::endl;
            return;
        }
        
        sceneFile << sceneData.dump(2); // Indentation de 2 espaces
        sceneFile.close();
        
        std::cout << "✅ Scene saved to " << scenePath << std::endl;
    }


    std::string getRelativePath(std::string fullPath, std::string basePath) {
        if(fullPath.empty()) return "";
        if(fullPath.find(basePath) != 0) {
            return fullPath;
        }
        std::string relativePath = fullPath.substr(basePath.length());
        if((relativePath[0] == '/' || relativePath[0] == '\\') && !relativePath.empty()) {
            relativePath = relativePath.substr(1);
        }
        return relativePath;
    }
    int currentTheme = 0; 

    // ════════════════════════════════════════════════════════════════
    //  THÈMES
    // ════════════════════════════════════════════════════════════════
    
    void applyDarkTheme(ImGuiStyle& style, ImVec4* colors){
        // Couleurs
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
        colors[ImGuiCol_Border]                 = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
        colors[ImGuiCol_Button]                 = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
        colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

        // Style
        style.WindowPadding                     = ImVec2(8.00f, 8.00f);
        style.FramePadding                      = ImVec2(5.00f, 2.00f);
        style.CellPadding                       = ImVec2(6.00f, 6.00f);
        style.ItemSpacing                       = ImVec2(6.00f, 6.00f);
        style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
        style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
        style.IndentSpacing                     = 25;
        style.ScrollbarSize                     = 15;
        style.GrabMinSize                       = 10;
        style.WindowBorderSize                  = 1;
        style.ChildBorderSize                   = 1;
        style.PopupBorderSize                   = 1;
        style.FrameBorderSize                   = 1;
        style.TabBorderSize                     = 1;
        style.WindowRounding                    = 7;
        style.ChildRounding                     = 4;
        style.FrameRounding                     = 3;
        style.PopupRounding                     = 4;
        style.ScrollbarRounding                 = 9;
        style.GrabRounding                      = 3;
        style.LogSliderDeadzone                 = 4;
        style.TabRounding                       = 4;
    }
    
    void applyLightTheme(ImGuiStyle& style, ImVec4* colors){
        ImGui::StyleColorsLight(); // Utilise le thème clair par défaut d'ImGui
        style.FrameRounding = 3.0f;
        style.WindowRounding = 3.0f;
    }

    void applyUnityTheme(ImGuiStyle& style, ImVec4* colors){
        // Unity-like: Gris foncé avec accents bleus
        colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_WindowBg]               = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.20f, 0.20f, 0.20f, 0.95f); // ✅ Ajouté
        colors[ImGuiCol_Border]                 = ImVec4(0.30f, 0.30f, 0.30f, 0.50f); // ✅ Ajouté
        colors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.45f, 0.45f, 0.45f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_CheckMark]              = ImVec4(0.25f, 0.50f, 0.75f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.25f, 0.50f, 0.75f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.30f, 0.60f, 0.85f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_Button]                 = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.25f, 0.50f, 0.75f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.25f, 0.50f, 0.75f, 0.55f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.25f, 0.50f, 0.75f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.25f, 0.50f, 0.75f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.30f, 0.30f, 0.30f, 0.50f); // ✅ Ajouté
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.40f, 0.40f, 0.40f, 0.70f); // ✅ Ajouté
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabHovered]             = ImVec4(0.25f, 0.50f, 0.75f, 0.80f); // ✅ Ajouté
        colors[ImGuiCol_TabActive]              = ImVec4(0.25f, 0.50f, 0.75f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.25f, 0.50f, 0.75f, 0.35f); // ✅ Ajouté
        
        style.WindowRounding = 0.0f;
        style.FrameRounding = 2.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding = 2.0f; // ✅ Ajouté
        style.TabRounding = 2.0f; // ✅ Ajouté
    }

    void applyDraculaTheme(ImGuiStyle& style, ImVec4* colors){
        // Dracula: Violet/Rose
        ImVec4 bg        = ImVec4(0.16f, 0.16f, 0.21f, 1.00f); // #282a36
        ImVec4 fg        = ImVec4(0.95f, 0.95f, 0.95f, 1.00f); // #f8f8f2
        ImVec4 purple    = ImVec4(0.74f, 0.58f, 0.98f, 1.00f); // #bd93f9
        ImVec4 pink      = ImVec4(1.00f, 0.47f, 0.78f, 1.00f); // #ff79c6
        ImVec4 cyan      = ImVec4(0.55f, 0.91f, 0.95f, 1.00f); // #8be9fd
        ImVec4 green     = ImVec4(0.31f, 0.98f, 0.48f, 1.00f); // #50fa7b
        ImVec4 orange    = ImVec4(1.00f, 0.71f, 0.42f, 1.00f); // #ffb86c
        
        colors[ImGuiCol_Text]                   = fg;
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.60f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_WindowBg]               = bg;
        colors[ImGuiCol_ChildBg]                = ImVec4(0.18f, 0.18f, 0.23f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_PopupBg]                = ImVec4(0.16f, 0.16f, 0.21f, 0.95f); // ✅ Ajouté
        colors[ImGuiCol_Border]                 = ImVec4(0.30f, 0.30f, 0.40f, 0.50f); // ✅ Ajouté
        colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.21f, 0.27f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.30f, 0.31f, 0.37f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.35f, 0.36f, 0.42f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBg]                = ImVec4(0.14f, 0.14f, 0.18f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.16f, 0.21f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.14f, 0.14f, 0.18f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.18f, 0.18f, 0.23f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.20f, 0.21f, 0.27f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.40f, 0.50f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.50f, 0.50f, 0.60f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.60f, 0.60f, 0.70f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_CheckMark]              = green; // ✅ Ajouté
        colors[ImGuiCol_SliderGrab]             = purple; // ✅ Ajouté
        colors[ImGuiCol_SliderGrabActive]       = pink; // ✅ Ajouté
        colors[ImGuiCol_Button]                 = purple;
        colors[ImGuiCol_ButtonHovered]          = pink;
        colors[ImGuiCol_ButtonActive]           = cyan;
        colors[ImGuiCol_Header]                 = purple;
        colors[ImGuiCol_HeaderHovered]          = pink;
        colors[ImGuiCol_HeaderActive]           = cyan;
        colors[ImGuiCol_Separator]              = ImVec4(0.30f, 0.30f, 0.40f, 0.50f); // ✅ Ajouté
        colors[ImGuiCol_SeparatorHovered]       = pink; // ✅ Ajouté
        colors[ImGuiCol_SeparatorActive]        = cyan; // ✅ Ajouté
        colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.21f, 0.27f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabHovered]             = pink; // ✅ Ajouté
        colors[ImGuiCol_TabActive]              = purple; // ✅ Ajouté
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.18f, 0.18f, 0.23f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.20f, 0.21f, 0.27f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(purple.x, purple.y, purple.z, 0.35f); // ✅ Ajouté
        
        style.FrameRounding = 5.0f;
        style.WindowRounding = 5.0f;
        style.ScrollbarRounding = 5.0f; // ✅ Ajouté
        style.GrabRounding = 5.0f; // ✅ Ajouté
        style.TabRounding = 5.0f; // ✅ Ajouté
    }

    void applyNordTheme(ImGuiStyle& style, ImVec4* colors){
        // Nord: Bleu glacial
        ImVec4 bg        = ImVec4(0.18f, 0.20f, 0.25f, 1.00f); // #2E3440
        ImVec4 fg        = ImVec4(0.92f, 0.93f, 0.95f, 1.00f); // #ECEFF4
        ImVec4 blue      = ImVec4(0.51f, 0.63f, 0.76f, 1.00f); // #81A1C1
        ImVec4 cyan      = ImVec4(0.55f, 0.75f, 0.82f, 1.00f); // #88C0D0
        ImVec4 frost     = ImVec4(0.56f, 0.74f, 0.73f, 1.00f); // #8FBCBB
        ImVec4 green     = ImVec4(0.64f, 0.75f, 0.54f, 1.00f); // #A3BE8C
        
        colors[ImGuiCol_Text]                   = fg;
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.55f, 0.60f, 0.70f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_WindowBg]               = bg;
        colors[ImGuiCol_ChildBg]                = ImVec4(0.20f, 0.22f, 0.27f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_PopupBg]                = ImVec4(0.18f, 0.20f, 0.25f, 0.95f); // ✅ Ajouté
        colors[ImGuiCol_Border]                 = ImVec4(0.30f, 0.35f, 0.40f, 0.50f); // ✅ Ajouté
        colors[ImGuiCol_FrameBg]                = ImVec4(0.22f, 0.24f, 0.29f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.27f, 0.29f, 0.34f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.32f, 0.34f, 0.39f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBg]                = ImVec4(0.16f, 0.18f, 0.23f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.18f, 0.20f, 0.25f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.16f, 0.18f, 0.23f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.20f, 0.22f, 0.27f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.22f, 0.24f, 0.29f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.35f, 0.40f, 0.50f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.45f, 0.50f, 0.60f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.55f, 0.60f, 0.70f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_CheckMark]              = green; // ✅ Ajouté
        colors[ImGuiCol_SliderGrab]             = blue; // ✅ Ajouté
        colors[ImGuiCol_SliderGrabActive]       = cyan; // ✅ Ajouté
        colors[ImGuiCol_Button]                 = blue;
        colors[ImGuiCol_ButtonHovered]          = cyan;
        colors[ImGuiCol_ButtonActive]           = frost; // ✅ Modifié
        colors[ImGuiCol_Header]                 = blue;
        colors[ImGuiCol_HeaderHovered]          = cyan;
        colors[ImGuiCol_HeaderActive]           = frost; // ✅ Modifié
        colors[ImGuiCol_Separator]              = ImVec4(0.30f, 0.35f, 0.40f, 0.50f); // ✅ Ajouté
        colors[ImGuiCol_SeparatorHovered]       = cyan; // ✅ Ajouté
        colors[ImGuiCol_SeparatorActive]        = frost; // ✅ Ajouté
        colors[ImGuiCol_Tab]                    = ImVec4(0.22f, 0.24f, 0.29f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabHovered]             = cyan; // ✅ Ajouté
        colors[ImGuiCol_TabActive]              = blue; // ✅ Ajouté
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.20f, 0.22f, 0.27f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.22f, 0.24f, 0.29f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(blue.x, blue.y, blue.z, 0.35f); // ✅ Ajouté
        
        style.FrameRounding = 4.0f;
        style.WindowRounding = 4.0f;
        style.ScrollbarRounding = 4.0f; // ✅ Ajouté
        style.GrabRounding = 4.0f; // ✅ Ajouté
        style.TabRounding = 4.0f; // ✅ Ajouté
    }

    void applyUnrealTheme(ImGuiStyle& style, ImVec4* colors){
        // Unreal-like: Noir profond avec accents orange
        ImVec4 bg        = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        ImVec4 fg        = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        ImVec4 orange    = ImVec4(1.00f, 0.50f, 0.00f, 1.00f);
        ImVec4 darkOrange= ImVec4(0.80f, 0.40f, 0.00f, 1.00f);
        
        colors[ImGuiCol_Text]                   = fg;
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_WindowBg]               = bg;
        colors[ImGuiCol_ChildBg]                = ImVec4(0.08f, 0.08f, 0.08f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_PopupBg]                = ImVec4(0.06f, 0.06f, 0.06f, 0.95f); // ✅ Ajouté
        colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.50f); // ✅ Ajouté
        colors[ImGuiCol_FrameBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.04f, 0.04f, 0.04f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.08f, 0.08f, 0.08f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_CheckMark]              = orange; // ✅ Ajouté
        colors[ImGuiCol_SliderGrab]             = darkOrange; // ✅ Ajouté
        colors[ImGuiCol_SliderGrabActive]       = orange; // ✅ Ajouté
        colors[ImGuiCol_Button]                 = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_Header]                 = ImVec4(1.00f, 0.50f, 0.00f, 0.55f); // Orange
        colors[ImGuiCol_HeaderHovered]          = ImVec4(1.00f, 0.50f, 0.00f, 0.80f);
        colors[ImGuiCol_HeaderActive]           = orange; // ✅ Modifié
        colors[ImGuiCol_Separator]              = ImVec4(0.20f, 0.20f, 0.20f, 0.50f); // ✅ Ajouté
        colors[ImGuiCol_SeparatorHovered]       = darkOrange; // ✅ Ajouté
        colors[ImGuiCol_SeparatorActive]        = orange; // ✅ Ajouté
        colors[ImGuiCol_Tab]                    = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabHovered]             = darkOrange; // ✅ Ajouté
        colors[ImGuiCol_TabActive]              = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.08f, 0.08f, 0.08f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // ✅ Ajouté
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(orange.x, orange.y, orange.z, 0.35f); // ✅ Ajouté
        
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.ScrollbarRounding = 0.0f; // ✅ Ajouté
        style.GrabRounding = 0.0f; // ✅ Ajouté
        style.TabRounding = 0.0f; // ✅ Ajouté
    }
};


#endif // EDITORSYSTEM_H