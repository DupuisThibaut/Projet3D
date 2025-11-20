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
    
            for(const auto& rb : rigidBodies){
                computeVelocity(rb.first, fixedDeltaTime);
    
            }
            accumulator -= FIXED_DELTA_TIME;
        }
    }
};

#endif // PHYSICSYSTEM_H