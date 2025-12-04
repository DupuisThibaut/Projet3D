#ifndef PHYSICSYSTEM_H
#define PHYSICSYSTEM_H
#include <unordered_map>
#include "../common/Geometry3D.h"
#include <iostream>

class PhysicSystem {
    const double FIXED_DELTA_TIME = 1.0/60.0;
    double accumulator = 0.0;
    const double MAX_ACCUMULATOR = 1.0/60.0;

public:
    EntityManager* entityManager;

    PhysicSystem(EntityManager* em) : entityManager(em) {}

    // Helpers
    void ApplyForces(uint32_t e){
        auto& rigid = entityManager->GetComponent<RigidBodyComponent>(e);
        rigid.forces = rigid.gravity * rigid.mass;
    }
    void AddLinearImpulse(uint32_t e, const glm::vec3& impulse){
        auto& rigid = entityManager->GetComponent<RigidBodyComponent>(e);
        if(rigid.mass <=0.0f) rigid.mass = 1.0f;
        rigid.velocity += impulse;
    }
    float InvMass(uint32_t e){
        auto& rigid = entityManager->GetComponent<RigidBodyComponent>(e);
        if(rigid.mass ==0.0f) return 0.0f;
        return 1.0f/rigid.mass;
    }


    // Calcul de la physique des objets (gravité, friction)
    void computeVelocity(uint32_t e, float dt){
        auto& rigid = entityManager->GetComponent<RigidBodyComponent>(e);
        auto& transform = entityManager->GetComponent<TransformComponent>(e);
        if(rigid.firstFrame){
            rigid.oldPosition = transform.position;
            rigid.firstFrame = false;
        }
        glm::vec3 velocity = transform.position - rigid.oldPosition;
        rigid.oldPosition = transform.position;
        rigid.acceleration = rigid.forces * InvMass(e);
        float deltaSquare = dt * dt;
        transform.position = transform.position + (velocity * rigid.friction + rigid.acceleration * deltaSquare);
        
        auto& mesh = entityManager->GetComponent<MeshComponent>(e);
        mesh.update=true;
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

    void detectCollisions(std::vector<std::pair<uint32_t, uint32_t>>& collidingPairs) {
        const auto& colliders = entityManager->GetComponents<ColliderComponent>();
        for (auto itA = colliders.begin(); itA != colliders.end(); itA++) {
            for (auto itB = std::next(itA); itB != colliders.end(); itB++) {
                uint32_t idA = itA->first;
                uint32_t idB = itB->first;
                auto& colliderA = entityManager->GetComponent<ColliderComponent>(idA);
                auto& colliderB = entityManager->GetComponent<ColliderComponent>(idB);
                if (!colliderA.collider || !colliderB.collider) continue;

                bool collision = false;

                // Cas spéciaux pour MESH (transforme les triangles du mesh en world space)
                if(colliderA.type == ColliderType::MESH || colliderB.type == ColliderType::MESH) {
                    uint32_t meshId = (colliderA.type == ColliderType::MESH) ? idA : idB;
                    uint32_t otherId = (colliderA.type == ColliderType::MESH) ? idB : idA;
                    auto& meshCollider = (colliderA.type == ColliderType::MESH) ? colliderA : colliderB;
                    auto& otherCollider = (colliderA.type == ColliderType::MESH) ? colliderB : colliderA;
                    
                    auto& meshTr = entityManager->GetComponent<TransformComponent>(meshId);
                    auto& otherTr = entityManager->GetComponent<TransformComponent>(otherId);
                    
                    auto* mesh = static_cast<MeshCollider*>(meshCollider.collider.get());
                    
                    // Transforme tous les triangles du mesh en world space
                    collision = false;
                    for(int i = 0; i < mesh->numTriangles && !collision; i++) {
                        Triangle worldTri;
                        worldTri.a = glm::vec3(meshTr.worldMatrix * glm::vec4(mesh->triangles[i].a, 1.0f));
                        worldTri.b = glm::vec3(meshTr.worldMatrix * glm::vec4(mesh->triangles[i].b, 1.0f));
                        worldTri.c = glm::vec3(meshTr.worldMatrix * glm::vec4(mesh->triangles[i].c, 1.0f));
                        
                        // Teste selon le type de l'autre collider
                        switch(otherCollider.type) {
                            case ColliderType::SPHERE: {
                                auto* sphere = static_cast<SphereCollider*>(otherCollider.collider.get());
                                collision = TriangleSphere(worldTri, *sphere);
                                break;
                            }
                            case ColliderType::AABB: {
                                auto* aabb = static_cast<AABBCollider*>(otherCollider.collider.get());
                                collision = TriangleAABB(worldTri, *aabb);
                                break;
                            }
                            case ColliderType::OBB: {
                                auto* obb = static_cast<OBBCollider*>(otherCollider.collider.get());
                                collision = TriangleOBB(worldTri, *obb);
                                break;
                            }
                            case ColliderType::PLANE: {
                                auto* plane = static_cast<PlaneCollider*>(otherCollider.collider.get());
                                collision = TrianglePlane(worldTri, *plane);
                                break;
                            }
                            case ColliderType::MESH: {
                                // Mesh vs Mesh : transforme aussi l'autre mesh
                                auto* otherMesh = static_cast<MeshCollider*>(otherCollider.collider.get());
                                for(int j = 0; j < otherMesh->numTriangles && !collision; j++) {
                                    Triangle otherWorldTri;
                                    otherWorldTri.a = glm::vec3(otherTr.worldMatrix * glm::vec4(otherMesh->triangles[j].a, 1.0f));
                                    otherWorldTri.b = glm::vec3(otherTr.worldMatrix * glm::vec4(otherMesh->triangles[j].b, 1.0f));
                                    otherWorldTri.c = glm::vec3(otherTr.worldMatrix * glm::vec4(otherMesh->triangles[j].c, 1.0f));
                                    collision = TriangleTriangle(worldTri, otherWorldTri);
                                }
                                break;
                            }
                            default: break;
                        }
                    }
                } 
                else {
                    // Collision normale (pas de mesh, utilise directement les colliders en world space)
                    switch (colliderA.type) {
                        case ColliderType::SPHERE:
                            switch (colliderB.type) {
                                case ColliderType::SPHERE:
                                    collision = SphereSphere(
                                        *static_cast<SphereCollider*>(colliderA.collider.get()),
                                        *static_cast<SphereCollider*>(colliderB.collider.get()));
                                    break;
                                case ColliderType::AABB:
                                    collision = SphereAABB(
                                        *static_cast<SphereCollider*>(colliderA.collider.get()),
                                        *static_cast<AABBCollider*>(colliderB.collider.get()));
                                    break;
                                case ColliderType::OBB:
                                    collision = SphereOBB(
                                        *static_cast<SphereCollider*>(colliderA.collider.get()),
                                        *static_cast<OBBCollider*>(colliderB.collider.get()));
                                    break;
                                case ColliderType::PLANE:
                                    collision = SpherePlane(
                                        *static_cast<SphereCollider*>(colliderA.collider.get()),
                                        *static_cast<PlaneCollider*>(colliderB.collider.get()));
                                    break;
                                default: break;
                            }
                            break;
                        case ColliderType::AABB:
                            switch (colliderB.type) {
                                case ColliderType::SPHERE:
                                    collision = SphereAABB(
                                        *static_cast<SphereCollider*>(colliderB.collider.get()),
                                        *static_cast<AABBCollider*>(colliderA.collider.get()));
                                    break;
                                case ColliderType::AABB:
                                    collision = AABBAABB(
                                        *static_cast<AABBCollider*>(colliderA.collider.get()),
                                        *static_cast<AABBCollider*>(colliderB.collider.get()));
                                    break;
                                case ColliderType::OBB:
                                    collision = AABBOBB(
                                        *static_cast<AABBCollider*>(colliderA.collider.get()),
                                        *static_cast<OBBCollider*>(colliderB.collider.get()));
                                    break;
                                case ColliderType::PLANE:
                                    collision = AABBPlane(
                                        *static_cast<AABBCollider*>(colliderA.collider.get()),
                                        *static_cast<PlaneCollider*>(colliderB.collider.get()));
                                    break;
                                default: break;
                            }
                            break;
                        case ColliderType::OBB:
                            switch (colliderB.type) {
                                case ColliderType::SPHERE:
                                    collision = SphereOBB(
                                        *static_cast<SphereCollider*>(colliderB.collider.get()),
                                        *static_cast<OBBCollider*>(colliderA.collider.get()));
                                    break;
                                case ColliderType::AABB:
                                    collision = AABBOBB(
                                        *static_cast<AABBCollider*>(colliderB.collider.get()),
                                        *static_cast<OBBCollider*>(colliderA.collider.get()));
                                    break;
                                case ColliderType::OBB:
                                    collision = OBBOBB(
                                        *static_cast<OBBCollider*>(colliderA.collider.get()),
                                        *static_cast<OBBCollider*>(colliderB.collider.get()));
                                    break;
                                case ColliderType::PLANE:
                                    collision = OBBPlane(
                                        *static_cast<OBBCollider*>(colliderA.collider.get()),
                                        *static_cast<PlaneCollider*>(colliderB.collider.get()));
                                    break;
                                default: break;
                            }
                            break;
                        case ColliderType::PLANE:
                            switch (colliderB.type) {
                                case ColliderType::SPHERE:
                                    collision = SpherePlane(
                                        *static_cast<SphereCollider*>(colliderB.collider.get()),
                                        *static_cast<PlaneCollider*>(colliderA.collider.get()));
                                    break;
                                case ColliderType::AABB:
                                    collision = AABBPlane(
                                        *static_cast<AABBCollider*>(colliderB.collider.get()),
                                        *static_cast<PlaneCollider*>(colliderA.collider.get()));
                                    break;
                                case ColliderType::OBB:
                                    collision = OBBPlane(
                                        *static_cast<OBBCollider*>(colliderB.collider.get()),
                                        *static_cast<PlaneCollider*>(colliderA.collider.get()));
                                    break;
                                default: break;
                            }
                            break;
                        default: break;
                    }
                }
    
                if (collision) {
                    collidingPairs.push_back({idA, idB});
                }
            }
        }
    }
    
    void SolveConstraints(uint32_t e, const WorldConstraints& wc){
        int size = wc.spheres.size() + wc.aabbs.size() + wc.obbs.size() + wc.planes.size() + wc.meshes.size();
        auto& rb = entityManager->GetComponent<RigidBodyComponent>(e);
        auto& transform = entityManager->GetComponent<TransformComponent>(e);
        for(int i=0; i<size; i++){
            if(i < wc.spheres.size()){
                // Sphere
                SphereCollider sphere = wc.spheres[i];
                LineCollider traveled(rb.oldPosition, transform.position);
                if(Linetest(sphere, traveled)){
                    rb.velocity = transform.position - rb.oldPosition;
                    glm::vec3 direction = glm::normalize(rb.velocity);
                    RayCollider ray(rb.oldPosition - direction * 0.01f, direction);
                    RaycastResult result;
                    if(Raycast(sphere,ray,&result)){
                        transform.position = result.point + result.normal * 0.001f;
                        glm::vec3 velNormal = glm::dot(rb.velocity, result.normal) * result.normal;
                        glm::vec3 velTangent = rb.velocity - velNormal;
                        rb.oldPosition = transform.position - (velTangent - velNormal * rb.bounce);
                        break;
                    }
                }
            } else if(i < wc.spheres.size() + wc.aabbs.size()){
                AABBCollider aabb = wc.aabbs[i - wc.spheres.size()];
                LineCollider traveled(rb.oldPosition, transform.position);
                if(Linetest(aabb, traveled)){
                    rb.velocity = transform.position - rb.oldPosition;
                    glm::vec3 direction = glm::normalize(rb.velocity);
                    RayCollider ray(rb.oldPosition - direction * 0.01f, direction);
                    RaycastResult result;
                    if(Raycast(aabb,ray,&result)){
                        transform.position = result.point + result.normal * 0.001f;
                        glm::vec3 velNormal = glm::dot(rb.velocity, result.normal) * result.normal;
                        glm::vec3 velTangent = rb.velocity - velNormal;
                        rb.oldPosition = transform.position - (velTangent - velNormal * rb.bounce);
                        break;
                    }
                }
            } else if(i < wc.spheres.size() + wc.aabbs.size() + wc.obbs.size()){
                OBBCollider obb = wc.obbs[i - wc.spheres.size() - wc.aabbs.size()];
                LineCollider traveled(rb.oldPosition, transform.position);
                if(Linetest(obb, traveled)){
                    rb.velocity = transform.position - rb.oldPosition;
                    glm::vec3 direction = glm::normalize(rb.velocity);
                    RayCollider ray(rb.oldPosition - direction * 0.01f, direction);
                    RaycastResult result;
                    if(Raycast(obb,ray,&result)){
                        transform.position = result.point + result.normal * 0.001f;
                        glm::vec3 velNormal = glm::dot(rb.velocity, result.normal) * result.normal;
                        glm::vec3 velTangent = rb.velocity - velNormal;
                        rb.oldPosition = transform.position - (velTangent - velNormal * rb.bounce);
                        break;
                    }
                }
            } else if(i < wc.spheres.size() + wc.aabbs.size() + wc.obbs.size() + wc.planes.size()){
                PlaneCollider plane = wc.planes[i - wc.spheres.size() - wc.aabbs.size() - wc.obbs.size()];
                auto& entityCollider = entityManager->GetComponent<ColliderComponent>(e);
                float entityRadius = 0.0f;
                if(entityCollider.type == ColliderType::SPHERE) {
                    auto* sphere = static_cast<SphereCollider*>(entityCollider.collider.get());
                    entityRadius = sphere->radius;
                }
                PlaneCollider adjustedPlane = plane;
                adjustedPlane.distance += entityRadius;
                LineCollider traveled(rb.oldPosition, transform.position);
                if(Linetest(adjustedPlane, traveled)){
                    rb.velocity = transform.position - rb.oldPosition;
                    glm::vec3 direction = glm::normalize(rb.velocity);
                    RayCollider ray(rb.oldPosition - direction * 0.01f, direction);
                    RaycastResult result;
                    if(Raycast(plane, ray, &result)){
                        transform.position = result.point + result.normal * (entityRadius + 0.001f);
                        glm::vec3 velNormal = glm::dot(rb.velocity, result.normal) * result.normal;
                        glm::vec3 velTangent = rb.velocity - velNormal;
                        rb.oldPosition = transform.position - (velTangent - velNormal * rb.bounce);
                        break;
                    }
                }
            } else {
                MeshCollider* mesh = wc.meshes[i - wc.spheres.size() - wc.aabbs.size() - wc.obbs.size() - wc.planes.size()];
                LineCollider traveled(rb.oldPosition, transform.position);
                if(Linetest(*mesh, traveled)){
                    rb.velocity = transform.position - rb.oldPosition;
                    glm::vec3 direction = glm::normalize(rb.velocity);
                    RayCollider ray(rb.oldPosition - direction * 0.01f, direction);
                    RaycastResult result;
                    if(Raycast(*mesh,ray,&result)){
                        transform.position = result.point + result.normal * 0.001f;
                        glm::vec3 velNormal = glm::dot(rb.velocity, result.normal) * result.normal;
                        glm::vec3 velTangent = rb.velocity - velNormal;
                        rb.oldPosition = transform.position - (velTangent - velNormal * rb.bounce);
                        break;
                    }
                }
            }
        }
    }
    
    void update(float deltaTime) {
        accumulator += deltaTime;
        if (accumulator > MAX_ACCUMULATOR) {
            accumulator = MAX_ACCUMULATOR;
        }
        const float fixedDeltaTime = static_cast<float>(FIXED_DELTA_TIME);
        while(accumulator >= FIXED_DELTA_TIME){
            const auto& rigidBodies = entityManager->GetComponents<RigidBodyComponent>();
            if(rigidBodies.empty()){
                accumulator -= FIXED_DELTA_TIME;
                continue;
            }
            // Appliquer les forces avant de calculer la vélocité
            for(const auto& rb : rigidBodies){
                ApplyForces(rb.first);
            }
            // Calcul de la vélocité
            for(const auto& rb : rigidBodies){
                computeVelocity(rb.first, fixedDeltaTime);
            }
            WorldConstraints wc = getWorldConstraints();
            for(const auto& rb : rigidBodies){
                SolveConstraints(rb.first, wc);
            } 
            accumulator -= FIXED_DELTA_TIME;
        }
    }
};

#endif // PHYSICSYSTEM_H