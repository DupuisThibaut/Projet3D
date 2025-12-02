#ifndef LUA_SCRIPT_COMPONENT_H
#define LUA_SCRIPT_COMPONENT_H
#include "InputEvent.h"
#include <string>

struct LuaScriptComponent {
    std::string luaScriptPath;
    lua_State* L = nullptr;

    bool initialized = false;

    void loadFromFile(const nlohmann::json& entityData, uint32_t entityId, const std::string& gameFolder) {
        if(entityData["entities"][entityId].contains("script")){
            if(entityData["entities"][entityId]["script"].contains("path")) luaScriptPath = gameFolder + entityData["entities"][entityId]["script"]["path"].get<std::string>();
        }
    }
};

#endif // SCRIPT_COMPONENT_H
