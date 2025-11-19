#ifndef PHYSICSYSTEM_H
#define PHYSICSYSTEM_H
#include <unordered_map>

class PhysicSystem {

public:
    EntityManager* entityManager;

    PhysicSystem(EntityManager* em) : entityManager(em) {}


    void computeVelocity(uint32_t e, float dt){
        auto& rigid = entityManager->GetComponent<RigidBodyComponent>(e);
        auto& transform = entityManager->GetComponent<TransformComponent>(e);
        std::cout << typeid(rigid.gravity).name() << std::endl;
        // rigid.acceleration = (1.0/rigid.mass) * rigid.gravity;
        // rigid.velocity = rigid.velocity * rigid.friction + dt * rigid.acceleration;
        // transform.position = transform.position + rigid.velocity*dt;
    }

    
    void update(float deltaTime) {
        const auto& rigidBodies = entityManager->GetComponents<RigidBodyComponent>();
        const auto& colliders = entityManager->GetComponents<ColliderComponent>();
        if(rigidBodies.empty() && colliders.empty()) return;

        for(const auto& rb : rigidBodies){
            computeVelocity(rb.first, deltaTime);

        }
    }
};

#endif // PHYSICSYSTEM_H