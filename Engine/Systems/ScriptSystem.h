#pragma once
#include "../Components/LuaScriptComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/InputEvent.h"
#include <iostream>
#include <filesystem>
#include <cstring>
#include <unordered_map>

struct ScriptSystem {
    EntityManager* entityManager;
    std::unordered_map<uint32_t, LuaScriptComponent*> luaScripts;

    void registerLuaScript(uint32_t entityId, LuaScriptComponent* script) {
        luaScripts[entityId] = script;
    }

    void registerEntityManager(EntityManager* em) {
        entityManager = em;
    }

    // Helper: push a vec3 as a Lua table with [1],[2],[3] indices (compatible with your math functions)
    static void pushVec3AsTable(lua_State* L, const glm::vec3& v) {
        lua_newtable(L);
        lua_pushnumber(L, v.x); lua_rawseti(L, -2, 1);
        lua_pushnumber(L, v.y); lua_rawseti(L, -2, 2);
        lua_pushnumber(L, v.z); lua_rawseti(L, -2, 3);
    }

    // Helper: read a Lua table {[1]=x,[2]=y,[3]=z} into a vec3
    static glm::vec3 tableToVec3(lua_State* L, int idx) {
        glm::vec3 result(0.0f);
        lua_rawgeti(L, idx, 1); result.x = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, idx, 2); result.y = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_rawgeti(L, idx, 3); result.z = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        return result;
    }

    static void pushVec3AsColorTable(lua_State* L, const glm::vec3& v) {
        lua_newtable(L);
        lua_pushnumber(L, v.x); lua_setfield(L, -2, "r");
        lua_pushnumber(L, v.y); lua_setfield(L, -2, "g");
        lua_pushnumber(L, v.z); lua_setfield(L, -2, "b");
    }

    // Helper: read color table {r=.., g=.., b=..} into vec3
    static glm::vec3 colorTableToVec3(lua_State* L, int idx) {
        glm::vec3 result(0.0f);
        lua_getfield(L, idx, "r"); result.x = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_getfield(L, idx, "g"); result.y = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        lua_getfield(L, idx, "b"); result.z = (float)lua_tonumber(L, -1); lua_pop(L, 1);
        return result;
    }

    void initScript(LuaScriptComponent& script, uint32_t entityId) {
        script.L = luaL_newstate();
        luaL_openlibs(script.L);

        if (!std::filesystem::exists(script.luaScriptPath)) {
            std::cerr << "Script introuvable: " << script.luaScriptPath << std::endl;
            return;
        }

        if (luaL_dofile(script.L, script.luaScriptPath.c_str()) != LUA_OK) {
            std::cerr << "Erreur Lua : " << lua_tostring(script.L, -1) << std::endl;
            return;
        }

        // Créer un userData Lua pour le TransformComponent avec métatables pour position
        TransformComponent** udata = (TransformComponent**)lua_newuserdata(script.L, sizeof(TransformComponent*));
        *udata = &entityManager->GetComponent<TransformComponent>(entityId);

        // Métatable pour transform
        luaL_newmetatable(script.L, "TransformMetaTable");

        lua_pushcfunction(script.L, [](lua_State* L) -> int {
            TransformComponent* t = *(TransformComponent**)lua_touserdata(L, 1);
            const char* key = luaL_checkstring(L, 2);
            if (strcmp(key, "x") == 0) {
                lua_pushnumber(L, t->position.x);
            } else if (strcmp(key, "y") == 0) {
                lua_pushnumber(L, t->position.y);
            } else if (strcmp(key, "z") == 0) {
                lua_pushnumber(L, t->position.z);
            } else if (strcmp(key, "position") == 0) {
                // Return position as a table {[1]=x, [2]=y, [3]=z}
                ScriptSystem::pushVec3AsTable(L, t->position);
            } else {
                lua_pushnil(L);
            }
            return 1;
        });
        lua_setfield(script.L, -2, "__index");

        lua_pushcfunction(script.L, [](lua_State* L) -> int {
            TransformComponent* t = *(TransformComponent**)lua_touserdata(L, 1);
            const char* key = luaL_checkstring(L, 2);
            
            if (strcmp(key, "x") == 0) {
                t->position.x = (float)luaL_checknumber(L, 3);
            } else if (strcmp(key, "y") == 0) {
                t->position.y = (float)luaL_checknumber(L, 3);
            } else if (strcmp(key, "z") == 0) {
                t->position.z = (float)luaL_checknumber(L, 3);
            } else if (strcmp(key, "position") == 0) {
                // Expect a table {[1]=x,[2]=y,[3]=z}
                if (lua_istable(L, 3)) {
                    t->position = ScriptSystem::tableToVec3(L, 3);
                }
            }
            return 0;
        });
        lua_setfield(script.L, -2, "__newindex");

        lua_setmetatable(script.L, -2);
        lua_setglobal(script.L, "transform");

        if(entityManager->HasComponent<CameraComponent>(entityId)){

            CameraComponent** camdata = (CameraComponent**)lua_newuserdata(script.L, sizeof(CameraComponent*));
            *camdata = &entityManager->GetComponent<CameraComponent>(entityId);
            luaL_newmetatable(script.L, "CameraMetaTable");
            // __index
            lua_pushcfunction(script.L, [](lua_State* L) -> int {
                CameraComponent* cam = *(CameraComponent**)lua_touserdata(L, 1);
                const char* key = luaL_checkstring(L, 2);
                if (strcmp(key, "yaw") == 0) lua_pushnumber(L, cam->yaw);
                else if (strcmp(key, "pitch") == 0) lua_pushnumber(L, cam->pitch);
                else if (strcmp(key, "target") == 0) ScriptSystem::pushVec3AsTable(L, cam->target);
                else lua_pushnil(L);
                return 1;
            });
            lua_setfield(script.L, -2, "__index");
            // __newindex
            lua_pushcfunction(script.L, [](lua_State* L) -> int {
                CameraComponent* cam = *(CameraComponent**)lua_touserdata(L, 1);
                const char* key = luaL_checkstring(L, 2);
                if (strcmp(key, "yaw") == 0) cam->yaw = (float)luaL_checknumber(L, 3);
                else if (strcmp(key, "pitch") == 0) cam->pitch = (float)luaL_checknumber(L, 3);
                else if (strcmp(key, "target") == 0) cam->target = ScriptSystem::tableToVec3(L, 3);
                return 0;
            });
            lua_setfield(script.L, -2, "__newindex");
            lua_setmetatable(script.L, -2);
            lua_setglobal(script.L, "camera");
        }
        if(entityManager->HasComponent<MaterialComponent>(entityId)){
            MaterialComponent** matdata = (MaterialComponent**)lua_newuserdata(script.L, sizeof(MaterialComponent*));
            *matdata = &entityManager->GetComponent<MaterialComponent>(entityId);
            luaL_newmetatable(script.L, "MaterialMetaTable");
            
            lua_pushcfunction(script.L, [](lua_State* L) -> int {
                MaterialComponent* mat = *(MaterialComponent**)lua_touserdata(L, 1);
                const char* key = luaL_checkstring(L, 2);
                if (strcmp(key, "type") == 0) {
                    lua_pushstring(L, mat->type == MaterialComponent::Type::Texture ? "texture" :
                                        mat->type == MaterialComponent::Type::Color ? "color" : "none");
                } else if (strcmp(key, "color") == 0) {
                   MaterialComponent** colorProxy = (MaterialComponent**)lua_newuserdata(L, sizeof(MaterialComponent*));
                   *colorProxy = mat;
                   
                   luaL_newmetatable(L, "ColorMetaTable");
                   
                   // __index pour color.r, color.g, color.b
                   lua_pushcfunction(L, [](lua_State* L) -> int {
                       MaterialComponent* m = *(MaterialComponent**)lua_touserdata(L, 1);
                       const char* field = luaL_checkstring(L, 2);
                       if (strcmp(field, "r") == 0) lua_pushnumber(L, m->color.x);
                       else if (strcmp(field, "g") == 0) lua_pushnumber(L, m->color.y);
                       else if (strcmp(field, "b") == 0) lua_pushnumber(L, m->color.z);
                       else lua_pushnil(L);
                       return 1;
                   });
                   lua_setfield(L, -2, "__index");
                   
                   // __newindex pour material.color.r = 1.0
                   lua_pushcfunction(L, [](lua_State* L) -> int {
                       MaterialComponent* m = *(MaterialComponent**)lua_touserdata(L, 1);
                       const char* field = luaL_checkstring(L, 2);
                       float value = (float)luaL_checknumber(L, 3);
                       if (strcmp(field, "r") == 0) m->color.x = value;
                       else if (strcmp(field, "g") == 0) m->color.y = value;
                       else if (strcmp(field, "b") == 0) m->color.z = value;
                       return 0;
                   });
                   lua_setfield(L, -2, "__newindex");
                  
                   lua_setmetatable(L, -2);
                } else {
                    lua_pushnil(L);
                }
                return 1;
            });
            lua_setfield(script.L, -2, "__index");
            
            // __newindex pour material.color = {r=.., g=.., b=..}
            lua_pushcfunction(script.L, [](lua_State* L) -> int {
                MaterialComponent* mat = *(MaterialComponent**)lua_touserdata(L, 1);
                const char* key = luaL_checkstring(L, 2);
                if (strcmp(key, "type") == 0) {
                    const char* val = luaL_checkstring(L, 3);
                    if (strcmp(val, "texture") == 0) mat->type = MaterialComponent::Type::Texture;
                    else if (strcmp(val, "color") == 0) mat->type = MaterialComponent::Type::Color;
                    else mat->type = MaterialComponent::Type::None;
                } else if (strcmp(key, "color") == 0 && lua_istable(L, 3)) {
                   // Support material.color = {r=1, g=0, b=0}
                    mat->color = ScriptSystem::colorTableToVec3(L, 3);
                }
                return 0;
            });
            lua_setfield(script.L, -2, "__newindex");
            
            lua_setmetatable(script.L, -2);
            lua_setglobal(script.L, "material");
        }

        lua_getglobal(script.L, "onInit");
        if (lua_isfunction(script.L, -1)) {
            if (lua_pcall(script.L, 0, 0, 0) != LUA_OK)
                std::cerr << "Erreur dans onInit: " << lua_tostring(script.L, -1) << std::endl;
        } else {
            lua_pop(script.L, 1);
        }
        script.initialized = true;
    }

    void updateScript(LuaScriptComponent& script, float dt) {
        if (!script.initialized || !script.L)
            return;

        lua_getglobal(script.L, "onUpdate");
        if (lua_isfunction(script.L, -1)) {
            lua_pushnumber(script.L, dt);
            if (lua_pcall(script.L, 1, 0, 0) != LUA_OK)
                std::cerr << "Erreur dans onUpdate: " << lua_tostring(script.L, -1) << std::endl;
        } else {
            lua_pop(script.L, 1);
        }
    }

    void onUpdate(float deltaTime) {
        for (auto& [id, script] : luaScripts) {
            if (script) updateScript(*script, deltaTime);
        }
    }

    void shutdownScript(LuaScriptComponent& script) {
        if (script.L)
            lua_close(script.L);
        script.L = nullptr;
    }

    void onInput(const InputEvent& event) {
        for (auto& [id, script] : luaScripts) {
            if (!script || !script->L) continue;
            lua_State* L = script->L;
            lua_getglobal(L, "onInput");
            if (!lua_isfunction(L, -1)) {
                lua_pop(L, 1);
                continue;
            }

            lua_newtable(L);
            lua_pushnumber(L, event.dt);
            lua_setfield(L, -2, "dt");
            lua_pushnumber(L, event.scroll);
            lua_setfield(L, -2, "scroll");
            lua_pushnumber(L, event.mouseDeltaX);
            lua_setfield(L, -2, "mouseDeltaX");
            lua_pushnumber(L, event.mouseDeltaY);
            lua_setfield(L, -2, "mouseDeltaY");
            lua_pushboolean(L, event.mouseMoved);
            lua_setfield(L, -2, "mouseMoved");

            lua_newtable(L);
            int i = 1;
            for (const auto& btn : event.buttons) {
                lua_pushstring(L, btn.c_str());
                lua_rawseti(L, -2, i++);
            }
            lua_setfield(L, -2, "buttons");

            if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                std::cerr << "Erreur Lua onInput: " << lua_tostring(L, -1) << std::endl;
                lua_pop(L, 1);
            }
        }
    }

    void bind(LuaScriptComponent& script, TransformComponent* t, CameraComponent* c, MaterialComponent* m) {
    lua_State* L = script.L;

    // --- Transform ---
    if (t) {
        TransformComponent** udata = (TransformComponent**)lua_newuserdata(L, sizeof(TransformComponent*));
        *udata = t;
        luaL_getmetatable(L, "TransformMetaTable");
        lua_setmetatable(L, -2);
        lua_setglobal(L, "transform");
    }

    // --- Camera ---
    if (c) {
        CameraComponent** udata = (CameraComponent**)lua_newuserdata(L, sizeof(CameraComponent*));
        *udata = c;
        luaL_getmetatable(L, "CameraMetaTable");  // <- récupère la métatable existante
        lua_setmetatable(L, -2);
        lua_setglobal(L, "camera");
    }

    // --- Material ---
    if (m) {
        MaterialComponent** udata = (MaterialComponent**)lua_newuserdata(L, sizeof(MaterialComponent*));
        *udata = m;
        luaL_getmetatable(L, "MaterialMetaTable");
        lua_setmetatable(L, -2);
        lua_setglobal(L, "material");
    }
}



};