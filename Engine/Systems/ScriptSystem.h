#pragma once

#include "../Components/LuaScriptComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/MaterialComponent.h"
#include "../Components/MeshComponent.h"
#include "../Components/LightComponent.h"
#include "../Components/ControllerComponent.h"
#include "../Components/MyAudioComponent.h"
#include "../Components/TagConponent.h"
#include "../Components/LayerComponent.h"
#include "../Components/InputEvent.h"
#include "../Systems/EntityManager.h"
#include "../Systems/Dispatcher.h"

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

struct ScriptSystem {
    EntityManager* entityManager = nullptr;
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
                if (strcmp(key, "x")==0) { lua_pushnumber(s, t->position.x); return 1; }
                if (strcmp(key, "y")==0) { lua_pushnumber(s, t->position.y); return 1; }
                if (strcmp(key, "z")==0) { lua_pushnumber(s, t->position.z); return 1; }
                lua_pushnil(s); return 1;
            });
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                TransformComponent* t = *(TransformComponent**)lua_touserdata(s, 1);
                const char* key = luaL_checkstring(s, 2);
                if (strcmp(key, "position")==0 && lua_istable(s,3)) { t->position = ScriptSystem::tableToVec3(s, 3); return 0; }
                if (strcmp(key, "rotation")==0 && lua_istable(s,3)) { t->rotation = ScriptSystem::tableToVec3(s, 3); return 0; }
                if (strcmp(key, "scale")==0    && lua_istable(s,3)) { t->scale    = ScriptSystem::tableToVec3(s, 3); return 0; }
                if (strcmp(key, "x")==0) { t->position.x = (float)luaL_checknumber(s,3); return 0; }
                if (strcmp(key, "y")==0) { t->position.y = (float)luaL_checknumber(s,3); return 0; }
                if (strcmp(key, "z")==0) { t->position.z = (float)luaL_checknumber(s,3); return 0; }
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
                lua_pushnil(s); return 1;
            }); lua_setfield(L,-2,"__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                CameraComponent* c = *(CameraComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"target")==0 && lua_istable(s,3)) c->target = ScriptSystem::tableToVec3(s,3);
                else if (strcmp(k,"yaw")==0) c->yaw = (float)luaL_checknumber(s,3);
                else if (strcmp(k,"pitch")==0) c->pitch = (float)luaL_checknumber(s,3);
                return 0;
            }); lua_setfield(L,-2,"__newindex");
        } else { lua_pop(L,1); }

        // MaterialMetaTable
        if (luaL_newmetatable(L, "MaterialMetaTable")) {
            lua_pushcfunction(L, [](lua_State* s)->int {
                MaterialComponent* m = *(MaterialComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"color")==0) { ScriptSystem::pushVec3AsColorTable(s, m->color); return 1; }
                if (strcmp(k,"type")==0) { lua_pushstring(s, m->type==MaterialComponent::Type::Color?"color":(m->type==MaterialComponent::Type::Texture?"texture":"none")); return 1; }
                lua_pushnil(s); return 1;
            }); lua_setfield(L,-2,"__index");
            lua_pushcfunction(L, [](lua_State* s)->int {
                MaterialComponent* m = *(MaterialComponent**)lua_touserdata(s,1);
                const char* k = luaL_checkstring(s,2);
                if (strcmp(k,"color")==0 && lua_istable(s,3)) m->color = ScriptSystem::colorTableToVec3(s,3);
                else if (strcmp(k,"type")==0) { const char* v = luaL_checkstring(s,3); if (strcmp(v,"color")==0) m->type = MaterialComponent::Type::Color; else if (strcmp(v,"texture")==0) m->type = MaterialComponent::Type::Texture; else m->type = MaterialComponent::Type::None; }
                return 0;
            }); lua_setfield(L,-2,"__newindex");
        } else { lua_pop(L,1); }

        // Lightweight metatables for other component types
        auto mkEmptyMeta = [&](const char* name){ if (luaL_newmetatable(L, name)) lua_pop(L,1); else lua_pop(L,1); };
        mkEmptyMeta("MeshMetaTable"); mkEmptyMeta("LightMetaTable"); mkEmptyMeta("ControllerMetaTable");
        mkEmptyMeta("AudioMetaTable"); mkEmptyMeta("TagMetaTable"); mkEmptyMeta("LayerMetaTable");
    }

    // Build a Lua table representing an entity and all available components.
    static void pushEntityTable(lua_State* L, ScriptSystem* sys, uint32_t entityId) {
        if (!sys || !sys->entityManager) { lua_pushnil(L); return; }
        ensureComponentMetatables(L);
        lua_newtable(L); // result table
        if (sys->entityManager->HasComponent<TransformComponent>(entityId)) {
            TransformComponent* t = &sys->entityManager->GetComponent<TransformComponent>(entityId);
            pushComponentUserdata<TransformComponent>(L, t, "TransformMetaTable"); lua_setfield(L, -2, "transform");
        }
        if (sys->entityManager->HasComponent<CameraComponent>(entityId)) { CameraComponent* c = &sys->entityManager->GetComponent<CameraComponent>(entityId); pushComponentUserdata<CameraComponent>(L, c, "CameraMetaTable"); lua_setfield(L, -2, "camera"); }
        if (sys->entityManager->HasComponent<MaterialComponent>(entityId)) { MaterialComponent* m = &sys->entityManager->GetComponent<MaterialComponent>(entityId); pushComponentUserdata<MaterialComponent>(L, m, "MaterialMetaTable"); lua_setfield(L, -2, "material"); }
        if (sys->entityManager->HasComponent<MeshComponent>(entityId)) { MeshComponent* mm = &sys->entityManager->GetComponent<MeshComponent>(entityId); pushComponentUserdata<MeshComponent>(L, mm, "MeshMetaTable"); lua_setfield(L, -2, "mesh"); }
        if (sys->entityManager->HasComponent<LightComponent>(entityId)) { LightComponent* li = &sys->entityManager->GetComponent<LightComponent>(entityId); pushComponentUserdata<LightComponent>(L, li, "LightMetaTable"); lua_setfield(L, -2, "light"); }
        if (sys->entityManager->HasComponent<ControllerComponent>(entityId)) { ControllerComponent* ctrl = &sys->entityManager->GetComponent<ControllerComponent>(entityId); pushComponentUserdata<ControllerComponent>(L, ctrl, "ControllerMetaTable"); lua_setfield(L, -2, "controller"); }
        if (sys->entityManager->HasComponent<MyAudioComponent>(entityId)) { MyAudioComponent* a = &sys->entityManager->GetComponent<MyAudioComponent>(entityId); pushComponentUserdata<MyAudioComponent>(L, a, "AudioMetaTable"); lua_setfield(L, -2, "audio"); }
        if (sys->entityManager->HasComponent<TagComponent>(entityId)) { TagComponent* tg = &sys->entityManager->GetComponent<TagComponent>(entityId); pushComponentUserdata<TagComponent>(L, tg, "TagMetaTable"); lua_setfield(L, -2, "tag"); }
        if (sys->entityManager->HasComponent<LayerComponent>(entityId)) { LayerComponent* ly = &sys->entityManager->GetComponent<LayerComponent>(entityId); pushComponentUserdata<LayerComponent>(L, ly, "LayerMetaTable"); lua_setfield(L, -2, "layer"); }
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
            sys->entityManager->AddComponent<LightComponent>(id, comp);
            LightComponent& ref = sys->entityManager->GetComponent<LightComponent>(id);
            LightComponent** udata = (LightComponent**)lua_newuserdata(L, sizeof(LightComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "LightMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Controller") {
            ControllerComponent comp;
            sys->entityManager->AddComponent<ControllerComponent>(id, comp);
            ControllerComponent& ref = sys->entityManager->GetComponent<ControllerComponent>(id);
            ControllerComponent** udata = (ControllerComponent**)lua_newuserdata(L, sizeof(ControllerComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "ControllerMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Audio") {
            MyAudioComponent comp;
            sys->entityManager->AddComponent<MyAudioComponent>(id, comp);
            MyAudioComponent& ref = sys->entityManager->GetComponent<MyAudioComponent>(id);
            MyAudioComponent** udata = (MyAudioComponent**)lua_newuserdata(L, sizeof(MyAudioComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "AudioMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Tag") {
            TagComponent comp;
            sys->entityManager->AddComponent<TagComponent>(id, comp);
            TagComponent& ref = sys->entityManager->GetComponent<TagComponent>(id);
            TagComponent** udata = (TagComponent**)lua_newuserdata(L, sizeof(TagComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "TagMetaTable");
            lua_setmetatable(L, -2);
            return 1;
        } else if (t == "Layer") {
            LayerComponent comp;
            sys->entityManager->AddComponent<LayerComponent>(id, comp);
            LayerComponent& ref = sys->entityManager->GetComponent<LayerComponent>(id);
            LayerComponent** udata = (LayerComponent**)lua_newuserdata(L, sizeof(LayerComponent*));
            *udata = &ref;
            luaL_getmetatable(L, "LayerMetaTable");
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

public:
    void initScript(LuaScriptComponent& script, uint32_t entityId) {
        script.L = luaL_newstate(); luaL_openlibs(script.L);
        if (!std::filesystem::exists(script.luaScriptPath)) { std::cerr << "Script introuvable: " << script.luaScriptPath << std::endl; return; }
        // store ScriptSystem* in the lua registry so C functions can access entityManager
        lua_pushlightuserdata(script.L, (void*)this); lua_setfield(script.L, LUA_REGISTRYINDEX, REGISTRY_SCRIPTSYS_KEY);

        // ensure metatables exist before running the script
        ensureComponentMetatables(script.L);

        if (luaL_dofile(script.L, script.luaScriptPath.c_str()) != LUA_OK) { std::cerr << "Erreur Lua : " << lua_tostring(script.L, -1) << std::endl; return; }

        // Build 'this' table for the script's entity
        pushEntityTable(script.L, this, entityId); lua_setglobal(script.L, "this");

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
    void bind(LuaScriptComponent& script, TransformComponent* t, CameraComponent* c, MaterialComponent* m) {
        lua_State* L = script.L;
        if (!L) return;
        if (t) { TransformComponent** udata = (TransformComponent**)lua_newuserdata(L, sizeof(TransformComponent*)); *udata = t; luaL_getmetatable(L, "TransformMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "transform"); }
        if (c) { CameraComponent** udata = (CameraComponent**)lua_newuserdata(L, sizeof(CameraComponent*)); *udata = c; luaL_getmetatable(L, "CameraMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "camera"); }
        if (m) { MaterialComponent** udata = (MaterialComponent**)lua_newuserdata(L, sizeof(MaterialComponent*)); *udata = m; luaL_getmetatable(L, "MaterialMetaTable"); lua_setmetatable(L, -2); lua_setglobal(L, "material"); }
    }
};