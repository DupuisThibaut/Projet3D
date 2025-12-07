#pragma once

#include "../Components/LuaScriptComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/MaterialComponent.h"
#include "../Components/MeshComponent.h"
#include "../Components/LightComponent.h"
#include "../Components/ControllerComponent.h"
#include "../Components/MyAudioComponent.h"
#include "../Components/TagComponent.h"
#include "../Components/LayerComponent.h"
#include "../Components/InputEvent.h"
#include "../Systems/EntityManager.h"
#include "../Systems/Dispatcher.h"
#include "../Systems/SceneManager.h"

#include <iostream>
#include <filesystem>
#include <cstring>
#include <unordered_map>
#include <algorithm>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class ScriptSystem {
public:
    EntityManager* entityManager = nullptr;
    SceneManager* sceneManager = nullptr;
    std::unordered_map<uint32_t, LuaScriptComponent*> luaScripts;
    Dispatcher* dispatcher = nullptr;
    std::vector<Entity>* entities = nullptr;

    struct GlobalValue {
        enum ValueType {NONE = 0, NUMBER = 1, STRING = 2, BOOLEAN = 3, VEC3 = 4 } type = NONE;
        double numberValue = 0.0;
        std::string stringValue ;
        bool boolValue = false;
        glm::vec3 vec3Value{0.0f};
        GlobalValue() = default;
    };
    std::unordered_map<std::string, GlobalValue> globalValues;

    static void pushGlobalValueToLua(lua_State* L, const GlobalValue& gv) {
        switch (gv.type) {
            case GlobalValue::NUMBER:
                lua_pushnumber(L, gv.numberValue);
                break;
            case GlobalValue::STRING:
                lua_pushstring(L, gv.stringValue.c_str());
                break;
            case GlobalValue::BOOLEAN:
                lua_pushboolean(L, gv.boolValue);
                break;
            case GlobalValue::VEC3:
                ScriptSystem::pushVec3AsTable(L, gv.vec3Value);
                break;
            default:
                lua_pushnil(L);
                break;
        }
    }

    static GlobalValue getGlobalValueFromLua(lua_State* L, int idx) {
        GlobalValue gv;
        if (lua_isnumber(L, idx)) {
            gv.type = GlobalValue::NUMBER;
            gv.numberValue = lua_tonumber(L, idx);
        } else if (lua_isstring(L, idx)) {
            gv.type = GlobalValue::STRING;
            gv.stringValue = lua_tostring(L, idx);
        } else if (lua_isboolean(L, idx)) {
            gv.type = GlobalValue::BOOLEAN;
            gv.boolValue = lua_toboolean(L, idx);
        } else if (lua_istable(L, idx)) {
            // 1) try numeric indices [1],[2],[3]
            lua_rawgeti(L, idx, 1);
            bool okNum = lua_isnumber(L, -1);
            float x = okNum ? (float)lua_tonumber(L, -1) : 0.0f;
            lua_pop(L, 1);
            lua_rawgeti(L, idx, 2); okNum = okNum && lua_isnumber(L, -1);
            float y = okNum ? (float)lua_tonumber(L, -1) : 0.0f;
            lua_pop(L, 1);
            lua_rawgeti(L, idx, 3); okNum = okNum && lua_isnumber(L, -1);
            float z = okNum ? (float)lua_tonumber(L, -1) : 0.0f;
            lua_pop(L, 1);

            if (okNum) {
                gv.type = GlobalValue::VEC3;
                gv.vec3Value = glm::vec3(x, y, z);
            } else {
                // 2) try named fields r/g/b or x/y/z
                bool okNamed = true;
                lua_getfield(L, idx, "r"); if (!lua_isnumber(L, -1)) okNamed = false; float rn = okNamed ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L,1);
                lua_getfield(L, idx, "g"); if (!lua_isnumber(L, -1)) okNamed = false; float gn = okNamed ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L,1);
                lua_getfield(L, idx, "b"); if (!lua_isnumber(L, -1)) okNamed = false; float bn = okNamed ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L,1);
                if (!okNamed) {
                    // try x/y/z as fallback
                    okNamed = true;
                    lua_getfield(L, idx, "x"); if (!lua_isnumber(L, -1)) okNamed = false; float xn = okNamed ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L,1);
                    lua_getfield(L, idx, "y"); if (!lua_isnumber(L, -1)) okNamed = false; float yn = okNamed ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L,1);
                    lua_getfield(L, idx, "z"); if (!lua_isnumber(L, -1)) okNamed = false; float zn = okNamed ? (float)lua_tonumber(L, -1) : 0.0f; lua_pop(L,1);
                    if (okNamed) { gv.type = GlobalValue::VEC3; gv.vec3Value = glm::vec3(xn, yn, zn); }
                } else {
                    gv.type = GlobalValue::VEC3;
                    gv.vec3Value = glm::vec3(rn, gn, bn);
                }
            }
        } else {
            gv.type = GlobalValue::NONE;
        }
         return gv;
    }

    void registerEntities(std::vector<Entity>* ents) {
        entities = ents;
    }

    void registerDispatcher(Dispatcher* disp) {
        dispatcher = disp;
        dispatcher->subscribe([this](const InputEvent& event) {
            this->onInput(event);
            return false; // don't stop propagation
        });
    }

    void registerLuaScript(uint32_t entityId, LuaScriptComponent* script) {
        luaScripts[entityId] = script;
    }

    void registerEntityManager(EntityManager* em) {
        entityManager = em;
    }

    void registerSceneManager(SceneManager* sm) {
        sceneManager = sm;
    }

    // Shutdown all scripts known to the ScriptSystem and clear registry
    void onSceneReset() {
        for (auto &kv : luaScripts) {
            LuaScriptComponent* s = kv.second;
            if (s) {
                if (s->L) {
                    lua_close(s->L);
                    s->L = nullptr;
                }
                s->initialized = false;
            }
        }
        luaScripts.clear();
    }

    // Helper: push a vec3 as a Lua table with [1],[2],[3] indices
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

        // try numeric indices first: {x,y,z} or {[1]=x,...}
        lua_rawgeti(L, idx, 1);
        bool ok1 = lua_isnumber(L, -1);
        if (ok1) {
            result.x = (float)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);

        lua_rawgeti(L, idx, 2);
        if (lua_isnumber(L, -1)) result.y = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        lua_rawgeti(L, idx, 3);
        if (lua_isnumber(L, -1)) result.z = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        if (ok1) return result;

        // fallback to named fields r/g/b
        lua_getfield(L, idx, "r");
        if (lua_isnumber(L, -1)) result.x = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, idx, "g");
        if (lua_isnumber(L, -1)) result.y = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, idx, "b");
        if (lua_isnumber(L, -1)) result.z = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        // final fallback: try x/y/z (common for position tables)
        lua_getfield(L, idx, "x");
        if (lua_isnumber(L, -1)) result.x = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, idx, "y");
        if (lua_isnumber(L, -1)) result.y = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);
        lua_getfield(L, idx, "z");
        if (lua_isnumber(L, -1)) result.z = (float)lua_tonumber(L, -1);
        lua_pop(L, 1);

        return result;
    }

private:
    // Registry key used to store ScriptSystem* in each lua_State
    static constexpr const char* REGISTRY_SCRIPTSYS_KEY = "ScriptSystemPtr";

    // Helper: push a component pointer as userdata and set its metatable (metatable must be created earlier)
    template<typename T>
    static void pushComponentUserdata(lua_State* L, T* ptr, const char* metaName) {
        if (!ptr) { lua_pushnil(L); return; }
        T** udata = (T**)lua_newuserdata(L, sizeof(T*));
        *udata = ptr;
        luaL_getmetatable(L, metaName);
        lua_setmetatable(L, -2);
    }

    // Create common metatables for components (idempotent)
    static void ensureComponentMetatables(lua_State* L) {
        // TransformMetaTable
        if (luaL_newmetatable(L, "TransformMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                TransformComponent* t = *(TransformComponent**)lua_touserdata(s, 1);
                const char* key = luaL_checkstring(s, 2);
                if (strcmp(key, "position")==0) { ScriptSystem::pushVec3AsTable(s, t->position); return 1; }
                if (strcmp(key, "rotation")==0) { ScriptSystem::pushVec3AsTable(s, t->rotation); return 1; }
                if (strcmp(key, "scale")==0)    { ScriptSystem::pushVec3AsTable(s, t->scale);    return 1; }
                lua_pushnil(s); return 1;
            });
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                TransformComponent* t = *(TransformComponent**)lua_touserdata(s, 1);
                const char* key = luaL_checkstring(s, 2);
                if (strcmp(key, "position")==0 && lua_istable(s,3)) { t->position = ScriptSystem::tableToVec3(s, 3); return 0; }
                if (strcmp(key, "rotation")==0 && lua_istable(s,3)) { t->rotation = ScriptSystem::tableToVec3(s, 3); return 0; }
                if (strcmp(key, "scale")==0    && lua_istable(s,3)) { t->scale    = ScriptSystem::tableToVec3(s, 3); return 0; }
                return 0;
            });
            lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L,1); }

        // CameraMetaTable
        if (luaL_newmetatable(L, "CameraMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                CameraComponent* c = *(CameraComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"target")==0) { ScriptSystem::pushVec3AsTable(s, c->target); return 1; }
                if (strcmp(k,"yaw")==0) { lua_pushnumber(s, c->yaw); return 1; }
                if (strcmp(k,"pitch")==0) { lua_pushnumber(s, c->pitch); return 1; }
                if (strcmp(k,"update")==0) { lua_pushboolean(s, c->update); return 1; }
                if (strcmp(k,"isActive")==0) { lua_pushboolean(s, c->isActive); return 1; }
                if (strcmp(k,"fov")==0) { lua_pushnumber(s, c->fov); return 1; }
                if (strcmp(k,"nearPlane")==0) { lua_pushnumber(s, c->nearPlane); return 1; }
                if (strcmp(k,"farPlane")==0) { lua_pushnumber(s, c->farPlane); return 1; }
                if (strcmp(k,"up")==0) { ScriptSystem::pushVec3AsTable(s, c->up); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L,-2,"__index");

            lua_pushcfunction(L, [](lua_State* s)->int {
                CameraComponent* c = *(CameraComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"target")==0 && lua_istable(s,3)) c->target = ScriptSystem::tableToVec3(s,3);
                else if (strcmp(k,"yaw")==0) c->yaw = (float)luaL_checknumber(s,3);
                else if (strcmp(k,"pitch")==0) c->pitch = (float)luaL_checknumber(s,3);
                else if (strcmp(k,"update")==0) c->update = lua_toboolean(s,3);
                else if (strcmp(k,"isActive")==0) c->isActive = lua_toboolean(s,3);
                else if (strcmp(k,"fov")==0) c->fov = (float)luaL_checknumber(s,3);
                else if (strcmp(k,"nearPlane")==0) c->nearPlane = (float)luaL_checknumber(s,3);
                else if (strcmp(k,"farPlane")==0) c->farPlane = (float)luaL_checknumber(s,3);
                else if (strcmp(k,"up")==0 && lua_istable(s,3)) c->up = ScriptSystem::tableToVec3(s,3);
                return 0;
            }); lua_setfield(L,-2,"__newindex");
        } else { lua_pop(L,1); }

        // LightMetaTable
        if (luaL_newmetatable(L, "LightMetaTable")) {
            // __index
            lua_pushcfunction(L, [](lua_State* s)->int {
                LightComponent* l = *(LightComponent**)luaL_checkudata(s, 1, "LightMetaTable");
                const char* key = luaL_checkstring(s, 2);
                if (strcmp(key, "intensity") == 0) { lua_pushnumber(s, l->intensity); return 1; }
                if (strcmp(key, "update") == 0) { lua_pushboolean(s, l->update); return 1; }
                if (strcmp(key, "nb") == 0) { lua_pushinteger(s, l->nb); return 1; }
                lua_pushnil(s); return 1;
            });
            lua_setfield(L, -2, "__index");
            // __newindex
            lua_pushcfunction(L, [](lua_State* s)->int {
                LightComponent* l = *(LightComponent**)luaL_checkudata(s, 1, "LightMetaTable");
                const char* key = luaL_checkstring(s, 2);
                if (strcmp(key, "intensity") == 0) { l->intensity = (float)lua_tonumber(s, 3); return 0; }
                if (strcmp(key, "update") == 0) { l->update = lua_toboolean(s, 3); return 0; }
                if (strcmp(key, "nb") == 0) { l->nb = (int)lua_tointeger(s, 3); return 0; }
                return 0;
            });
            lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L,1); }

        // MaterialMetaTable
        if (luaL_newmetatable(L, "MaterialMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                MaterialComponent* m = *(MaterialComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"color")==0) { ScriptSystem::pushVec3AsColorTable(s, m->color); return 1; }
                if (strcmp(k,"type")==0) { lua_pushstring(s, m->type==MaterialComponent::Type::Color?"color":(m->type==MaterialComponent::Type::Texture?"texture":"none")); return 1; }
                if (strcmp(k,"texturePath")==0) { lua_pushstring(s, m->texturePath.c_str()); return 1; }
                if (strcmp(k,"diffuse_material")==0) { ScriptSystem::pushVec3AsColorTable(s, m->diffuse_material); return 1; }
                if (strcmp(k,"specular_material")==0) { ScriptSystem::pushVec3AsColorTable(s, m->specular_material); return 1; }
                if (strcmp(k,"ambient_material")==0) { ScriptSystem::pushVec3AsColorTable(s, m->ambient_material); return 1; }
                if (strcmp(k,"shininess")==0) { lua_pushnumber(s, m->shininess); return 1; }
                if (strcmp(k,"particularite")==0) { lua_pushinteger(s, m->particularite); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L,-2,"__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                MaterialComponent* m = *(MaterialComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"color")==0 && lua_istable(s,3)) m->color = ScriptSystem::colorTableToVec3(s,3);
                else if (strcmp(k,"type")==0) { const char* v = luaL_checkstring(s,3); if (strcmp(v,"color")==0) m->type = MaterialComponent::Type::Color; else if (strcmp(v,"texture")==0) m->type = MaterialComponent::Type::Texture; else m->type = MaterialComponent::Type::None; }
                else if (strcmp(k,"texturePath")==0) m->texturePath = luaL_checkstring(s,3);
                else if (strcmp(k,"diffuse_material")==0 && lua_istable(s,3)) m->diffuse_material = ScriptSystem::colorTableToVec3(s,3);
                else if (strcmp(k,"specular_material")==0 && lua_istable(s,3)) m->specular_material = ScriptSystem::colorTableToVec3(s,3);
                else if (strcmp(k,"ambient_material")==0 && lua_istable(s,3)) m->ambient_material = ScriptSystem::colorTableToVec3(s,3);
                else if (strcmp(k,"shininess")==0) m->shininess = (float)luaL_checknumber(s,3);
                else if (strcmp(k,"particularite")==0) m->particularite = (int)luaL_checkinteger(s,3);
                return 0;
            }); lua_setfield(L,-2,"__newindex");
        } else { lua_pop(L,1); }

        // MeshMetaTable
        if (luaL_newmetatable(L, "MeshMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                MeshComponent* m = *(MeshComponent**)luaL_checkudata(s, 1, "MeshMetaTable");
                if (!m) { lua_pushnil(s); return 1; }
                if (!lua_isstring(s, 2)) { lua_pushnil(s); return 1; }
                const char* k = lua_tostring(s, 2);
                if (strcmp(k, "meshPath") == 0) { lua_pushstring(s, m->meshFilePath.c_str()); return 1; }
                if (strcmp(k, "type") == 0) {
                    const char* ts = "MESH";
                    switch (m->type) {
                        case PrimitiveType::PLANE:   ts = "PLANE";   break;
                        case PrimitiveType::CUBE:    ts = "BOX";     break;
                        case PrimitiveType::SPHERE:  ts = "SPHERE";  break;
                        case PrimitiveType::CYLINDER:ts = "CYLINDER";break;
                        case PrimitiveType::MESH:    ts = "MESH";    break;
                        default:                     ts = "MESH";    break;
                    }
                    lua_pushstring(s, ts);
                    return 1;
                }
                lua_pushnil(s); return 1;
            });
            lua_setfield(L, -2, "__index");

            lua_pushcfunction(L, [](lua_State* s)->int {
                MeshComponent* m = *(MeshComponent**)luaL_checkudata(s, 1, "MeshMetaTable");
                if (!m) return 0;
                if (!lua_isstring(s, 2)) return 0;
                const char* k = lua_tostring(s, 2);
                if (strcmp(k, "meshPath") == 0 && lua_isstring(s, 3)) {
                    m->meshFilePath = lua_tostring(s, 3);
                } else if (strcmp(k, "type") == 0) {
                    // accept string names or integer enums
                    if (lua_isstring(s, 3)) {
                        const char* v = lua_tostring(s, 3);
                        if (strcmp(v, "PLANE") == 0) m->type = PrimitiveType::PLANE;
                        else if (strcmp(v, "BOX") == 0 || strcmp(v, "CUBE") == 0) m->type = PrimitiveType::CUBE;
                        else if (strcmp(v, "SPHERE") == 0) m->type = PrimitiveType::SPHERE;
                        else if (strcmp(v, "CYLINDER") == 0) m->type = PrimitiveType::CYLINDER;
                        else if (strcmp(v, "MESH") == 0) m->type = PrimitiveType::MESH;
                    } else if (lua_isinteger(s, 3)) {
                        int iv = (int)lua_tointeger(s, 3);
                        switch (iv) {
                            case 0: m->type = PrimitiveType::PLANE; break;
                            case 1: m->type = PrimitiveType::CUBE;  break;
                            case 2: m->type = PrimitiveType::SPHERE;break;
                            case 3: m->type = PrimitiveType::CYLINDER;break;
                            default: m->type = PrimitiveType::MESH; break;
                        }
                    }
                }
                return 0;
            });
            lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L, 1); }

        //AnimationMetaTable
        if (luaL_newmetatable(L, "AnimationMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                AnimationComponent* a = *(AnimationComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"fbxPath")==0) { lua_pushstring(s, a->fbxPath.c_str()); return 1; }
                else if (strcmp(k,"currentAnimation")==0) { lua_pushinteger(s, a->currentAnimation); return 1; }
                else if (strcmp(k,"isPlaying")==0) { lua_pushboolean(s, a->isPlaying); return 1; }
                else if (strcmp(k,"speed")==0) { lua_pushnumber(s, a->animationSpeed); return 1; }
                else if (strcmp(k,"loop")==0) { lua_pushboolean(s, a->loop); return 1; }
                else if (strcmp(k,"currentClip")==0) { lua_pushinteger(s, a->currentClip); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L,-2,"__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                AnimationComponent* a = *(AnimationComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"fbxPath")==0) { a->fbxPath = luaL_checkstring(s,3); }
                else if (strcmp(k,"currentAnimation")==0) { a->currentAnimation = (int)luaL_checkinteger(s,3); }
                else if (strcmp(k,"isPlaying")==0) { a->isPlaying = lua_toboolean(s,3); }
                else if (strcmp(k,"speed")==0) { a->animationSpeed = (float)luaL_checknumber(s,3); }
                else if (strcmp(k,"loop")==0) { a->loop = lua_toboolean(s,3); }
                else if (strcmp(k,"currentClip")==0) { a->currentClip = (int)luaL_checkinteger(s,3); }
                return 0;
            }); lua_setfield(L,-2,"__newindex");
        } else { lua_pop(L,1); }

        //ColliderMetaTable
        if (luaL_newmetatable(L, "ColliderMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                ColliderComponent* c = *(ColliderComponent**)luaL_checkudata(s, 1, "ColliderMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "type")==0) {
                    const char* ts = "none";
                    switch (c->type) {
                        case ColliderType::AABB:   ts = "aabb"; break;
                        case ColliderType::OBB:    ts = "obb";  break;
                        case ColliderType::SPHERE: ts = "sphere"; break;
                        case ColliderType::PLANE:  ts = "plane"; break;
                        case ColliderType::MESH:   ts = "mesh"; break;
                        default:                   ts = "none"; break;
                    }
                    lua_pushstring(s, ts); return 1;
                }
                if (strcmp(k,"isTrigger")==0) { lua_pushboolean(s, c->isTrigger); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L,-2,"__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                ColliderComponent* c = *(ColliderComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if(strcmp(k, "type")==0) {
                    const char* ts = luaL_checkstring(s,3);
                    if (strcmp(ts,"aabb")==0) c->type = ColliderType::AABB;
                    else if (strcmp(ts,"obb")==0) c->type = ColliderType::OBB;
                    else if (strcmp(ts,"sphere")==0) c->type = ColliderType::SPHERE;
                    else if (strcmp(ts,"plane")==0) c->type = ColliderType::PLANE;
                    else if (strcmp(ts,"mesh")==0) c->type = ColliderType::MESH;
                    else c->type = ColliderType::NONE;
                }
                else if (strcmp(k,"isTrigger")==0) { c->isTrigger = lua_toboolean(s,3); }
                return 0;
            }); lua_setfield(L,-2,"__newindex");
        } else { lua_pop(L,1); }

        //ControllerMetaTable
        if (luaL_newmetatable(L, "ControllerMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                ControllerComponent* c = *(ControllerComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"moveSpeed")==0) { lua_pushnumber(s, c->moveSpeed); return 1; }
                else if (strcmp(k,"zoomSpeed")==0) { lua_pushnumber(s, c->zoomSpeed); return 1; }
                else if (strcmp(k,"sensitivity")==0) { lua_pushnumber(s, c->sensitivity); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L,-2,"__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                ControllerComponent* c = *(ControllerComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"moveSpeed")==0) { c->moveSpeed = (float)luaL_checknumber(s,3); }
                else if (strcmp(k,"zoomSpeed")==0) { c->zoomSpeed = (float)luaL_checknumber(s,3); }
                else if (strcmp(k,"sensitivity")==0) { c->sensitivity = (float)luaL_checknumber(s,3); }
                return 0;
            }); lua_setfield(L,-2,"__newindex");
        } else { lua_pop(L,1); }

        // LayerMetaTable
        if (luaL_newmetatable(L, "LayerMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                LayerComponent* l = *(LayerComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"name")==0) { lua_pushstring(s, l->name.c_str()); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L,-2,"__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                LayerComponent* l = *(LayerComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"name")==0) { l->name = luaL_checkstring(s,3); }
                return 0;
            }); lua_setfield(L,-2,"__newindex");
        } else { lua_pop(L,1); }

        // MeshMetaTable
        if (luaL_newmetatable(L, "MeshMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                MeshComponent* m = *(MeshComponent**)luaL_checkudata(s, 1, "MeshMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "meshPath") == 0) { lua_pushstring(s, m->meshFilePath.c_str()); return 1; }
                else if (strcmp(k, "type") == 0) {
                    const char* ts = "MESH";
                    switch (m->type) {
                        case PrimitiveType::PLANE:   ts = "PLANE";   break;
                        case PrimitiveType::CUBE:    ts = "BOX";     break;
                        case PrimitiveType::SPHERE:  ts = "SPHERE";  break;
                        case PrimitiveType::CYLINDER:ts = "CYLINDER";break;
                        case PrimitiveType::MESH:    ts = "MESH";    break;
                        default:                     ts = "MESH";    break;
                    }
                    lua_pushstring(s, ts);
                    return 1;
                }
                lua_pushnil(s); return 1;
            });
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                MeshComponent* m = *(MeshComponent**)luaL_checkudata(s, 1, "MeshMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "meshPath") == 0 && lua_isstring(s, 3)) {
                    m->meshFilePath = lua_tostring(s, 3);
                } else if (strcmp(k, "type") == 0) {
                    if (lua_isstring(s, 3)) {
                        const char* v = lua_tostring(s, 3);
                        if (strcmp(v, "PLANE") == 0) m->type = PrimitiveType::PLANE;
                        else if (strcmp(v, "BOX") == 0 || strcmp(v, "CUBE") == 0) m->type = PrimitiveType::CUBE;
                        else if (strcmp(v, "SPHERE") == 0) m->type = PrimitiveType::SPHERE;
                        else if (strcmp(v, "CYLINDER") == 0) m->type = PrimitiveType::CYLINDER;
                        else if (strcmp(v, "MESH") == 0) m->type = PrimitiveType::MESH;
                    } else if (lua_isinteger(s, 3)) {
                        int iv = (int)lua_tointeger(s, 3);
                        switch (iv) {
                            case 0: m->type = PrimitiveType::PLANE; break;
                            case 1: m->type = PrimitiveType::CUBE;  break;
                            case 2: m->type = PrimitiveType::SPHERE;break;
                            case 3: m->type = PrimitiveType::CYLINDER;break;
                            default: m->type = PrimitiveType::MESH; break;
                        }
                    }
                }
                return 0;
            });
            lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L, 1); }

        //AudioMetaTable and TagMetaTable
        if( luaL_newmetatable(L, "AudioMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                MyAudioComponent* a = *(MyAudioComponent**)luaL_checkudata(s, 1, "AudioMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "audioFilePath") == 0) { lua_pushstring(s, a->audioFilePath.c_str()); return 1; }
                if (strcmp(k, "type") == 0) {
                    const char* ts = "NONE";
                    switch (a->type) {
                        case AudioType::NONE:   ts = "NONE"; break;
                        case AudioType::MUSIC:  ts = "MUSIC"; break;
                        case AudioType::SFX:    ts = "SFX";  break;
                        case AudioType::SPATIAL: ts = "SPATIAL"; break;
                        default:                ts = "NONE"; break;
                    }
                    lua_pushstring(s, ts);
                    return 1;
                }
                if (strcmp(k, "volume") == 0) { lua_pushnumber(s, a->volume); return 1; }
                if (strcmp(k, "loop") == 0) { lua_pushboolean(s, a->loop); return 1; }
                if (strcmp(k, "playOnStart") == 0) { lua_pushboolean(s, a->playOnStart); return 1; }
                if (strcmp(k, "isPlaying") == 0) { lua_pushboolean(s, a->isPlaying); return 1; }
                lua_pushnil(s); return 1;
            });
            lua_setfield(L, -2, "__index");

            lua_pushcfunction(L, [](lua_State* s)->int {
                MyAudioComponent* a = *(MyAudioComponent**)luaL_checkudata(s, 1, "AudioMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "audioFilePath") == 0 && lua_isstring(s, 3)) {
                    a->audioFilePath = lua_tostring(s, 3);
                } else if (strcmp(k, "type") == 0) {
                    if (lua_isstring(s, 3)) {
                        const char* v = lua_tostring(s, 3);
                        if (strcmp(v, "NONE") == 0) a->type = AudioType::NONE;
                        else if (strcmp(v, "MUSIC") == 0) a->type = AudioType::MUSIC;
                        else if (strcmp(v, "SFX") == 0) a->type = AudioType::SFX;
                        else if (strcmp(v, "SPATIAL") == 0) a->type = AudioType::SPATIAL;
                    }
                } else if (strcmp(k, "volume") == 0) {
                    a->volume = (float)luaL_checknumber(s, 3);
                } else if (strcmp(k, "loop") == 0) {
                    a->loop = lua_toboolean(s, 3);
                } else if (strcmp(k, "playOnStart") == 0) {
                    a->playOnStart = lua_toboolean(s, 3);
                } else if (strcmp(k, "isPlaying") == 0) {
                    a->isPlaying = lua_toboolean(s, 3);
                }
                return 0;
            });
            lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L,1); }

        //ParticleMetaTable
        if( luaL_newmetatable(L, "ParticleMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                ParticuleComponent* p = *(ParticuleComponent**)luaL_checkudata(s, 1, "ParticleMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "nb") == 0) { lua_pushinteger(s, p->nb); return 1; }
                if (strcmp(k, "path") == 0) { lua_pushstring(s, p->path.c_str()); return 1; }
                if (strcmp(k, "position") == 0) { ScriptSystem::pushVec3AsTable(s, p->position); return 1; }
                if (strcmp(k, "offset") == 0) { ScriptSystem::pushVec3AsTable(s, p->offset); return 1; }
                if (strcmp(k, "bouncingFactor") == 0) { lua_pushnumber(s, p->bouncingFactor); return 1; }
                 lua_pushnil(s); return 1;
            });
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                ParticuleComponent* p = *(ParticuleComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k, "nb") == 0) { p->nb = (int)luaL_checkinteger(s, 3); }
                else if (strcmp(k, "path") == 0) { p->path = luaL_checkstring(s, 3); }
                else if (strcmp(k, "position") == 0 && lua_istable(s,3)) { p->position = ScriptSystem::tableToVec3(s,3); }
                else if (strcmp(k, "offset") == 0 && lua_istable(s,3)) { p->offset = ScriptSystem::tableToVec3(s,3); }
                else if (strcmp(k, "bouncingFactor") == 0) { p->bouncingFactor = (float)luaL_checknumber(s, 3); }
                return 0;
            }); lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L,1); }

        //RigidBodyMetaTable
        if( luaL_newmetatable(L, "RigidBodyMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                RigidBodyComponent* r = *(RigidBodyComponent**)luaL_checkudata(s, 1, "RigidBodyMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "gravity") == 0) { ScriptSystem::pushVec3AsTable(s, r->gravity); return 1; }
                if (strcmp(k, "friction") == 0) { lua_pushnumber(s, r->friction); return 1; }
                if (strcmp(k, "bounce") == 0) { lua_pushnumber(s, r->bounce); return 1; }
                if (strcmp(k, "mass") == 0) { lua_pushnumber(s, r->mass); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                RigidBodyComponent* r = *(RigidBodyComponent**)luaL_checkudata(s, 1, "RigidBodyMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "gravity") == 0) { r->gravity = ScriptSystem::tableToVec3(s, 3); return 0; }
                if (strcmp(k, "friction") == 0) { r->friction = (float)luaL_checknumber(s, 3); return 0; }
                if (strcmp(k, "bounce") == 0) { r->bounce = (float)luaL_checknumber(s, 3); return 0; }
                if (strcmp(k, "mass") == 0) { r->mass = (float)luaL_checknumber(s, 3); return 0; }
                return 0;
            }); lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L,1); }
        
        // TagMetaTable
        if( luaL_newmetatable(L, "TagMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                TagComponent* t = *(TagComponent**)luaL_checkudata(s, 1, "TagMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "tag") == 0) { lua_pushstring(s, t->tag.c_str()); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                TagComponent* t = *(TagComponent**)luaL_checkudata(s, 1, "TagMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "tag") == 0) { t->tag = luaL_checkstring(s, 3); }
                return 0;
            }); lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L,1); }

        //TextureMetaTable
        if( luaL_newmetatable(L, "TextureMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                TextureComponent* t = *(TextureComponent**)luaL_checkudata(s, 1, "TextureMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "path") == 0) { lua_pushstring(s, t->path.c_str()); return 1; }
                if (strcmp(k, "positionX") == 0) { lua_pushnumber(s, t->positionX); return 1; }
                if (strcmp(k, "positionY") == 0) { lua_pushnumber(s, t->positionY); return 1; }
                if (strcmp(k, "width") == 0) { lua_pushnumber(s, t->width); return 1; }
                if (strcmp(k, "height") == 0) { lua_pushnumber(s, t->height); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                TextureComponent* t = *(TextureComponent**)luaL_checkudata(s, 1, "TextureMetaTable");
                const char* k = luaL_checkstring(s, 2);
                if (strcmp(k, "path") == 0) { t->path = luaL_checkstring(s, 3); }
                else if (strcmp(k, "positionX") == 0) { t->positionX = (float)luaL_checknumber(s, 3); }
                else if (strcmp(k, "positionY") == 0) { t->positionY = (float)luaL_checknumber(s, 3); }
                else if (strcmp(k, "width") == 0) { t->width = (float)luaL_checknumber(s, 3); }
                else if (strcmp(k, "height") == 0) { t->height = (float)luaL_checknumber(s, 3); }
                return 0;
            }); lua_setfield(L, -2, "__newindex");
        } else { lua_pop(L,1); }
    }

    // Build a Lua table representing an entity and all available components.
    static void pushEntityTable(lua_State* L, ScriptSystem* sys, uint32_t entityId) {
        if (!sys || !sys->entityManager) { lua_pushnil(L); return; }
        ensureComponentMetatables(L);
        lua_newtable(L); // result table
        if (sys->entityManager->HasComponent<AnimationComponent>(entityId)) { AnimationComponent* anim = &sys->entityManager->GetComponent<AnimationComponent>(entityId); pushComponentUserdata<AnimationComponent>(L, anim, "AnimationMetaTable"); lua_setfield(L, -2, "animation"); }
        if (sys->entityManager->HasComponent<CameraComponent>(entityId)) { CameraComponent* c = &sys->entityManager->GetComponent<CameraComponent>(entityId); pushComponentUserdata<CameraComponent>(L, c, "CameraMetaTable"); lua_setfield(L, -2, "camera"); }
        if (sys->entityManager->HasComponent<ColliderComponent>(entityId)) { ColliderComponent* col = &sys->entityManager->GetComponent<ColliderComponent>(entityId); pushComponentUserdata<ColliderComponent>(L, col, "ColliderMetaTable"); lua_setfield(L, -2, "collider"); }
        if (sys->entityManager->HasComponent<ControllerComponent>(entityId)) { ControllerComponent* ctrl = &sys->entityManager->GetComponent<ControllerComponent>(entityId); pushComponentUserdata<ControllerComponent>(L, ctrl, "ControllerMetaTable"); lua_setfield(L, -2, "controller"); }
        if (sys->entityManager->HasComponent<LayerComponent>(entityId)) { LayerComponent* ly = &sys->entityManager->GetComponent<LayerComponent>(entityId); pushComponentUserdata<LayerComponent>(L, ly, "LayerMetaTable"); lua_setfield(L, -2, "layer"); }
        if (sys->entityManager->HasComponent<LightComponent>(entityId)) { LightComponent* li = &sys->entityManager->GetComponent<LightComponent>(entityId); pushComponentUserdata<LightComponent>(L, li, "LightMetaTable"); lua_setfield(L, -2, "light"); }
        if (sys->entityManager->HasComponent<MaterialComponent>(entityId)) { MaterialComponent* m = &sys->entityManager->GetComponent<MaterialComponent>(entityId); pushComponentUserdata<MaterialComponent>(L, m, "MaterialMetaTable"); lua_setfield(L, -2, "material"); }
        if (sys->entityManager->HasComponent<MeshComponent>(entityId)) { MeshComponent* mm = &sys->entityManager->GetComponent<MeshComponent>(entityId); pushComponentUserdata<MeshComponent>(L, mm, "MeshMetaTable"); lua_setfield(L, -2, "mesh"); }
        if (sys->entityManager->HasComponent<MyAudioComponent>(entityId)) { MyAudioComponent* a = &sys->entityManager->GetComponent<MyAudioComponent>(entityId); pushComponentUserdata<MyAudioComponent>(L, a, "AudioMetaTable"); lua_setfield(L, -2, "audio"); }
        if (sys->entityManager->HasComponent<ParticuleComponent>(entityId)) { ParticuleComponent* p = &sys->entityManager->GetComponent<ParticuleComponent>(entityId); pushComponentUserdata<ParticuleComponent>(L, p, "ParticleMetaTable"); lua_setfield(L, -2, "particle"); }
        if (sys->entityManager->HasComponent<RigidBodyComponent>(entityId)) { RigidBodyComponent* r = &sys->entityManager->GetComponent<RigidBodyComponent>(entityId); pushComponentUserdata<RigidBodyComponent>(L, r, "RigidBodyMetaTable"); lua_setfield(L, -2, "rigidbody"); }
        if (sys->entityManager->HasComponent<TagComponent>(entityId)) { TagComponent* tg = &sys->entityManager->GetComponent<TagComponent>(entityId); pushComponentUserdata<TagComponent>(L, tg, "TagMetaTable"); lua_setfield(L, -2, "tag"); }
        if (sys->entityManager->HasComponent<TextureComponent>(entityId)) { TextureComponent* tex = &sys->entityManager->GetComponent<TextureComponent>(entityId); pushComponentUserdata<TextureComponent>(L, tex, "TextureMetaTable"); lua_setfield(L, -2, "texture"); }
        if (sys->entityManager->HasComponent<TransformComponent>(entityId)) {TransformComponent* t = &sys->entityManager->GetComponent<TransformComponent>(entityId);pushComponentUserdata<TransformComponent>(L, t, "TransformMetaTable"); lua_setfield(L, -2, "transform");}
        lua_pushinteger(L, (lua_Integer)entityId); lua_setfield(L, -2, "id");
    }

    // Lua C API functions
    static int lua_get_entity(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys || !sys->entityManager) { lua_pushnil(L); return 1; }
        int entId = (int)luaL_checkinteger(L, 1);
        if (!sys->entityManager->HasEntity((uint32_t)entId)) { lua_pushnil(L); return 1; }
        pushEntityTable(L, sys, (uint32_t)entId);
        return 1;
    }

    static int lua_set_transform(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys || !sys->entityManager) return 0;
        int entId = (int)luaL_checkinteger(L, 1);
        if (!lua_istable(L, 2)) return 0;
        if (!sys->entityManager->HasComponent<TransformComponent>((uint32_t)entId)) return 0;
        TransformComponent& t = sys->entityManager->GetComponent<TransformComponent>((uint32_t)entId);
        lua_getfield(L, 2, "position"); if (lua_istable(L, -1)) t.position = tableToVec3(L, lua_gettop(L)); lua_pop(L,1);
        lua_getfield(L, 2, "rotation"); if (lua_istable(L, -1)) t.rotation = tableToVec3(L, lua_gettop(L)); lua_pop(L,1);
        lua_getfield(L, 2, "scale");    if (lua_istable(L, -1)) t.scale    = tableToVec3(L, lua_gettop(L)); lua_pop(L,1);
        lua_getfield(L, 2, "parent"); if (lua_isinteger(L, -1)) t.parent = (EntityID)lua_tointeger(L, -1); lua_pop(L,1);
        lua_getfield(L, 2, "children"); if (lua_istable(L, -1)) {
            t.children.clear();
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                if (lua_isinteger(L, -1)) {
                    EntityID childId = (EntityID)lua_tointeger(L, -1);
                    t.children.push_back(childId);
                }
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
        return 0;
    }

    static int lua_create_entity(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys || !sys->entityManager) { lua_pushnil(L); return 1; }
        EntityID id = sys->entityManager->CreateEntity(); lua_pushinteger(L, (lua_Integer)id); 
        if (sys->entities) {
            sys->entities->push_back(Entity{id});
        }
        return 1;
    }

    static int lua_destroy_entity(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys || !sys->entityManager) return 0;
        EntityID id = (EntityID)luaL_checkinteger(L, 1);
        if (sys->entityManager->HasEntity(id)) sys->entityManager->DestroyEntity(id);
        if (sys->entities) {
            auto it = std::find_if(sys->entities->begin(), sys->entities->end(),[id](const Entity& e){ return e.id == id; });
            if (it != sys->entities->end()) {
                sys->entities->erase(it);
            }
        }
        return 0;
    }

    static int lua_add_transform(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys || !sys->entityManager) { lua_pushnil(L); return 1; }
        EntityID id = (EntityID)luaL_checkinteger(L, 1);
        TransformComponent t; sys->entityManager->AddComponent<TransformComponent>(id, t);
        TransformComponent& ref = sys->entityManager->GetComponent<TransformComponent>(id);
        TransformComponent** udata = (TransformComponent**)lua_newuserdata(L, sizeof(TransformComponent*)); *udata = &ref;
        luaL_getmetatable(L, "TransformMetaTable"); lua_setmetatable(L, -2); return 1;
    }

    static int lua_get_component(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys || !sys->entityManager) { lua_pushnil(L); return 1; }
        EntityID id = (EntityID)luaL_checkinteger(L, 1);
        const char* cname = luaL_checkstring(L, 2);
        std::string name(cname);
        if (name == "Transform") {
            if (!sys->entityManager->HasComponent<TransformComponent>(id)) { lua_pushnil(L); return 1; }
            TransformComponent& ref = sys->entityManager->GetComponent<TransformComponent>(id);
            TransformComponent** udata = (TransformComponent**)lua_newuserdata(L, sizeof(TransformComponent*)); *udata = &ref; luaL_getmetatable(L, "TransformMetaTable"); lua_setmetatable(L, -2); return 1;
        } else if (name == "Camera") {
            if (!sys->entityManager->HasComponent<CameraComponent>(id)) { lua_pushnil(L); return 1; }
            CameraComponent& ref = sys->entityManager->GetComponent<CameraComponent>(id);
            CameraComponent** udata = (CameraComponent**)lua_newuserdata(L, sizeof(CameraComponent*)); *udata = &ref; luaL_getmetatable(L, "CameraMetaTable"); lua_setmetatable(L, -2); return 1;
        } else if (name == "Material") {
            if (!sys->entityManager->HasComponent<MaterialComponent>(id)) { lua_pushnil(L); return 1; }
            MaterialComponent& ref = sys->entityManager->GetComponent<MaterialComponent>(id);
            MaterialComponent** udata = (MaterialComponent**)lua_newuserdata(L, sizeof(MaterialComponent*)); *udata = &ref; luaL_getmetatable(L, "MaterialMetaTable"); lua_setmetatable(L, -2); return 1;
        } else if (name == "Mesh") {
            if (!sys->entityManager->HasComponent<MeshComponent>(id)) { lua_pushnil(L); return 1; }
            MeshComponent& ref = sys->entityManager->GetComponent<MeshComponent>(id);
            MeshComponent** udata = (MeshComponent**)lua_newuserdata(L, sizeof(MeshComponent*)); *udata = &ref; luaL_getmetatable(L, "MeshMetaTable"); lua_setmetatable(L, -2); return 1;
        } else if (name == "Light") {
            if (!sys->entityManager->HasComponent<LightComponent>(id)) { lua_pushnil(L); return 1; }
            LightComponent& ref = sys->entityManager->GetComponent<LightComponent>(id);
            LightComponent** udata = (LightComponent**)lua_newuserdata(L, sizeof(LightComponent*)); *udata = &ref; luaL_getmetatable(L, "LightMetaTable"); lua_setmetatable(L, -2); return 1;
        }
        lua_pushnil(L); return 1;
    }

    static int lua_add_component(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys || !sys->entityManager) { lua_pushnil(L); return 1; }

        EntityID id = (EntityID)luaL_checkinteger(L, 1);
        const char* type = luaL_checkstring(L, 2);
        bool hasTable = lua_gettop(L) >= 3 && lua_istable(L, 3);

        std::string t(type);
        if (t == "Transform") {
            TransformComponent comp;
            if (hasTable) {
                lua_getfield(L, 3, "position");
                if (lua_istable(L, -1)) comp.position = tableToVec3(L, lua_gettop(L));
                lua_pop(L,1);
                lua_getfield(L, 3, "rotation");
                if (lua_istable(L, -1)) comp.rotation = tableToVec3(L, lua_gettop(L));
                lua_pop(L,1);
                lua_getfield(L, 3, "scale");
                if (lua_istable(L, -1)) comp.scale = tableToVec3(L, lua_gettop(L));
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<TransformComponent>(id, comp);
            TransformComponent& ref = sys->entityManager->GetComponent<TransformComponent>(id);
            TransformComponent** udata = (TransformComponent**)lua_newuserdata(L, sizeof(TransformComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "TransformMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Material") {
            MaterialComponent comp;
            if (hasTable) {
                lua_getfield(L, 3, "color");
                if (lua_istable(L, -1)) comp.color = colorTableToVec3(L, lua_gettop(L));
                lua_pop(L,1);
            }
            comp.type = MaterialComponent::Type::Color;
            comp.setColor(glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f),1.0f);
            sys->entityManager->AddComponent<MaterialComponent>(id, comp);
            MaterialComponent& ref = sys->entityManager->GetComponent<MaterialComponent>(id);
            MaterialComponent** udata = (MaterialComponent**)lua_newuserdata(L, sizeof(MaterialComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "MaterialMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Camera") {
            CameraComponent comp;
            if (hasTable) {
                lua_getfield(L, 3, "target");
                if (lua_istable(L, -1)) comp.target = tableToVec3(L, lua_gettop(L));
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<CameraComponent>(id, comp);
            CameraComponent& ref = sys->entityManager->GetComponent<CameraComponent>(id);
            CameraComponent** udata = (CameraComponent**)lua_newuserdata(L, sizeof(CameraComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "CameraMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Mesh") {
            MeshComponent comp;
            if (hasTable) {
                lua_getfield(L, 3, "type");
                if (lua_isstring(L, -1)) {
                    std::string mtype = lua_tostring(L, -1);
                    lua_pop(L, 1);

                    if (mtype == "primitive") {
                        // read optional params
                        lua_getfield(L, 3, "name");
                        std::string pname = "BOX";
                        if (lua_isstring(L, -1)) pname = lua_tostring(L, -1);
                        lua_pop(L, 1);

                        lua_getfield(L, 3, "subdivisions");
                        if (lua_isnumber(L, -1)) comp.subdivisions = (int)lua_tonumber(L, -1);
                        lua_pop(L, 1);

                        lua_getfield(L, 3, "width");
                        if (lua_isnumber(L, -1)) comp.width = (float)lua_tonumber(L, -1);
                        lua_pop(L, 1);

                        lua_getfield(L, 3, "height");
                        if (lua_isnumber(L, -1)) comp.height = (float)lua_tonumber(L, -1);
                        lua_pop(L, 1);

                        // create primitive (uses MeshComponent::loadPrimitive)
                        comp.loadPrimitive(pname);
                    } else if (mtype == "file") {
                        lua_getfield(L, 3, "path");
                        if (lua_isstring(L, -1)) {
                            std::string meshPath = lua_tostring(L, -1);
                            comp.load_OFF(meshPath);
                        }
                        lua_pop(L, 1);
                    }
                } else {
                    lua_pop(L, 1);
                }
            }
            sys->entityManager->AddComponent<MeshComponent>(id, comp);
            MeshComponent& ref = sys->entityManager->GetComponent<MeshComponent>(id);
            MeshComponent** udata = (MeshComponent**)lua_newuserdata(L, sizeof(MeshComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "MeshMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Light") {
            LightComponent comp;
            if( hasTable) {
                lua_getfield(L, 3, "intensity");
                if (lua_isnumber(L, -1)) comp.intensity = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<LightComponent>(id, comp);
            LightComponent& ref = sys->entityManager->GetComponent<LightComponent>(id);
            LightComponent** udata = (LightComponent**)lua_newuserdata(L, sizeof(LightComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "LightMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Controller") {
            ControllerComponent comp;
            if( hasTable) {
                lua_getfield(L, 3, "moveSpeed");
                if (lua_isnumber(L, -1)) comp.moveSpeed = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "zoomSpeed");
                if (lua_isnumber(L, -1)) comp.zoomSpeed = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "sensitivity");
                if (lua_isnumber(L, -1)) comp.sensitivity = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<ControllerComponent>(id, comp);
            ControllerComponent& ref = sys->entityManager->GetComponent<ControllerComponent>(id);
            ControllerComponent** udata = (ControllerComponent**)lua_newuserdata(L, sizeof(ControllerComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "ControllerMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Audio") {
            MyAudioComponent comp;
            if( hasTable) {
                lua_getfield(L, 3, "audioFilePath");
                if (lua_isstring(L, -1)) comp.audioFilePath = lua_tostring(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "volume");
                if (lua_isnumber(L, -1)) comp.volume = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "loop");
                if (lua_isboolean(L, -1)) comp.loop = lua_toboolean(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "playOnStart");
                if (lua_isboolean(L, -1)) comp.playOnStart = lua_toboolean(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "isPlaying");
                if (lua_isboolean(L, -1)) comp.isPlaying = lua_toboolean(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "type");
                if (lua_isstring(L, -1)) {
                    std::string atype = lua_tostring(L, -1);
                    if (atype == "Music") comp.type = AudioType::MUSIC;
                    else if (atype == "SFX") comp.type = AudioType::SFX;
                    else if(atype == "SPATIAL") comp.type = AudioType::SPATIAL;
                    else comp.type = AudioType::NONE;
                }
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<MyAudioComponent>(id, comp);
            MyAudioComponent& ref = sys->entityManager->GetComponent<MyAudioComponent>(id);
            MyAudioComponent** udata = (MyAudioComponent**)lua_newuserdata(L, sizeof(MyAudioComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "AudioMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Tag") {
            TagComponent comp;
            if( hasTable) {
                lua_getfield(L, 3, "tag");
                if (lua_isstring(L, -1)) comp.tag = lua_tostring(L, -1);
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<TagComponent>(id, comp);
            TagComponent& ref = sys->entityManager->GetComponent<TagComponent>(id);
            TagComponent** udata = (TagComponent**)lua_newuserdata(L, sizeof(TagComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "TagMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Layer") {
            LayerComponent comp;
            if( hasTable) {
                lua_getfield(L, 3, "layerNumber");
                if (lua_isstring(L, -1)) comp.name = lua_tostring(L, -1);
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<LayerComponent>(id, comp);
            LayerComponent& ref = sys->entityManager->GetComponent<LayerComponent>(id);
            LayerComponent** udata = (LayerComponent**)lua_newuserdata(L, sizeof(LayerComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "LayerMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Collider") {
            ColliderComponent comp;
            if( hasTable) {
                lua_getfield(L, 3, "isTrigger");
                if (lua_isboolean(L, -1)) comp.isTrigger = lua_toboolean(L, -1);
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<ColliderComponent>(id, std::move(comp));
            ColliderComponent& ref = sys->entityManager->GetComponent<ColliderComponent>(id);
            ColliderComponent** udata = (ColliderComponent**)lua_newuserdata(L, sizeof(ColliderComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "ColliderMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        }  else if (t == "RigidBody") {
            RigidBodyComponent comp;
            sys->entityManager->AddComponent<RigidBodyComponent>(id, comp);
            RigidBodyComponent& ref = sys->entityManager->GetComponent<RigidBodyComponent>(id);
            RigidBodyComponent** udata = (RigidBodyComponent**)lua_newuserdata(L, sizeof(RigidBodyComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "RigidBodyMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if( t == "Particle") {
            ParticuleComponent comp;
            sys->entityManager->AddComponent<ParticuleComponent>(id, comp);
            ParticuleComponent& ref = sys->entityManager->GetComponent<ParticuleComponent>(id);
            ParticuleComponent** udata = (ParticuleComponent**)lua_newuserdata(L, sizeof(ParticuleComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "ParticleMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if( t == "Animation") {
            AnimationComponent comp;
            sys->entityManager->AddComponent<AnimationComponent>(id, comp);
            AnimationComponent& ref = sys->entityManager->GetComponent<AnimationComponent>(id);
            AnimationComponent** udata = (AnimationComponent**)lua_newuserdata(L, sizeof(AnimationComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "AnimationMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if( t =="texture"){
            TextureComponent comp;
            if( hasTable) {
                lua_getfield(L, 3, "path");
                if (lua_isstring(L, -1)) comp.path = lua_tostring(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "positionX");
                if (lua_isnumber(L, -1)) comp.positionX = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "positionY");
                if (lua_isnumber(L, -1)) comp.positionY = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "width");
                if (lua_isnumber(L, -1)) comp.width = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
                lua_getfield(L, 3, "height");
                if (lua_isnumber(L, -1)) comp.height = (float)lua_tonumber(L, -1);
                lua_pop(L,1);
            }
            sys->entityManager->AddComponent<TextureComponent>(id, comp);
            TextureComponent& ref = sys->entityManager->GetComponent<TextureComponent>(id);
            TextureComponent** udata = (TextureComponent**)lua_newuserdata(L, sizeof(TextureComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "TextureMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        }
        lua_pushnil(L);
        return 1;
    }

    static int lua_set_global(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys) return 0;
        const char* name = luaL_checkstring(L, 1);
        GlobalValue gv = getGlobalValueFromLua(L, 2);
        sys->globalValues[name] = gv;
        return 0;
    }

    static int lua_get_global(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys) return 0;
        const char* name = luaL_checkstring(L, 1);
        auto it = sys->globalValues.find(name);
        if (it != sys->globalValues.end()) {
            pushGlobalValueToLua(L, it->second);
            return 1;
        }
        lua_pushnil(L);
        return 1;
    }

    static int lua_change_scene(lua_State* L) {
        lua_getfield(L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);
        ScriptSystem* sys = (ScriptSystem*)lua_touserdata(L, -1);
        lua_pop(L, 1);
        if (!sys || !sys->sceneManager) return 0;
        const char* path = luaL_checkstring(L, 1);
        if (!path) return 0;
        std::string fullPath = path;
        sys->sceneManager->requestChangeScene(fullPath);
        return 0;
    }

public:
    void initScript(LuaScriptComponent& script, uint32_t entityId) {
        script.L = luaL_newstate(); luaL_openlibs(script.L);
        if (!std::filesystem::exists(script.luaScriptPath)) { std::cerr << "Script introuvable: " << script.luaScriptPath << std::endl; return; }
        // store ScriptSystem* in the lua registry so C functions can access entityManager
        lua_pushlightuserdata(script.L, (void*)this); lua_setfield(script.L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);

        // ensure metatables exist before running the script
        ensureComponentMetatables(script.L);

        // Register utility globals
        lua_pushcfunction(script.L, lua_get_entity); lua_setglobal(script.L, "get_entity");
        lua_pushcfunction(script.L, lua_set_transform); lua_setglobal(script.L, "set_transform");
        lua_pushcfunction(script.L, lua_create_entity); lua_setglobal(script.L, "create_entity");
        lua_pushcfunction(script.L, lua_destroy_entity); lua_setglobal(script.L, "destroy_entity");
        lua_pushcfunction(script.L, lua_add_transform); lua_setglobal(script.L, "add_transform");
        lua_pushcfunction(script.L, lua_get_component); lua_setglobal(script.L, "get_component");
        lua_pushcfunction(script.L, lua_add_component); lua_setglobal(script.L, "add_component");
        lua_pushcfunction(script.L, lua_set_global); lua_setglobal(script.L, "set_global");
        lua_pushcfunction(script.L, lua_get_global); lua_setglobal(script.L, "get_global");
        lua_pushcfunction(script.L, lua_change_scene); lua_setglobal(script.L, "change_scene");
        if (luaL_dofile(script.L, script.luaScriptPath.c_str()) != LUA_OK) { std::cerr << "Erreur Lua : " << lua_tostring(script.L, -1) << std::endl; return; }

        // Build 'this' table for the script's entity
        pushEntityTable(script.L, this, entityId); lua_setglobal(script.L, "this");


        // Call onInit if present
        lua_getglobal(script.L, "onInit");
        if (lua_isfunction(script.L, -1)) {
            if (lua_pcall(script.L, 0, 0, 0) != LUA_OK) std::cerr << "Erreur dans onInit: " << lua_tostring(script.L, -1) << std::endl;
        } else lua_pop(script.L, 1);
        script.initialized = true;
    }

    void updateScript(LuaScriptComponent& script, float dt) {
        if (!script.initialized || !script.L) return;
        lua_getglobal(script.L, "onUpdate");
        if (lua_isfunction(script.L, -1)) { lua_pushnumber(script.L, dt); if (lua_pcall(script.L, 1, 0, 0) != LUA_OK) std::cerr << "Erreur dans onUpdate: " << lua_tostring(script.L, -1) << std::endl; }
        else lua_pop(script.L, 1);
    }

    void onUpdate(float deltaTime) { for (auto& [id, script] : luaScripts) if (script) updateScript(*script, deltaTime); }

    void shutdownScript(LuaScriptComponent& script) { if (script.L) lua_close(script.L); script.L = nullptr; }

    void onInput(const InputEvent& event) {
        for (auto& [id, script] : luaScripts) {
            if (!script || !script->L) continue;
            lua_State* L = script->L; lua_getglobal(L, "onInput"); if (!lua_isfunction(L, -1)) { lua_pop(L,1); continue; }
            lua_newtable(L); lua_pushnumber(L, event.dt); lua_setfield(L, -2, "dt"); lua_pushnumber(L, event.scroll); lua_setfield(L, -2, "scroll"); lua_pushnumber(L, event.mouseDeltaX); lua_setfield(L, -2, "mouseDeltaX"); lua_pushnumber(L, event.mouseDeltaY); lua_setfield(L, -2, "mouseDeltaY"); lua_pushboolean(L, event.mouseMoved); lua_setfield(L, -2, "mouseMoved");
            lua_newtable(L); int i = 1; for (const auto& btn : event.buttons) { lua_pushstring(L, btn.c_str()); lua_rawseti(L, -2, i++); } lua_setfield(L, -2, "buttons");
            if (lua_pcall(L, 1, 0, 0) != LUA_OK) { std::cerr << "Erreur Lua onInput: " << lua_tostring(L, -1) << std::endl; lua_pop(L, 1); }
        }
    }

    // Backwards-compatible bind helper
    void bind(LuaScriptComponent& script, TransformComponent* t, CameraComponent* c, MaterialComponent* m, LightComponent* l, MeshComponent* me, MyAudioComponent* a, RigidBodyComponent* r, TextureComponent* tex, ColliderComponent* col, ControllerComponent* ctrl, TagComponent* tag, LayerComponent* layer, ParticuleComponent* p, AnimationComponent* anim) {
        lua_State* L = script.L;
        if (!L) return;
        if (t) { TransformComponent** udata = (TransformComponent**)lua_newuserdata(L, sizeof(TransformComponent*)); *udata = t; luaL_getmetatable(L, "TransformMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "transform"); }
        if (c) { CameraComponent** udata = (CameraComponent**)lua_newuserdata(L, sizeof(CameraComponent*)); *udata = c; luaL_getmetatable(L, "CameraMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "camera"); }
        if (m) { MaterialComponent** udata = (MaterialComponent**)lua_newuserdata(L, sizeof(MaterialComponent*)); *udata = m; luaL_getmetatable(L, "MaterialMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "material"); }
        if (l) { LightComponent** udata = (LightComponent**)lua_newuserdata(L, sizeof(LightComponent*)); *udata = l; luaL_getmetatable(L, "LightMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "light");}
        if (me) { MeshComponent** udata = (MeshComponent**)lua_newuserdata(L, sizeof(MeshComponent*)); *udata = me; luaL_getmetatable(L, "MeshMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "mesh"); }
        if (a) { MyAudioComponent** udata = (MyAudioComponent**)lua_newuserdata(L, sizeof(MyAudioComponent*)); *udata = a; luaL_getmetatable(L, "AudioMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "audio"); }
        if (r) { RigidBodyComponent** udata = (RigidBodyComponent**)lua_newuserdata(L, sizeof(RigidBodyComponent*)); *udata = r; luaL_getmetatable(L, "RigidBodyMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "rigidbody"); }
        if (tex) { TextureComponent** udata = (TextureComponent**)lua_newuserdata(L, sizeof(TextureComponent*)); *udata = tex; luaL_getmetatable(L, "TextureMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "texture"); }
        if (col) { ColliderComponent** udata = (ColliderComponent**)lua_newuserdata(L, sizeof(ColliderComponent*)); *udata = col; luaL_getmetatable(L, "ColliderMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "collider"); }
        if (ctrl) { ControllerComponent** udata = (ControllerComponent**)lua_newuserdata(L, sizeof(ControllerComponent*)); *udata = ctrl; luaL_getmetatable(L, "ControllerMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "controller"); }
        if (tag) { TagComponent** udata = (TagComponent**)lua_newuserdata(L, sizeof(TagComponent*)); *udata = tag; luaL_getmetatable(L, "TagMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "tag"); }
        if (layer) { LayerComponent** udata = (LayerComponent**)lua_newuserdata(L, sizeof(LayerComponent*)); *udata = layer; luaL_getmetatable(L, "LayerMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "layer"); }
        if (p) { ParticuleComponent** udata = (ParticuleComponent**)lua_newuserdata(L, sizeof(ParticuleComponent*)); *udata = p; luaL_getmetatable(L, "ParticleMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "particle"); }
        if (anim) { AnimationComponent** udata = (AnimationComponent**)lua_newuserdata(L, sizeof(AnimationComponent*)); *udata = anim; luaL_getmetatable(L, "AnimationMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "animation"); }

    }
};