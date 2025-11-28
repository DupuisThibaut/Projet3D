#ifndef COLLIDER_COMPONENT_H
#define COLLIDER_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

float MagnitudeSq(const glm::vec3& v) {
    return glm::dot(v, v);
}

enum class ColliderType {
    AABB,
    OBB,
    SPHERE,
    PLANE,
    MESH
};

struct ICollider{
    virtual ~ICollider() = default;
    virtual ColliderType getType() const = 0;
    virtual std::unique_ptr<ICollider> clone() const = 0;
    virtual bool PointInCollider(const glm::vec3& point) const = 0;
    virtual glm::vec3 ClosestPoint(const glm::vec3& point) const = 0;
};

struct SphereCollider : ICollider {
    glm::vec3 position = glm::vec3(0.0f);
    float radius = 1.0f;

    SphereCollider() = default;
    SphereCollider(const glm::vec3& p, float r) : position(p), radius(r) {}
    ColliderType getType() const override {
        return ColliderType::SPHERE;
    }
    std::unique_ptr<ICollider> clone() const override {
        return std::make_unique<SphereCollider>(*this);
    }

    bool PointInCollider(const glm::vec3& point) const {
        float magSq = MagnitudeSq(point - position);
        float radSq = radius * radius;
        return magSq <= radSq;
    }

    glm::vec3 ClosestPoint(const glm::vec3& point) const {
        glm::vec3 dir = glm::normalize(point - position);
        dir *= radius;
        return dir + position;
    }
};

struct AABBCollider : ICollider {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 size = glm::vec3(1.0f);

    AABBCollider() = default;
    AABBCollider(const glm::vec3& p, const glm::vec3& s) : position(p), size(s) {}
    ColliderType getType() const override {
        return ColliderType::AABB;
    }
    std::unique_ptr<ICollider> clone() const override {
        return std::make_unique<AABBCollider>(*this);
    }

    glm::vec3 getMin() const {
        glm::vec3 p1 = position + size;
        glm::vec3 p2 = position - size;
        return glm::vec3(fminf(p1.x, p2.x),fminf(p1.y, p2.y),fminf(p1.z, p2.z));
    }

    glm::vec3 getMax() const {
        glm::vec3 p1 = position + size;
        glm::vec3 p2 = position - size;
        return glm::vec3(fmaxf(p1.x, p2.x),fmaxf(p1.y, p2.y),fmaxf(p1.z, p2.z));
    }

    void FromMinMax(const glm::vec3& min, const glm::vec3& max) {
        position = (min + max) * 0.5f;
        size = (max - min) * 0.5f;
    }

    bool PointInCollider(const glm::vec3& point) const {
        glm::vec3 min = getMin();
        glm::vec3 max = getMax();
        if(point.x < min.x || point.x > max.x ||
           point.y < min.y || point.y > max.y ||
           point.z < min.z || point.z > max.z) {
            return false;
        }
        return true;
    }

    glm::vec3 ClosestPoint(const glm::vec3& point) const {
        glm::vec3 min = getMin();
        glm::vec3 max = getMax();
        glm::vec3 clamped;
        clamped.x = fmaxf(min.x, fminf(point.x, max.x));
        clamped.y = fmaxf(min.y, fminf(point.y, max.y));
        clamped.z = fmaxf(min.z, fminf(point.z, max.z));
        return clamped;
    }
};

struct OBBCollider : ICollider {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 size = glm::vec3(1.0f);
    glm::mat3 orientation = glm::mat3(1.0f);

    OBBCollider() = default;
    OBBCollider(const glm::vec3& p, const glm::vec3& s) : position(p), size(s), orientation(glm::mat3(1.0f)) {}
    OBBCollider(const glm::vec3& c, const glm::vec3& s, const glm::mat3& r) : position(c), size(s), orientation(r) {}
    ColliderType getType() const override {
        return ColliderType::OBB;
    }
    std::unique_ptr<ICollider> clone() const override {
        return std::make_unique<OBBCollider>(*this);
    }

    bool PointInCollider(const glm::vec3& point) const {
        glm::vec3 dir = point - position;
        for (int i = 0; i < 3; ++i) {
            const float* orientation = &this->orientation[0][i];
            glm::vec3 axis = glm::vec3(orientation[0], orientation[1], orientation[2]);
            float distance = glm::dot(dir, axis);
            if (distance > size[i] || distance < -size[i]) {
                return false;
            }
        }
        return true;
    }

    glm::vec3 ClosestPoint(const glm::vec3& point) const {
        glm::vec3 dir = point - position;
        glm::vec3 closest = position;

        for (int i = 0; i < 3; ++i) {
            const float* orientation = &this->orientation[0][i];
            glm::vec3 axis = glm::vec3(orientation[0], orientation[1], orientation[2]);
            float distance = glm::dot(dir, axis);
            if (distance > size[i]) distance = size[i];
            if (distance < -size[i]) distance = -size[i];
            closest += distance * axis;
        }
        return closest;
    }
};

struct PlaneCollider : ICollider {
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    float distance = 0.0f;

    PlaneCollider() = default;
    PlaneCollider(const glm::vec3& n, float d) : normal(n), distance(d) {}
    ColliderType getType() const override {
        return ColliderType::PLANE;
    }
    std::unique_ptr<ICollider> clone() const override {
        return std::make_unique<PlaneCollider>(*this);
    }

    float PlaneEquation(const glm::vec3& point) const {
        return glm::dot(point, normal) - distance;
    }

    bool PointInCollider(const glm::vec3& point) const {
        float dot = glm::dot(point, normal);
        return dot - distance == 0.0f;
    }

    glm::vec3 ClosestPoint(const glm::vec3& point) const {
        float dot = glm::dot(point, normal);
        float t = dot - distance;
        return point - t * normal;
    }
};

// struct Line{
//     glm::vec3 start;
//     glm::vec3 end;
//     Line() = default;
//     Line(const glm::vec3& s, const glm::vec3& e) : start(s), end(e) {}
//     float Length() const {
//         return glm::length(end - start);
//     }
//     float LengthSq() const {
//         return MagnitudeSq(start - end);
//     }

//     glm::vec3 ClosestPoint(const glm::vec3& point) const {
//         glm::vec3 lVec = end - start;
//         float t = glm::dot(point - start, lVec) / glm::dot(lVec, lVec);
//         t = fmaxf(0.0f, fminf(1.0f, t));
//         return start + t * lVec;
//     }

//     bool PointOnLine(const glm::vec3& point) const {
//         float lineLenSq = LengthSq();
//         float d1 = MagnitudeSq(point - start);
//         float d2 = MagnitudeSq(point - end);
//         return fabsf(lineLenSq - (d1 + d2)) < 0.0001f;
//     }
// };

struct TriangleCollider;
struct Interval{
    float min;
    float max;

    Interval() : min(0.0f), max(0.0f) {}
    Interval(float mi, float ma) : min(mi), max(ma) {}

    Interval GetInterval(const AABBCollider& aabb, const glm::vec3& axis) {
        glm::vec3 corners[8];
        glm::vec3 i = aabb.getMin();
        glm::vec3 a = aabb.getMax();
        corners[0] = glm::vec3(i.x, a.y, a.z);
        corners[1] = glm::vec3(i.x, a.y, i.z);
        corners[2] = glm::vec3(i.x, i.y, a.z);
        corners[3] = glm::vec3(i.x, i.y, i.z);
        corners[4] = glm::vec3(a.x, a.y, a.z);
        corners[5] = glm::vec3(a.x, a.y, i.z);
        corners[6] = glm::vec3(a.x, i.y, a.z);
        corners[7] = glm::vec3(a.x, i.y, i.z);
        Interval result;
        result.min = glm::dot(corners[0], axis);
        result.max = result.min;
        for(int i = 1; i < 8; ++i) {
            float projection = glm::dot(axis, corners[i]);
            result.min = (projection < result.min) ? projection : result.min;
            result.max = (projection > result.max) ? projection : result.max;
        }
        return result;
    }

    Interval GetInterval(const OBBCollider& obb, const glm::vec3& axis) {
        glm::vec3 corners[8];
        glm::vec3 C = obb.position;
        glm::vec3 E = obb.size;
        const glm::mat3& R = obb.orientation;
        glm::vec3 A[] = {
            glm::vec3(R[0][0], R[1][0], R[2][0]),
            glm::vec3(R[0][1], R[1][1], R[2][1]),
            glm::vec3(R[0][2], R[1][2], R[2][2])
        };
        corners[0] = C + A[0]*E.x + A[1]*E.y + A[2]*E.z;
        corners[1] = C - A[0]*E.x + A[1]*E.y + A[2]*E.z;
        corners[2] = C + A[0]*E.x - A[1]*E.y + A[2]*E.z;
        corners[3] = C + A[0]*E.x + A[1]*E.y - A[2]*E.z;
        corners[4] = C - A[0]*E.x - A[1]*E.y - A[2]*E.z;
        corners[5] = C + A[0]*E.x - A[1]*E.y - A[2]*E.z;
        corners[6] = C - A[0]*E.x + A[1]*E.y - A[2]*E.z;
        corners[7] = C - A[0]*E.x - A[1]*E.y + A[2]*E.z;
        Interval result;
        result.min = glm::dot(axis, corners[0]);
        result.max = result.min;
        for(int i = 1; i < 8; ++i) {
            float projection = glm::dot(axis, corners[i]);
            result.min = (projection < result.min) ? projection : result.min;
            result.max = (projection > result.max) ? projection : result.max;
        }
        return result;
    }

    Interval GetInterval(const TriangleCollider& tri, const glm::vec3& axis); 
};

struct TriangleCollider : ICollider {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec3 c;

    TriangleCollider() = default;
    TriangleCollider(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) : a(a), b(b), c(c) {}
    ColliderType getType() const override {
        return ColliderType::MESH;
    }
    std::unique_ptr<ICollider> clone() const override {
        return std::make_unique<TriangleCollider>(*this);
    }
    bool PointInCollider(const glm::vec3& point) const {
        glm::vec3 a = a-point;
        glm::vec3 b = b-point;
        glm::vec3 c = c-point;
        glm::vec3 normPBC = glm::cross(b, c);
        glm::vec3 normPCA = glm::cross(c, a);
        glm::vec3 normPAB = glm::cross(a, b);
        if (glm::dot(normPBC, normPCA) < 0.0f) return false;
        if (glm::dot(normPBC, normPAB) < 0.0f) return false;
        return true;
    }
    PlaneCollider FromTriangle() const {
        PlaneCollider plane;
        plane.normal = glm::normalize(glm::cross(b - a, c - a));
        plane.distance = glm::dot(plane.normal, a);
        return plane;
    }
    glm::vec3 ClosestPoint(const glm::vec3& point) const {
        PlaneCollider plane = FromTriangle();
        if(PointInCollider(point)){
            return plane.ClosestPoint(point);
        }
        glm::vec3 c1 = Line(a,b).ClosestPoint(point);
        glm::vec3 c2 = Line(b,c).ClosestPoint(point);
        glm::vec3 c3 = Line(c,a).ClosestPoint(point);
        float magSq1 = MagnitudeSq(point - c1);
        float magSq2 = MagnitudeSq(point - c2);
        float magSq3 = MagnitudeSq(point - c3);
        if(magSq1 <= magSq2 && magSq1 <= magSq3){
            return c1;
        }
        else if(magSq2 <= magSq1 && magSq2 <= magSq3){
            return c2;
        }
        else{
            return c3;
        }
    }

    bool Intersect(const SphereCollider& sphere) const {
        glm::vec3 closestPoint = ClosestPoint(sphere.position);
        float distSq = MagnitudeSq(closestPoint - sphere.position);
        return distSq <= (sphere.radius * sphere.radius);
    }

    bool OverlapOnAxis(const AABBCollider& aabb, const glm::vec3& axis) const {
        Interval a = Interval().GetInterval(aabb, axis);
        Interval b = Interval().GetInterval(*this, axis);
        return (b.min <= a.max) && (a.min <= b.max);
    }

    bool Intersect(const AABBCollider& aabb) const {
        glm::vec3 f0 = b - a;
        glm::vec3 f1 = c - b;
        glm::vec3 f2 = a - c;
        glm::vec3 u0 = glm::vec3(1, 0, 0);
        glm::vec3 u1 = glm::vec3(0, 1, 0);
        glm::vec3 u2 = glm::vec3(0, 0, 1);
        glm::vec3 test[13] ={
            u0,
            u1,
            u2,
            glm::cross(f0, f1),
            glm::cross(u0,f0),
            glm::cross(u0,f1),
            glm::cross(u0,f2),
            glm::cross(u1,f0),
            glm::cross(u1,f1),
            glm::cross(u1,f2),
            glm::cross(u2,f0),
            glm::cross(u2,f1),
            glm::cross(u2,f2)
        };
        for(int i =0; i<13; i++){
            if(!OverlapOnAxis(aabb, test[i])){
                return false;
            }
        }
        return true;
    }

    bool OverlapOnAxis(const OBBCollider& obb, const glm::vec3& axis) const {
        Interval a = Interval().GetInterval(obb, axis);
        Interval b = Interval().GetInterval(*this, axis);
        return (b.min <= a.max) && (a.min <= b.max);
    }

    bool Intersect(const OBBCollider& obb) const {
        glm::vec3 f0 = b - a;
        glm::vec3 f1 = c - b;
        glm::vec3 f2 = a - c;
        glm::vec3 o[3] = {
            obb.orientation[0],
            obb.orientation[1],
            obb.orientation[2]
        };
        glm::vec3 test[13] ={
            o[0],
            o[1],
            o[2],
            glm::cross(f0, f1),
            glm::cross(o[0],f0),
            glm::cross(o[0],f1),
            glm::cross(o[0],f2),
            glm::cross(o[1],f0),
            glm::cross(o[1],f1),
            glm::cross(o[1],f2),
            glm::cross(o[2],f0),
            glm::cross(o[2],f1),
            glm::cross(o[2],f2)
        };
        for(int i =0; i<13; i++){
            if(!OverlapOnAxis(obb, test[i])){
                return false;
            }
        }
        return true;
    }

    bool Intersect(const PlaneCollider& plane) const {
        float side1 = plane.PlaneEquation(a);
        float side2 = plane.PlaneEquation(b);
        float side3 = plane.PlaneEquation(c);
        if ((side1 > 0.0f && side2 > 0.0f && side3 > 0.0f) ||
            (side1 < 0.0f && side2 < 0.0f && side3 < 0.0f)) {
            return false;
        }
        return true;
    }

    bool OverlapOnAxis(const TriangleCollider& other, const glm::vec3& axis) const {
        Interval a = Interval().GetInterval(other, axis);
        Interval b = Interval().GetInterval(*this, axis);
        return (b.min <= a.max) && (a.min <= b.max);
    }

    glm::vec3 SatCrossEdge(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d) const {
        glm::vec3 ab = a-b;
        glm::vec3 cd = c-d;
        glm::vec3 result = glm::cross(ab, cd);
        if(!MagnitudeSq(result) == 0.0f){
            return result;
        } else {
            glm::vec3 axis = glm::cross(ab, c - a);
            result = glm::cross(ab, axis);
            if(!MagnitudeSq(result) == 0.0f){
                return result;
            }
        }
        return glm::vec3(1.0f, 0.0f, 0.0f); // Fallback axis
    }

    bool Intersect(const TriangleCollider& other) const {
        glm::vec3 axisToTest[] = {
            SatCrossEdge(a, b, b, c),
            SatCrossEdge(other.a, other.b, other.c, other.a),
            SatCrossEdge(other.a, other.b, a, b),
            SatCrossEdge(other.a, other.b, b, c),
            SatCrossEdge(other.a, other.b, c, a),
            SatCrossEdge(other.b, other.c, a, b),
            SatCrossEdge(other.b, other.c, b, c),
            SatCrossEdge(other.b, other.c, c, a),
            SatCrossEdge(other.c, other.a, a, b),
            SatCrossEdge(other.c, other.a, b, c),
            SatCrossEdge(other.c, other.a, c, a)
        };
        for(int i =0; i<11; i++){
            if(!OverlapOnAxis(other, axisToTest[i])){
                if(MagnitudeSq(axisToTest[i]) != 0.0f) return false;
            }
        }
        return true;
    }


};
inline Interval Interval::GetInterval(const TriangleCollider& tri, const glm::vec3& axis) {
    glm::vec3 corners[3] = {tri.a, tri.b, tri.c};
    Interval result;
    result.min = glm::dot(axis, corners[0]);
    result.max = result.min;
    for(int i = 1; i < 3; ++i) {
        float projection = glm::dot(axis, corners[i]);
        result.min = fminf(result.min, projection);
        result.max = fmaxf(result.max, projection);
    }
    return result;
}

struct BVHNode {
    AABBCollider bounds;
    BVHNode * children;
    int numTriangles;
    int* triangles;
    BVHNode() : children(0), numTriangles(0), triangles(0) {};
};

struct MeshCollider : ICollider{
    int numTriangles = 0;
    TriangleCollider* triangles = nullptr;
    glm::vec3* vertices = nullptr;
    float* values = nullptr;
    BVHNode* accelerator;
    MeshCollider() : numTriangles(0),  values(0), accelerator(0) {}
    ColliderType getType() const override {
        return ColliderType::MESH;
    }
    std::unique_ptr<ICollider> clone() const override {
        return std::make_unique<MeshCollider>(*this);
    }
    bool PointInCollider(const glm::vec3& point) const {
        // Point-in-mesh test not implemented
        return false;
    }
    glm::vec3 ClosestPoint(const glm::vec3& point) const {
        // Closest point on mesh not implemented
        return glm::vec3(0.0f);
    }
    void AccelerateMesh(){
        if(accelerator != 0) return;
        glm::vec3 min = vertices[0];
        glm::vec3 max = vertices[0];
        for(int i = 1; i < numTriangles * 3; ++i) {
            glm::vec3 v = vertices[i];
            min.x = fminf(min.x, v.x);
            min.y = fminf(min.y, v.y);
            min.z = fminf(min.z, v.z);
            max.x = fmaxf(max.x, v.x);
            max.y = fmaxf(max.y, v.y);
            max.z = fmaxf(max.z, v.z);
        }
        accelerator = new BVHNode();
        accelerator->bounds.position = min;
        accelerator->bounds.size = max - min;
        accelerator->numTriangles = numTriangles;
        accelerator->triangles = new int[numTriangles];
        for(int i = 0; i < numTriangles; ++i) {
            accelerator->triangles[i] = i;
        }
        SplitBVHNode(accelerator, 3);
    }
    void SplitBVHNode(BVHNode* node, int depth) {
        if(depth == 0) {
            return;
        }
        if(node->children ==0){
            if (node->numTriangles >0) {
                node->children = new BVHNode[8];
                glm::vec3 c = node->bounds.position;
                glm::vec3 e = node->bounds.size * 0.5f;
                node->children[0].bounds = AABBCollider(c + glm::vec3(-e.x, +e.y, -e.z), e);
                node->children[1].bounds = AABBCollider(c + glm::vec3(+e.x, +e.y, -e.z), e);
                node->children[2].bounds = AABBCollider(c + glm::vec3(-e.x, +e.y, +e.z), e);
                node->children[3].bounds = AABBCollider(c + glm::vec3(+e.x, +e.y, +e.z), e);
                node->children[4].bounds = AABBCollider(c + glm::vec3(-e.x, -e.y, -e.z), e);
                node->children[5].bounds = AABBCollider(c + glm::vec3(+e.x, -e.y, -e.z), e);
                node->children[6].bounds = AABBCollider(c + glm::vec3(-e.x, -e.y, +e.z), e);
                node->children[7].bounds = AABBCollider(c + glm::vec3(+e.x, -e.y, +e.z), e);
            }
        }
        if(node->children != 0 && node->numTriangles > 0) {
            for(int i = 0; i < 8; ++i) {
                node->children[i].numTriangles = 0;
                for(int j = 0; j < node->numTriangles; ++j) {
                    TriangleCollider t = triangles[node->triangles[j]];
                    if(t.Intersect(node->children[i].bounds)) {
                        node->children[i].numTriangles++;
                    }
                }
                if(node->children[i].numTriangles == 0) continue;
                node->children[i].triangles = new int[node->children[i].numTriangles];
                int index = 0;
                for(int j = 0; j < node->numTriangles; ++j) {
                    TriangleCollider t = triangles[node->triangles[j]];
                    if(t.Intersect(node->children[i].bounds)) {
                        node->children[i].triangles[index++] = node->triangles[j];
                    }
                }
            }
            node->numTriangles = 0;
            delete[] node->triangles;
            node->triangles = 0;
            for(int i = 0; i < 8; ++i) {
                SplitBVHNode(&node->children[i], depth);
            }
        }
    }

    void FreeBVHNode(BVHNode* node) {
        if(node->children != 0) {
            for(int i = 0; i < 8; ++i) {
                FreeBVHNode(&node->children[i]);
            }
            delete[] node->children;
            node->children = 0;
        }
        if(node->triangles != 0|| node->numTriangles != 0) {
            delete[] node->triangles;
            node->triangles = 0;
            node->numTriangles = 0;
        }
    }
};


struct ColliderComponent {
    ColliderType type = ColliderType::SPHERE;
    bool isTrigger = false;

    std::unique_ptr<ICollider> collider;

    ColliderComponent() : collider(std::make_unique<SphereCollider>()) {}

    ColliderComponent(const ColliderComponent& other) 
        : type(other.type), isTrigger(other.isTrigger), collider(other.collider->clone()) {}
        
    ColliderComponent(const ICollider& col, bool trigger = false) 
        : type(col.getType()), isTrigger(trigger), collider(col.clone()) {}

    ColliderComponent& operator=(const ColliderComponent& other) {
        if (this != &other) {
            type = other.type;
            isTrigger = other.isTrigger;
            collider = other.collider->clone();
        }
        return *this;
    }

    void loadFromFile(const nlohmann::json& entityData, uint32_t& index){
        if (!entityData.contains("collider")) {
            std::cerr << "Collider data not found in entity data." << std::endl;
            return;
        }
        std::string colliderType = entityData["collider"][index]["type"];
        isTrigger = entityData["collider"][index]["isTrigger"];
        if(colliderType == "SPHERE"){
            type = ColliderType::SPHERE;
            glm::vec3 center = glm::vec3(
                entityData["collider"][index]["position"][0],
                entityData["collider"][index]["position"][1],
                entityData["collider"][index]["position"][2]
            );
            float radius = entityData["collider"][index]["radius"];
            collider = std::make_unique<SphereCollider>(center, radius);
        }
        else if(colliderType == "AABB"){
            type = ColliderType::AABB;
            glm::vec3 origin = glm::vec3(
                entityData["collider"][index]["position"][0],
                entityData["collider"][index]["position"][1],
                entityData["collider"][index]["position"][2]
            );
            glm::vec3 size = glm::vec3(
                entityData["collider"][index]["size"][0],
                entityData["collider"][index]["size"][1],
                entityData["collider"][index]["size"][2]
            );
            collider = std::make_unique<AABBCollider>(origin, size);
        }
        else if(colliderType == "OBB"){
            type = ColliderType::OBB;
            glm::vec3 position = glm::vec3(
                entityData["collider"][index]["position"][0],
                entityData["collider"][index]["position"][1],
                entityData["collider"][index]["position"][2]
            );
            glm::vec3 size = glm::vec3(
                entityData["collider"][index]["size"][0],
                entityData["collider"][index]["size"][1],
                entityData["collider"][index]["size"][2]
            );
            glm::mat3 orientation = glm::mat3(1.0f);
            for(int i = 0; i < 3; ++i){
                for(int j = 0; j < 3; ++j){
                    orientation[i][j] = entityData["collider"][index]["orientation"][i][j];
                }
            }
            collider = std::make_unique<OBBCollider>(position, size, orientation);
        }
        else if(colliderType == "PLANE"){
            type = ColliderType::PLANE;
            glm::vec3 normal = glm::vec3(
                entityData["colliders"][index]["normal"][0],
                entityData["colliders"][index]["normal"][1],
                entityData["colliders"][index]["normal"][2]
            );
            float distance = entityData["colliders"][index]["distance"];
            collider = std::make_unique<PlaneCollider>(normal, distance);
        }
        else if(colliderType == "MESH"){
            type = ColliderType::MESH;
            // Mesh loading not implemented
        }
        index++;
    }

    bool OverlapOnAxis(const AABBCollider& aabb, const OBBCollider& obb, const glm::vec3& axis) const {
        Interval a = Interval().GetInterval(aabb, axis);
        Interval b = Interval().GetInterval(obb, axis);
        return (b.min <= a.max) && (a.min <= b.max);
    }

    bool OverlapOnAxis(const OBBCollider& obb1, const OBBCollider& obb2, const glm::vec3& axis) const {
        Interval a = Interval().GetInterval(obb1, axis);
        Interval b = Interval().GetInterval(obb2, axis);
        return (b.min <= a.max) && (a.min <= b.max);
    }

    bool intersectColliders(const SphereCollider& sphere) const {
        if(type == ColliderType::SPHERE) {
            float radiisumSum = static_cast<SphereCollider*>(collider.get())->radius + sphere.radius;
            float distSq = MagnitudeSq(static_cast<SphereCollider*>(collider.get())->position - sphere.position);
            return distSq <= (radiisumSum * radiisumSum);
        }
        else if(type == ColliderType::AABB){
            glm::vec3 closestPoint = static_cast<AABBCollider*>(collider.get())->ClosestPoint(sphere.position);
            float distSq = MagnitudeSq(closestPoint - sphere.position);
            return distSq <= (sphere.radius * sphere.radius);
        }
        else if(type == ColliderType::OBB){
            glm::vec3 closestPoint = static_cast<OBBCollider*>(collider.get())->ClosestPoint(sphere.position);
            float distSq = MagnitudeSq(closestPoint - sphere.position);
            return distSq <= (sphere.radius * sphere.radius);
        }
        else if(type == ColliderType::PLANE){
            glm::vec3 closestPoint = sphere.position - static_cast<PlaneCollider*>(collider.get())->ClosestPoint(sphere.position);
            float distSq = MagnitudeSq(sphere.position - closestPoint);
            return distSq <= (sphere.radius * sphere.radius);
        }
        else if (type == ColliderType::MESH) {
            
        }
        return false;
    }
    bool intersectColliders(const AABBCollider& aabb) const {
        if(type == ColliderType::SPHERE) {
            SphereCollider sphere = *static_cast<SphereCollider*>(collider.get());
            glm::vec3 closestPoint = aabb.ClosestPoint(sphere.position);
            float distSq = MagnitudeSq(closestPoint - sphere.position);
            return distSq <= (sphere.radius * sphere.radius);
        }
        else if(type == ColliderType::AABB){
            glm::vec3 aMin = aabb.getMin();
            glm::vec3 aMax = aabb.getMax();
            glm::vec3 bMin = static_cast<AABBCollider*>(collider.get())->getMin();
            glm::vec3 bMax = static_cast<AABBCollider*>(collider.get())->getMax();
            return (aMin.x <= bMax.x && aMax.x >= bMin.x) &&
                   (aMin.y <= bMax.y && aMax.y >= bMin.y) &&
                   (aMin.z <= bMax.z && aMax.z >= bMin.z);
        }
        else if(type == ColliderType::OBB){
            const OBBCollider& obb = *static_cast<OBBCollider*>(collider.get());
            glm::mat3 o = obb.orientation;
            glm::vec3 test[15] = {
                glm::vec3(1, 0, 0), // AABB axis 1
                glm::vec3(0, 1, 0), // AABB axis 2
                glm::vec3(0, 0, 1), // AABB axis 3
                o[0], // OBB axis 1
                o[1], // OBB axis 2
                o[2] // OBB axis 3
            };
            for(int i =0; i<3; i++){
                test[6 + i * 3 + 0] = glm::cross(test[i], test[0]);
                test[6 + i * 3 + 1] = glm::cross(test[i], test[1]);
                test[6 + i * 3 + 2] = glm::cross(test[i], test[2]);
            }
            for(int i = 0; i < 15; ++i) {
                if(!OverlapOnAxis(aabb, obb, test[i])) {
                    return false;
                }
            }
            return true;
        }
        else if(type == ColliderType::PLANE){
            const PlaneCollider& plane = *static_cast<PlaneCollider*>(collider.get());
            float pLen = aabb.size.x * fabsf(glm::dot(plane.normal, glm::vec3(1,0,0))) +
                          aabb.size.y * fabsf(glm::dot(plane.normal, glm::vec3(0,1,0))) +
                          aabb.size.z * fabsf(glm::dot(plane.normal, glm::vec3(0,0,1)));
            float dot = glm::dot(plane.normal, aabb.position);
            float dist = dot - plane.distance;
            return fabsf(dist) <= pLen;
        }
        else if (type == ColliderType::MESH) {
            // Mesh collision detection not implemented
            return false;
        }
        return false;
    }
    bool intersectColliders(const OBBCollider& obb) const {
        if(type == ColliderType::SPHERE) {
            SphereCollider sphere = *static_cast<SphereCollider*>(collider.get());
            glm::vec3 closestPoint = obb.ClosestPoint(sphere.position);
            float distSq = MagnitudeSq(closestPoint - sphere.position);
            return distSq <= (sphere.radius * sphere.radius);
        }
        else if(type == ColliderType::AABB){
            const AABBCollider& aabb = *static_cast<AABBCollider*>(collider.get());
            glm::mat3 o = obb.orientation;
            glm::vec3 test[15] = {
                glm::vec3(1, 0, 0), // AABB axis 1
                glm::vec3(0, 1, 0), // AABB axis 2
                glm::vec3(0, 0, 1), // AABB axis 3
                o[0], // OBB axis 1
                o[1], // OBB axis 2
                o[2] // OBB axis 3
            };
            for(int i =0; i<3; i++){
                test[6 + i * 3 + 0] = glm::cross(test[i], test[0]);
                test[6 + i * 3 + 1] = glm::cross(test[i], test[1]);
                test[6 + i * 3 + 2] = glm::cross(test[i], test[2]);
            }
            for(int i = 0; i < 15; ++i) {
                if(!OverlapOnAxis(aabb, obb, test[i])) {
                    return false;
                }
            }
            return true;

        }
        else if(type == ColliderType::OBB){
            const OBBCollider& obb1 = *static_cast<OBBCollider*>(collider.get());
            glm::mat3 o = obb1.orientation;
            glm::mat3 o1 = obb.orientation;
            glm::vec3 test[15] = {
                o1[0], // OBB1 axis 1
                o1[1], // OBB1 axis 2
                o1[2], // OBB1 axis 3
                o[0], // OBB2 axis 1
                o[1], // OBB2 axis 2
                o[2] // OBB2 axis 3
            };
            for(int i =0; i<3; i++){
                test[6 + i * 3 + 0] = glm::cross(test[i], test[0]);
                test[6 + i * 3 + 1] = glm::cross(test[i], test[1]);
                test[6 + i * 3 + 2] = glm::cross(test[i], test[2]);
            }
            for(int i = 0; i < 15; ++i) {
                if(!OverlapOnAxis(obb1, obb, test[i])) {
                    return false;
                }
            }
            return true;
        }
        else if(type == ColliderType::PLANE){
            const PlaneCollider& plane = *static_cast<PlaneCollider*>(collider.get());
            const glm::mat3& o = obb.orientation;
            glm::vec3 rot[3] = {
                o[0], // OBB axis 1
                o[1], // OBB axis 2
                o[2] // OBB axis 3
            };
            float pLen = obb.size.x * fabsf(glm::dot(plane.normal, rot[0])) +
                          obb.size.y * fabsf(glm::dot(plane.normal, rot[1])) +
                          obb.size.z * fabsf(glm::dot(plane.normal, rot[2]));
            float dot = glm::dot(plane.normal, obb.position);
            float dist = dot - plane.distance;
            return fabsf(dist) <= pLen;
        }
        else if (type == ColliderType::MESH) {
            // Mesh collision detection not implemented
            return false;
        }
        return false;
    }
    bool intersectColliders(const PlaneCollider& plane) const {
        if(type == ColliderType::SPHERE) {
            std::cout<<"Plane-Sphere"<<std::endl;
            SphereCollider sphere = *static_cast<SphereCollider*>(collider.get());
            std::cout<<"Sphere Position: "<<sphere.position.x<<","<<sphere.position.y<<","<<sphere.position.z<<std::endl;
            glm::vec3 closestPoint = sphere.position - static_cast<PlaneCollider*>(collider.get())->ClosestPoint(sphere.position);
            float distSq = MagnitudeSq(sphere.position - closestPoint);
            return distSq <= (sphere.radius * sphere.radius);
            
        }
        else if(type == ColliderType::AABB){
            std::cout<<"Plane-AABB"<<std::endl;
            const AABBCollider& aabb = *static_cast<AABBCollider*>(collider.get());
            std::cout<<"AABB Position: "<<aabb.position.x<<","<<aabb.position.y<<","<<aabb.position.z<<std::endl;
            float pLen = aabb.size.x * fabsf(glm::dot(plane.normal, glm::vec3(1,0,0))) +
                          aabb.size.y * fabsf(glm::dot(plane.normal, glm::vec3(0,1,0))) +
                          aabb.size.z * fabsf(glm::dot(plane.normal, glm::vec3(0,0,1)));
            float dot = glm::dot(plane.normal, aabb.position);
            float dist = dot - plane.distance;
            return fabsf(dist) <= pLen;
        }
        else if(type == ColliderType::OBB){
            const OBBCollider& obb = *static_cast<OBBCollider*>(collider.get());
            const glm::mat3& o = obb.orientation;
            glm::vec3 rot[3] = {
                o[0], // OBB axis 1
                o[1], // OBB axis 2
                o[2] // OBB axis 3
            };
            float pLen = obb.size.x * fabsf(glm::dot(plane.normal, rot[0])) +
                          obb.size.y * fabsf(glm::dot(plane.normal, rot[1])) +
                          obb.size.z * fabsf(glm::dot(plane.normal, rot[2]));
            float dot = glm::dot(plane.normal, obb.position);
            float dist = dot - plane.distance;
            return fabsf(dist) <= pLen;
        }
        else if(type == ColliderType::PLANE){
            const PlaneCollider& otherPlane = *static_cast<PlaneCollider*>(collider.get());
            glm::vec3 d = glm::cross(plane.normal, otherPlane.normal);
            return glm::dot(d, d) != 0.0f;
        }
        else if (type == ColliderType::MESH) {
            // Mesh collision detection not implemented
            return false;
        }
        return false;
    }

    ~ColliderComponent() = default;
};

#endif // COLLIDER_COMPONENT_H
