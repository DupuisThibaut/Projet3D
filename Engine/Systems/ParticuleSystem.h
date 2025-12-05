#ifndef PARTICULESYSTEM_H
#define PARTICULESYSTEM_H
#include <glm/fwd.hpp>
#include <unordered_map>
#include <cfloat>
#include <vector>

#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <string_view>

#include "../Components/ParticuleComponent.h"
#include "../common/Geometry3D.h"

class ParticuleSystem {
public:
    EntityManager* entityManager;

    ParticuleSystem(EntityManager* em) : entityManager(em) {
        initialize();
    }

    void computeVelocity(ParticuleComponent* p, int i, float dt){
        if(p->firstFrame[i]){
            p->posAvant[i] = p->pos[i];
            p->firstFrame[i] = false;
        }
        glm::vec3 velocity = p->pos[i] - p->posAvant[i];
        p->posAvant[i] = p->pos[i];
        p->speed[i] = glm::vec3(0.0,-9.81,0.0);
        float deltaSquare = dt * dt;
        p->pos[i] = p->pos[i] + (velocity + glm::vec3(0.0,-9.81,0.0) * deltaSquare);
    }

    struct WorldConstraints{
        std::vector<SphereCollider> spheres;
        std::vector<AABBCollider> aabbs;
        std::vector<OBBCollider> obbs;
        std::vector<PlaneCollider> planes;
        std::vector<MeshCollider*> meshes;
        std::vector<glm::mat4> meshTransforms;
    };

    WorldConstraints getWorldConstraints(){
        WorldConstraints wc;
        const auto& colliders = entityManager->GetComponents<ColliderComponent>();
        for(const auto& pair : colliders){
            if(entityManager->HasComponent<RigidBodyComponent>(pair.first)) continue;
            
            uint32_t id = pair.first;
            auto& colliderComp = entityManager->GetComponent<ColliderComponent>(id);
            auto& transform = entityManager->GetComponent<TransformComponent>(id);
            glm::mat4 worldMatrix = transform.worldMatrix;
            
            switch(colliderComp.type){
                case ColliderType::SPHERE: {
                    auto* sphere = static_cast<SphereCollider*>(colliderComp.collider.get());
                    SphereCollider worldSphere = *sphere;
                    worldSphere.position = glm::vec3(transform.position);
                    worldSphere.radius = sphere->radius * std::max({transform.scale.x, transform.scale.y, transform.scale.z});
                    wc.spheres.push_back(worldSphere);
                    break;
                }
                case ColliderType::AABB: {
                    auto* aabb = static_cast<AABBCollider*>(colliderComp.collider.get());
                    AABBCollider worldAABB = *aabb;
                    glm::vec3 min = GetMin(*aabb);
                    glm::vec3 max = GetMax(*aabb);
                    glm::vec3 worldMin = glm::vec3(transform.position + min);
                    glm::vec3 worldMax = glm::vec3(transform.position + max);
                    worldAABB.origin = worldMin;
                    worldAABB.size = worldMax - worldMin;
                    wc.aabbs.push_back(worldAABB);
                    break;
                }
                case ColliderType::OBB: {
                    auto* obb = static_cast<OBBCollider*>(colliderComp.collider.get());
                    OBBCollider worldOBB = *obb;
                    worldOBB.position = glm::vec3(worldMatrix * glm::vec4(obb->position, 1.0f));
                    worldOBB.orientation = glm::mat3(worldMatrix) * obb->orientation;
                    wc.obbs.push_back(worldOBB);
                    break;
                }
                case ColliderType::PLANE: {
                    auto* plane = static_cast<PlaneCollider*>(colliderComp.collider.get());
                    PlaneCollider worldPlane = *plane;
                    worldPlane.normal = plane->normal;
                    worldPlane.distance = plane->distance;                    
                    wc.planes.push_back(worldPlane);
                    break;
                }
                case ColliderType::MESH: {
                    auto* mesh = static_cast<MeshCollider*>(colliderComp.collider.get());
                    wc.meshes.push_back(mesh);
                    wc.meshTransforms.push_back(worldMatrix);
                    break;
                }
                default: break;
            }
        }
        return wc;
    }

    void SolveConstraints(const WorldConstraints& wc, ParticuleComponent* p){
        int size = wc.spheres.size() + wc.aabbs.size() + wc.obbs.size() + wc.planes.size() + wc.meshes.size();
        for(int par=0;par<p->nb;par++){
            for(int i=0; i<size; i++){
                if(i < wc.spheres.size()){
                    // Sphere
                    SphereCollider sphere = wc.spheres[i];
                    LineCollider traveled(p->posAvant[par], p->pos[par]);
                    if(Linetest(sphere, traveled)){
                        p->velocity[par] = p->pos[par] - p->posAvant[par];
                        glm::vec3 direction = glm::normalize(p->velocity[par]);
                        RayCollider ray(p->posAvant[par] - direction * 0.01f, direction);
                        RaycastResult result;
                        if(Raycast(sphere,ray,&result)){
                            p->pos[par] = result.point + result.normal * 0.001f;
                            glm::vec3 velNormal = glm::dot(p->velocity[par], result.normal) * result.normal;
                            glm::vec3 velTangent = p->velocity[par] - velNormal;
                            p->posAvant[par] = p->pos[par] - (velTangent - velNormal * 0.2f);
                            break;
                        }
                    }
                } else if(i < wc.spheres.size() + wc.aabbs.size()){
                    AABBCollider aabb = wc.aabbs[i - wc.spheres.size()];
                    LineCollider traveled(p->posAvant[par], p->pos[par]);
                    if(Linetest(aabb, traveled)){
                        p->velocity[par] = p->pos[par] - p->posAvant[par];
                        glm::vec3 direction = glm::normalize(p->velocity[par]);
                        RayCollider ray(p->posAvant[par] - direction * 0.01f, direction);
                        RaycastResult result;
                        if(Raycast(aabb,ray,&result)){
                            p->pos[par] = result.point + result.normal * 0.001f;
                            glm::vec3 velNormal = glm::dot(p->velocity[par], result.normal) * result.normal;
                            glm::vec3 velTangent = p->velocity[par] - velNormal;
                            p->posAvant[par] = p->pos[par] - (velTangent - velNormal * 0.2f);
                            break;
                        }
                    }
                } else if(i < wc.spheres.size() + wc.aabbs.size() + wc.obbs.size()){
                    OBBCollider obb = wc.obbs[i - wc.spheres.size() - wc.aabbs.size()];
                    LineCollider traveled(p->posAvant[par], p->pos[par]);
                    if(Linetest(obb, traveled)){
                        p->velocity[par] = p->pos[par] - p->posAvant[par];
                        glm::vec3 direction = glm::normalize(p->velocity[par]);
                        RayCollider ray(p->posAvant[par] - direction * 0.01f, direction);
                        RaycastResult result;
                        if(Raycast(obb,ray,&result)){
                            p->pos[par] = result.point + result.normal * 0.001f;
                            glm::vec3 velNormal = glm::dot(p->velocity[par], result.normal) * result.normal;
                            glm::vec3 velTangent = p->velocity[par] - velNormal;
                            p->posAvant[par] = p->pos[par] - (velTangent - velNormal * 0.2f);
                            break;
                        }
                    }
                } else if(i < wc.spheres.size() + wc.aabbs.size() + wc.obbs.size() + wc.planes.size()){
                    PlaneCollider plane = wc.planes[i - wc.spheres.size() - wc.aabbs.size() - wc.obbs.size()];
                    float entityRadius = p->rayon[par];
                    PlaneCollider adjustedPlane = plane;
                    adjustedPlane.distance += entityRadius;
                    LineCollider traveled(p->posAvant[par], p->pos[par]);
                    if(Linetest(adjustedPlane, traveled)){
                        p->velocity[par] = p->pos[par] - p->posAvant[par];
                        glm::vec3 direction = glm::normalize(p->velocity[par]);
                        RayCollider ray(p->posAvant[par] - direction * 0.01f, direction);
                        RaycastResult result;
                        if(Raycast(plane, ray, &result)){
                            p->pos[par] = result.point + result.normal * (entityRadius + 0.001f);
                            glm::vec3 velNormal = glm::dot(p->velocity[par], result.normal) * result.normal;
                            glm::vec3 velTangent = p->velocity[par] - velNormal;
                            p->posAvant[par] = p->pos[par] - (velTangent - velNormal * 0.2f);
                            break;
                        }
                    }
                } else {
                    MeshCollider* mesh = wc.meshes[i - wc.spheres.size() - wc.aabbs.size() - wc.obbs.size() - wc.planes.size()];
                    LineCollider traveled(p->posAvant[par], p->pos[par]);
                    if(Linetest(*mesh, traveled)){
                        p->velocity[par] = p->pos[par] - p->posAvant[par];
                        glm::vec3 direction = glm::normalize(p->velocity[par]);
                        RayCollider ray(p->posAvant[par] - direction * 0.01f, direction);
                        RaycastResult result;
                        if(Raycast(*mesh,ray,&result)){
                            p->pos[par] = result.point + result.normal * 0.001f;
                            glm::vec3 velNormal = glm::dot(p->velocity[par], result.normal) * result.normal;
                            glm::vec3 velTangent = p->velocity[par] - velNormal;
                            p->posAvant[par] = p->pos[par] - (velTangent - velNormal * 0.2f);
                            break;
                        }
                    }
                }
            }
        }
    }

    void update(float t){
        for (auto& [id, p] : entityManager->GetComponents<ParticuleComponent>()) {
            // p.update(t);
            auto& particule=entityManager->GetComponent<ParticuleComponent>(id);
            // std::cout<<"particule x : "<<particule.speed[0].x<<" particule y : "<<particule.speed[0].y<<" particule z : "<<particule.speed[0].z<<std::endl;
            // std::cout<<"time : "<<t<<std::endl;
            for(int i=0;i<particule.nb;i++){
                // std::cout<<"le temps : "<<t<<std::endl;
                // particule.speed[i][1] -= t*9.81f;
                // particule.pos[i] += 0.1f*particule.speed[i];
                // // particule.pos[i] += particule.speed[i];

                // if (particule.pos[i][1] < 0.0) {
                //     particule.speed[i][1] = -0.8 * particule.speed[i][1];
                //     particule.pos[i][1] = 0.0;
                // }

                computeVelocity(&particule,i,t);

                particule.age[i] += 1.0f;
                // age+=t/15.0f;
                // std::cout<<"age : "<<age<<std::endl;
                if(particule.age[i] >= particule.ageMax[i]) init(i,&particule);
            }
            WorldConstraints wc = getWorldConstraints();
            SolveConstraints(wc,&particule);
            // std::cout<<"particule x : "<<particule.pos[0].x<<" particule y : "<<particule.pos[0].y<<" particule z : "<<particule.pos[0].z<<std::endl;

        }
    }

    void init(int i, ParticuleComponent* p){
        p->pos[i] = p->position;
        p->pos[i]+=glm::vec3(8.0f*rand() / static_cast<float>(RAND_MAX)-4.0f,rand() / static_cast<float>(RAND_MAX)-0.5f,8.0f*rand() / static_cast<float>(RAND_MAX)-4.0f);
        float angle = 2.0 * M_PI * rand() / RAND_MAX;
        float norm = 0.04 * rand() / RAND_MAX;
        p->speed[i] = glm::vec3(norm * cos(angle), norm * sin(angle),
                    rand() / static_cast<float>(RAND_MAX));
        p->age[i] = 0.0f;
        p->ageMax[i] = 125.0f + (100.0f * rand() / float(RAND_MAX));
        p->rayon[i]=rand()/float(RAND_MAX)*0.05f;
        p->firstFrame[i]=true;
    }

    void initialize(){
        for (auto& [id, p] : entityManager->GetComponents<ParticuleComponent>()) {
            // p.update(t);
            auto& particule=entityManager->GetComponent<ParticuleComponent>(id);
            particule.pos.resize(particule.nb);
            particule.speed.resize(particule.nb);
            particule.age.resize(particule.nb);
            particule.ageMax.resize(particule.nb);
            particule.rayon.resize(particule.nb);
            particule.firstFrame.resize(particule.nb);
            particule.posAvant.resize(particule.nb);
            particule.velocity.resize(particule.nb);
            for(int i=0;i<particule.nb;i++){
                init(i,&particule);
            }
        }
    }
};

#endif // PARTICULESYSTEM_H