#ifndef LUA_SCRIPT_COMPONENT_H
#define LUA_SCRIPT_COMPONENT_H
#include "InputEvent.h"
#include <string>

struct LuaScriptComponent {
    std::string luaScriptPath;
    lua_State* L = nullptr;

    bool initialized = false;
};

#endif // SCRIPT_COMPONENT_H
