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

    bool isColliding(uint32_t e1){
        std::cout<<"Checking collision for entity "<<e1<<std::endl;
        auto& colliders = entityManager->GetComponents<ColliderComponent>();
        if(entityManager->HasComponent<ColliderComponent>(e1) == false) return false;
        for(const auto& collider : colliders){
            if(collider.first == e1) continue;
            if(entityManager->HasComponent<ColliderComponent>(collider.first)){
                auto& collider1 = entityManager->GetComponent<ColliderComponent>(e1);
                auto& collider2 = entityManager->GetComponent<ColliderComponent>(collider.first);
                switch(collider2.type) {
                    case ColliderType::SPHERE: {
                        SphereCollider col = *static_cast<SphereCollider*>(collider2.collider.get());
                        if(collider1.intersectColliders(col)){
                                return true;
                                std::cout<<"position e1: "<< col.position.x<<" "<< col.position.y<<" "<< col.position.z<<std::endl;

                            }
                        break;
                    }
                    case ColliderType::AABB: 
                        if(collider1.intersectColliders(
                            *static_cast<AABBCollider*>(collider2.collider.get()))){
                                return true;
                            }
                        break;
                    case ColliderType::OBB:
                        if(collider1.intersectColliders(
                            *static_cast<OBBCollider*>(collider2.collider.get()))){
                                return true;
                            }
                        break;
                    case ColliderType::PLANE: {
                        if(collider1.intersectColliders(
                            *static_cast<PlaneCollider*>(collider2.collider.get()))){
                                return true;
                            }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        return false;
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
            std::vector<uint32_t> collidingEntities;
            for(const auto& collider : colliders){
                if(isColliding(collider.first) && entityManager->HasComponent<RigidBodyComponent>(collider.first)){
                    std::cout<<"Collision detected for entity "<<collider.first<<std::endl;
                    entityManager->GetComponent<RigidBodyComponent>(collider.first).velocity = glm::vec3(0.0f);
                    entityManager->GetComponent<RigidBodyComponent>(collider.first).gravity = glm::vec3(0.0f);
                    collidingEntities.push_back(collider.first);
                }
            }
            for(const auto& rb : rigidBodies){
                if(std::find(collidingEntities.begin(), collidingEntities.end(), rb.first) != collidingEntities.end()){
                    continue; // Skip velocity computation for colliding entities
                }
                computeVelocity(rb.first, fixedDeltaTime);
    
            }
            accumulator -= FIXED_DELTA_TIME;
        }
    }
};

#endif // PHYSICSYSTEM_H