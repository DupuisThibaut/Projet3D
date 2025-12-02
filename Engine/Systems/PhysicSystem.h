#ifndef PHYSICSYSTEM_H
#define PHYSICSYSTEM_H
#include <unordered_map>

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
        ApplyForces(e);
        rigid.acceleration = rigid.forces * InvMass(e);
        rigid.velocity = rigid.velocity + dt * rigid.acceleration;
        rigid.velocity *= rigid.friction;
        transform.position = transform.position + rigid.velocity*dt;
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
    void update(float deltaTime) {
        accumulator += deltaTime;
        if (accumulator > MAX_ACCUMULATOR) {
            accumulator = MAX_ACCUMULATOR;
        }
        const float fixedDeltaTime = static_cast<float>(FIXED_DELTA_TIME);
        while(accumulator >= FIXED_DELTA_TIME){
            const auto& rigidBodies = entityManager->GetComponents<RigidBodyComponent>();
            const auto& colliders = entityManager->GetComponents<ColliderComponent>();
            if(rigidBodies.empty() && colliders.empty()) return;
            std::vector<std::pair<uint32_t, uint32_t>> collidingPairs;
            detectCollisions(collidingPairs);                
            for(const auto& pair : collidingPairs){
                if(entityManager->HasComponent<RigidBodyComponent>(pair.first)){
                    auto& rigidA = entityManager->GetComponent<RigidBodyComponent>(pair.first);
                    rigidA.gravity = glm::vec3(0.0f);
                }
                if(entityManager->HasComponent<RigidBodyComponent>(pair.second)){
                    auto& rigidB = entityManager->GetComponent<RigidBodyComponent>(pair.second);
                    rigidB.gravity = glm::vec3(0.0f);
                }
            }
        
            for(const auto& rb : rigidBodies){
                computeVelocity(rb.first, fixedDeltaTime);
            }
            accumulator -= FIXED_DELTA_TIME;
        }
    }
};

#endif // PHYSICSYSTEM_H