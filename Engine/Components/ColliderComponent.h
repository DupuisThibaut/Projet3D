#ifndef COLLIDER_COMPONENT_H
#define COLLIDER_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "Systems/EntityManager.h"
#include <glm/gtx/euler_angles.hpp>
#include "common/Geometry3D.h"

enum class ColliderType {
    AABB,
    OBB,
    SPHERE,
    PLANE,
    MESH
};


struct ColliderComponent {
    ColliderType type = ColliderType::SPHERE;
    bool isTrigger = false;
    std::unique_ptr<BaseCollider> collider;

    ColliderComponent() = default;

    // Supprimer le copy ctor/assignment pour éviter la perte du unique_ptr
    ColliderComponent(const ColliderComponent& other) = delete;
    ColliderComponent& operator=(const ColliderComponent& other) = delete;

    // Autoriser le move (transfert du unique_ptr)
    ColliderComponent(ColliderComponent&& other) noexcept
        : type(other.type), isTrigger(other.isTrigger), collider(std::move(other.collider)) {}
    ColliderComponent& operator=(ColliderComponent&& other) noexcept {
        if (this != &other) {
            type = other.type;
            isTrigger = other.isTrigger;
            collider = std::move(other.collider);
        }
        return *this;
    }

    void loadFromFile(const nlohmann::json& entityData, uint32_t& index, EntityManager *entityManager) {
        std::string colliderType = entityData["collider"]["type"];
        if(entityData["collider"].contains("isTrigger")) isTrigger = entityData["collider"]["isTrigger"];
        if(colliderType == "SPHERE"){
            type = ColliderType::SPHERE;
            glm::vec3 center = entityManager->GetComponent<TransformComponent>(index).position;
            float radius = entityData["collider"]["radius"];
            collider = std::make_unique<SphereCollider>(center, radius);
        }
        else if(colliderType == "AABB"){
            type = ColliderType::AABB;
            MeshComponent mesh = entityManager->GetComponent<MeshComponent>(index);
            TransformComponent t = entityManager->GetComponent<TransformComponent>(index);
            std::cout<<"Loading AABB collider for entity "<<index<<" with "<<mesh.vertices.size()<<std::endl;
            if (mesh.vertices.empty()) {
                std::cerr << "Mesh vertices empty for entity " << index << std::endl;
                return;
            }
            glm::vec3 min = mesh.vertices[0]*t.scale;
            glm::vec3 max = mesh.vertices[0]*t.scale;
            for(int i =1; i < mesh.vertices.size(); i++){
                min.x = std::min(min.x, mesh.vertices[i].x*t.scale.x);
                min.y =  std::min(min.y, mesh.vertices[i].y*t.scale.y);
                min.z =  std::min(min.z, mesh.vertices[i].z*t.scale.z);
                max.x =  std::max(max.x, mesh.vertices[i].x*t.scale.x);
                max.y =  std::max(max.y, mesh.vertices[i].y*t.scale.y);
                max.z =  std::max(max.z, mesh.vertices[i].z*t.scale.z);
            }
            collider = std::make_unique<AABBCollider>( (min + max) * 0.5f, (max - min) * 0.5f);
            std::cout<<"min: "<<min.x<<","<<min.y<<","<<min.z<<std::endl;
            std::cout<<"max: "<<max.x<<","<<max.y<<","<<max.z<<std::endl;
        }
        else if(colliderType == "OBB"){
            type = ColliderType::OBB;
            MeshComponent mesh = entityManager->GetComponent<MeshComponent>(index);
            if (mesh.vertices.empty()) {
                std::cerr << "Mesh vertices empty for entity " << index << std::endl;
                return;
            }
            glm::vec3 min = mesh.vertices[0];
            glm::vec3 max = mesh.vertices[0];
            for(int i =1; i < mesh.vertices.size(); i++){
                min.x = std::min(min.x, mesh.vertices[i].x);
                min.y =  std::min(min.y, mesh.vertices[i].y);
                min.z =  std::min(min.z, mesh.vertices[i].z);
                max.x =  std::max(max.x, mesh.vertices[i].x);
                max.y =  std::max(max.y, mesh.vertices[i].y);
                max.z =  std::max(max.z, mesh.vertices[i].z);
            }
            glm::vec3 eulerAngles = entityManager->GetComponent<TransformComponent>(index).rotation;
            glm::mat4 rotMat4 = glm::eulerAngleYXZ(eulerAngles.y, eulerAngles.x, eulerAngles.z);
            glm::mat3 rotation = glm::mat3(rotMat4);
            collider = std::make_unique<OBBCollider>( (min + max) * 0.5f, (max - min) * 0.5f, rotation);
        }
        else if(colliderType == "PLANE"){
            type = ColliderType::PLANE;
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            if (entityData["collider"].contains("normal")) {
                normal = glm::vec3(entityData["collider"]["normal"][0],
                                   entityData["collider"]["normal"][1],
                                   entityData["collider"]["normal"][2]);
            }
            glm::vec3 position = entityManager->GetComponent<TransformComponent>(index).position;
            float distance = glm::dot(position, normal);
            collider = std::make_unique<PlaneCollider>(normal, distance);
        }
        else if(colliderType == "MESH"){
            type = ColliderType::MESH;
            MeshComponent meshComp = entityManager->GetComponent<MeshComponent>(index);
            if (meshComp.vertices.empty()) {
                std::cerr << "[DEBUG] Mesh vertices empty for entity " << index << std::endl;
                return;
            }
            MeshCollider mesh;
            size_t realNumTriangles = 0;
            size_t triSet = 0;
            for (const auto& tri : meshComp.triangles) {
                if (tri.size() < 3) {
                    std::cerr << "[DEBUG] Triangle set " << triSet << " too small (" << tri.size() << " vertices)" << std::endl;
                    triSet++;
                    continue;
                }
                realNumTriangles += tri.size() - 2;
                triSet++;
            }
            mesh.numTriangles = realNumTriangles;
            mesh.triangles = new Triangle[mesh.numTriangles];
            size_t triIdx = 0;
            triSet = 0;
            for (const auto& tri : meshComp.triangles) {
                if (tri.size() < 3) {
                    triSet++;
                    continue;
                }
                for (size_t j = 0; j < tri.size() - 2; ++j) {
                    if (tri[0] >= meshComp.vertices.size() ||
                        tri[j + 1] >= meshComp.vertices.size() ||
                        tri[j + 2] >= meshComp.vertices.size()) {
                        std::cerr << "[DEBUG] Triangle index out of bounds for entity " << index
                                << " (triSet=" << triSet
                                << ", tri[0]=" << tri[0]
                                << ", tri[j+1]=" << tri[j+1]
                                << ", tri[j+2]=" << tri[j+2]
                                << ", vertices=" << meshComp.vertices.size() << ")" << std::endl;
                        continue;
                    }
                    mesh.triangles[triIdx].a = meshComp.vertices[tri[0]];
                    mesh.triangles[triIdx].b = meshComp.vertices[tri[j + 1]];
                    mesh.triangles[triIdx].c = meshComp.vertices[tri[j + 2]];
                    triIdx++;
                }
                triSet++;
            }
            mesh.vertices = meshComp.vertices.data();
            mesh.values = nullptr;
            AccelerateMesh(mesh);
            collider = std::make_unique<MeshCollider>(mesh);
        }
        if (!collider) {
            std::cerr << "Collider not created for entity " << index << " (" << colliderType << ")" << std::endl;
        }
    }


    ~ColliderComponent() = default;
};

#endif // COLLIDER_COMPONENT_H
