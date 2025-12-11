#ifndef PHYSICSYSTEM_H
#define PHYSICSYSTEM_H
#include <unordered_map>
#include "../common/Geometry3D.h"
#include <iostream>

class PhysicSystem {
    const double FIXED_DELTA_TIME = 1.0/60.0;
    double accumulator = 0.0;
    const double MAX_ACCUMULATOR = 1.0/60.0;
    std::vector<uint32_t> bodies;
    std::vector<uint32_t> colliders1;
    std::vector<uint32_t> colliders2;
    std::vector<CollisionManifold> results;

public:
    EntityManager* entityManager;
    float linearProjectionPercent = 0.8f;
    float penetrationSlack = 0.01f;
    int ImpulseIterations = 10;

    PhysicSystem(EntityManager* em) : entityManager(em) {
        colliders1.reserve(100);
        colliders2.reserve(100);
        results.reserve(100);
    }

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

    glm::mat4 invTensor(uint32_t e){
        float ix = 0.0f;
        float iy = 0.0f;
        float iz = 0.0f;
        float iw = 0.0f;
        auto& rigid = entityManager->GetComponent<RigidBodyComponent>(e);
        auto& colliderComp = entityManager->GetComponent<ColliderComponent>(e);
        if (rigid.mass != 0 && colliderComp.type == ColliderType::SPHERE) {
            auto* sphere = static_cast<SphereCollider*>(colliderComp.collider.get());
            float r2 = sphere->radius * sphere->radius;
            float fraction = (2.0f / 5.0f);
            ix = r2 * rigid.mass * fraction;
            iy = r2 * rigid.mass * fraction;
            iz = r2 * rigid.mass * fraction;
            iw = 1.0f;
        }else if(colliderComp.type == ColliderType::OBB){
                auto* obb = static_cast<OBBCollider*>(colliderComp.collider.get());
                glm::vec3 size = obb->size * 2.0f;
                float fraction = (1.0f / 12.0f);
                float x2 = size.x * size.x;
                float y2 = size.y * size.y;
                float z2 = size.z * size.z;
                ix = (y2 + z2) * rigid.mass * fraction;
                iy = (x2 + z2) * rigid.mass * fraction;
                iz = (x2 + y2) * rigid.mass * fraction;
                iw = 1.0f;
            }
        return glm::inverse(glm::mat4(
                ix, 0, 0, 0,
                0, iy, 0, 0,
                0, 0, iz, 0,
                0, 0, 0, iw));
    }

    void addRotationImpulse(uint32_t e, const glm::vec3& impulse){
        auto& rigid = entityManager->GetComponent<RigidBodyComponent>(e);
        glm::vec3& centerOfMass = entityManager->GetComponent<TransformComponent>(e).position;
        glm::vec3 torque = glm::cross(impulse - centerOfMass, impulse);
        glm::vec3 angAcc = glm::vec3(invTensor(e) * glm::vec4(torque, 0.0f));
        rigid.angVel += angAcc;
    }

    void ApplyImpulse(uint32_t e1,  uint32_t e2, const CollisionManifold& manifold, float restitution){
        auto& rigid1 = entityManager->GetComponent<RigidBodyComponent>(e1);
        auto& rigid2 = entityManager->GetComponent<RigidBodyComponent>(e2);
        if(glm::dot(rigid1.velocity, rigid1.velocity) < 0.00001f && glm::dot(rigid2.velocity, rigid2.velocity) < 0.00001f) return;
        float invMass1 = InvMass(e1);
        float invMass2 = InvMass(e2);
        float invMassSum = invMass1 + invMass2;
        if(invMassSum == 0.0f) return;
        glm::vec3 relativeVelocity = rigid2.velocity - rigid1.velocity;
        glm::vec3 relativeNormal = glm::normalize(manifold.normal);
        if(glm::dot(relativeVelocity, relativeNormal) > 0) return;
        float e = fminf(rigid1.bounce, rigid2.bounce);
        float numerator = (-(1.0f + e) * glm::dot(relativeVelocity, relativeNormal));
        float j = numerator / invMassSum;
        if(manifold.contacts.size() > 0){
            j /= static_cast<float>(manifold.contacts.size());
        }
        glm::vec3 impulse = j * relativeNormal;
        rigid1.velocity -= impulse * invMass1;
        rigid2.velocity += impulse * invMass2;
        glm::vec3 t = relativeVelocity - (glm::dot(relativeVelocity, relativeNormal) * relativeNormal);
        if(CMP(MagnitudeSq(t),0.0f)) return;
        t = glm::normalize(t);
        numerator = -glm::dot(relativeVelocity, t);
        float jt = numerator / invMassSum;
        if(manifold.contacts.size() > 0 && jt != 0.0f){
            jt /= static_cast<float>(manifold.contacts.size());
        }
        if(CMP(jt,0.0f)) return;
        float friction = std::sqrt(rigid1.friction * rigid2.friction);
        if(jt > j * friction){
            jt = j * friction;
        } else if(jt < -j * friction){
            jt = -j * friction;
        }
        glm::vec3 frictionImpulse = jt * t;
        rigid1.velocity -= frictionImpulse * invMass1;
        rigid2.velocity += frictionImpulse * invMass2;
    }

    float InvMass(uint32_t e){
        auto& rigid = entityManager->GetComponent<RigidBodyComponent>(e);
        if(rigid.mass ==0.0f) return 0.0f;
        return 1.0f/rigid.mass;
    }

    void SynchCollisionVolumes(){
        const auto& colliders = entityManager->GetComponents<ColliderComponent>();
        for(const auto& pair : colliders){
            uint32_t id = pair.first;
            auto& t = entityManager->GetComponent<TransformComponent>(id);
            auto& col = entityManager->GetComponent<ColliderComponent>(id);
            if(col.type == ColliderType::SPHERE) {
                auto& collider = static_cast<SphereCollider&>(*col.collider);
                collider.position = t.position;
            } else if(col.type == ColliderType::OBB){
                auto& collider = static_cast<OBBCollider&>(*col.collider);
                collider.position = t.position;
                collider.orientation = glm::mat3(glm::quat(glm::radians(t.rotation)));
            }
        }
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
    if (accumulator > MAX_ACCUMULATOR) accumulator = MAX_ACCUMULATOR;
    const float fixedDeltaTime = static_cast<float>(FIXED_DELTA_TIME);

    while (accumulator >= FIXED_DELTA_TIME) {
        bodies.clear();
        for (const auto& rb : entityManager->GetComponents<RigidBodyComponent>()) {
            bodies.push_back(rb.first);
        }
        colliders1.clear();
        colliders2.clear();
        results.clear();
        for (int i = 0; i < bodies.size(); ++i) {
            for (int j = 0; j < bodies.size(); ++j) {
                if(i == j) continue;
                uint32_t idA = bodies[i];
                uint32_t idB = bodies[j];
                CollisionManifold manifold;
                manifold = FindCollisionFeatures(
                    entityManager->GetComponent<ColliderComponent>(idA).collider.get(),
                    entityManager->GetComponent<ColliderComponent>(idB).collider.get()
                );
                //std::cout << "Testing collision between Entity " << idA << " and Entity " << idB << std::endl;
                if (manifold.colliding) {
                    //std::cout << "Collision detected between Entity " << idA << " and Entity " << idB << std::endl;
                    colliders1.push_back(idA);
                    colliders2.push_back(idB);
                    results.push_back(manifold);
                }
            }
        }
        for (uint32_t e : bodies) {
            ApplyForces(e);
        }
        for (int k = 0; k < ImpulseIterations; ++k) {
            for (int i = 0; i < results.size(); ++i) {
                for (int j = 0; j < results[i].contacts.size(); ++j) {
                    ApplyImpulse(colliders1[i], colliders2[i], results[i], 0.7f);
                }
            }
        }
        for (uint32_t e : bodies) {
            computeVelocity(e, fixedDeltaTime);
            // if(entityManager->HasComponent<ColliderComponent>(e)){
            //     auto& collider = entityManager->GetComponent<ColliderComponent>(e);
            //     auto& rigid =  entityManager->GetComponent<RigidBodyComponent>(e);
            //     if(collider.type == ColliderType::SPHERE || collider.type == ColliderType::OBB){
            //         glm::vec3 angAccel = rigid.torques * invTensor(e);
            //         angVel = angVel + angAccel * dt;
            //         angVel = angVel * damping;
            //         SynchCollisionVolumes();
            //     }
            // }
        }
        // Vélocité linéaire
        for (int i = 0; i < results.size(); ++i) {
            float invMass1 = InvMass(colliders1[i]);
            float invMass2 = InvMass(colliders2[i]);
            float totalMass = invMass1 + invMass2;
            if (totalMass == 0.0f) continue;
            float depth = std::max(results[i].depth - penetrationSlack, 0.0f);
            float scalar = depth / totalMass;
            glm::vec3 correction = results[i].normal * scalar * linearProjectionPercent;
            auto& t1 = entityManager->GetComponent<TransformComponent>(colliders1[i]);
            auto& t2 = entityManager->GetComponent<TransformComponent>(colliders2[i]);
            t1.position -= correction * invMass1;
            t2.position += correction * invMass2;
        }
        for (uint32_t e : bodies) {
            SolveConstraints(e, getWorldConstraints());
        }

        accumulator -= FIXED_DELTA_TIME;
    }
}
};

#endif // PHYSICSYSTEM_H