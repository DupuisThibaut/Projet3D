#ifndef LUA_SCRIPT_COMPONENT_H
#define LUA_SCRIPT_COMPONENT_H
#include "InputEvent.h"
#include <string>

struct LuaScriptComponent {
    std::string luaScriptPath;
    lua_State* L = nullptr;
    std::string GameFolder;
    bool initialized = false;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId, const std::string& gameFolder) {
        GameFolder = gameFolder;
        if(entityData["entities"][entityId].contains("script")){
            if(entityData["entities"][entityId]["script"].contains("path")) luaScriptPath = gameFolder + "/" + entityData["entities"][entityId]["script"]["path"].get<std::string>();
        }
    }

    void renderEditor() {
            char pathBuffer[512];
            strncpy(pathBuffer, luaScriptPath.c_str(), sizeof(pathBuffer));
            pathBuffer[sizeof(pathBuffer) - 1] = '\0';

            if (ImGui::InputText("Script Path", pathBuffer, sizeof(pathBuffer))) {
                luaScriptPath = std::string(pathBuffer);
            }
            ImGui::Text("Lua Script File:");
            ImGui::BeginChild("LuaScriptDropZone", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::TextWrapped("%s", luaScriptPath.empty() ? "Drag & drop a Lua script file here" : luaScriptPath.c_str());
            ImGui::EndChild();

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                    const char* droppedPath = (const char*)payload->Data;
                    std::string ext = droppedPath;
                    ext = ext.substr(ext.find_last_of('.') + 1);
                    // Vérifie si c'est un fichier Lua
                    if (ext == "lua") {
                        luaScriptPath = droppedPath;
                    }
                }
            }
    }

    json toJson() {
        nlohmann::json j;
        std::string relPath = luaScriptPath;
        size_t pos = luaScriptPath.find(GameFolder);
        std::cout << "GameFolder: " << GameFolder << std::endl;
        if (pos != std::string::npos) {
            std::cout << "Found GameFolder in path at position: " << pos << std::endl;
            relPath = luaScriptPath.substr(pos + GameFolder.length() + 1); ;
        } else {
            relPath = luaScriptPath;
        }
        std::cout << "Saving Lua script path: " << relPath << std::endl;
        j["path"] = relPath;
        return j;
    }
};

#endif // SCRIPT_COMPONENT_H
