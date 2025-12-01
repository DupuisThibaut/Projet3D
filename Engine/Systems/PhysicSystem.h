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
        //std::cout<<"Detecting collisions among "<<colliders.size()<<" colliders."<<std::endl;
        for (auto itA = colliders.begin(); itA != colliders.end(); itA++) {
            for (auto itB = std::next(itA); itB != colliders.end(); itB++) {
                //std::cout<<" Checking collider of entity "<<itA->first<<std::endl;
                uint32_t idA = itA->first;
                uint32_t idB = itB->first;
                auto& colliderA = entityManager->GetComponent<ColliderComponent>(idA);
                auto& colliderB = entityManager->GetComponent<ColliderComponent>(idB);
                if (!colliderA.collider) {
                    std::cerr << "Null collider pointer for entities " << idA << std::endl;
                    continue;
                } else if (!colliderB.collider) {
                    std::cerr << "Null collider pointer for entities " << idB << std::endl;
                    continue;
                }
                // Appelle la bonne fonction selon les types
                bool collision = false;
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
                            case ColliderType::MESH:
                                collision = SphereMesh(
                                    *static_cast<SphereCollider*>(colliderA.collider.get()),
                                    *static_cast<MeshCollider*>(colliderB.collider.get()));
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
                            case ColliderType::MESH:
                                collision = MeshAABB(
                                    *static_cast<MeshCollider*>(colliderB.collider.get()),
                                    *static_cast<AABBCollider*>(colliderA.collider.get()));
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
                            case ColliderType::MESH:
                                collision = MeshOBB(
                                    *static_cast<MeshCollider*>(colliderB.collider.get()),
                                    *static_cast<OBBCollider*>(colliderA.collider.get()));
                                break;
                            default: break;
                        }
                        break;
                    case ColliderType::MESH: 
                        switch (colliderB.type) {
                            case ColliderType::SPHERE:
                                collision = SphereMesh(
                                    *static_cast<SphereCollider*>(colliderB.collider.get()),
                                    *static_cast<MeshCollider*>(colliderA.collider.get()));
                                break;
                            case ColliderType::AABB:
                                collision = MeshAABB(
                                    *static_cast<MeshCollider*>(colliderA.collider.get()),
                                    *static_cast<AABBCollider*>(colliderB.collider.get()));
                                break;
                            case ColliderType::OBB:
                                collision = MeshOBB(
                                    *static_cast<MeshCollider*>(colliderA.collider.get()),
                                    *static_cast<OBBCollider*>(colliderB.collider.get()));
                                break;
                            case ColliderType::PLANE:
                                collision = MeshPlane(
                                    *static_cast<MeshCollider*>(colliderA.collider.get()),
                                    *static_cast<PlaneCollider*>(colliderB.collider.get()));
                                break;
                            case ColliderType::MESH:
                                collision = MeshMesh(
                                    *static_cast<MeshCollider*>(colliderA.collider.get()),
                                    *static_cast<MeshCollider*>(colliderB.collider.get()));
                                break;
                            default: break;
                        }
                        break;
                    default: break;
                }
    
                if (collision) {
                    std::cout<<"Collision detected between entities "<<idA<<" and "<<idB<<std::endl;
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