#ifndef ENTITYMANAGER_H
#define ENTITYMANAGER_H

#include <cstdint>
#include <queue>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_set>
#include "../Entities/Entity.h"

using EntityID = uint32_t;
static constexpr EntityID INVALID_ENTITY_ID = UINT32_MAX;

class EntityManager {
private:
    EntityID nextEntityId{0};
    std::queue<EntityID> freeIds;
    std::unordered_set<EntityID> usedIds;

    struct StoreBase {
        virtual ~StoreBase() = default;
        virtual void erase(EntityID e) = 0;
    };
    template<typename T> 
    struct Store : StoreBase {
        std::unordered_map<EntityID, T> data;
        void erase(EntityID e) override {
            data.erase(e);
        }
    };

    std::unordered_map<std::type_index, std::unique_ptr<StoreBase>> stores;

    template<typename T>
    std::unordered_map<EntityID, T>& getStore() {
        std::type_index idx(typeid(T));
        auto it = stores.find(idx);
        if (it == stores.end()) {
            stores[idx] = std::make_unique<Store<T>>();
            it = stores.find(idx);
        }
        return static_cast<Store<T>*>(it->second.get())->data;
    }

public:
    EntityID CreateEntity(){
        if(!freeIds.empty()){ EntityID id = freeIds.front(); freeIds.pop(); usedIds.insert(id); return id;}
        EntityID id = nextEntityId;
        usedIds.insert(id);
        nextEntityId++;
        return id;
    }

    bool CreateEntity(EntityID id) {
        if (usedIds.count(id)) return false;
        usedIds.insert(id);
        if (id >= nextEntityId) nextEntityId = id + 1;
        return true;
    }

    void DestroyEntity(EntityID e){
        for (auto &kv : stores) kv.second->erase(e);
        usedIds.erase(e);
        freeIds.push(e);
    }

    template<typename T>
    void AddComponent(EntityID e, T component) {
        getStore<T>()[e] = component;
    }
    template<typename T>
    void RemoveComponent(EntityID e) {
        auto it = stores.find(std::type_index(typeid(T)));
        if (it!=stores.end()) static_cast<Store<T>*>(it->second.get())->data.erase(e);
    }

    template<typename T>
    bool HasComponent(EntityID e) const {
        auto it = stores.find(std::type_index(typeid(T)));
        if (it==stores.end()) return false;
        return static_cast<Store<T>*>(it->second.get())->data.count(e) != 0;
    }

    template<typename T>
    T& GetComponent(EntityID e) {
        return static_cast<Store<T>*>(stores.at(std::type_index(typeid(T))).get())->data.at(e);
    }

    template<typename T>
    const std::unordered_map<EntityID, T>& GetComponents() {
        if(stores.find(std::type_index(typeid(T))) == stores.end()) {
            stores[std::type_index(typeid(T))] = std::make_unique<Store<T>>();
        }
        return static_cast<Store<T>*>(stores.at(std::type_index(typeid(T))).get())->data;
    }

    template<typename T, typename F>
    void ForEach(F f) {
        auto it = stores.find(std::type_index(typeid(T)));
        if (it==stores.end()) return;
        for (auto &kv : static_cast<Store<T>*>(it->second.get())->data) f(kv.first, kv.second);
    }

    template<typename T1, typename T2, typename F>
    void forEach(F f) {
        auto it1 = stores.find(std::type_index(typeid(T1)));
        auto it2 = stores.find(std::type_index(typeid(T2)));
        if (it1==stores.end() || it2==stores.end()) return;
        auto &m1 = static_cast<Store<T1>*>(it1->second.get())->data;
        auto &m2 = static_cast<Store<T2>*>(it2->second.get())->data;
        for (auto &kv : m1) {
            auto it = m2.find(kv.first);
            if (it != m2.end()) f(kv.first, kv.second, it->second);
        }
    }

};

#endif // ENTITYMANAGER_H