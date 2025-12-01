#ifndef _H_GEOMETRY_3D_
#define _H_GEOMETRY_3D_
#include "matrices.h"
#include "vectors.h"
#include <list>

// ------ BASE 3D TYPES ------
struct BaseCollider{
    virtual ~BaseCollider() {}
};

// -------- Point --------
typedef glm::vec3 PointCollider;

// -------- Line --------
typedef struct LineCollider{
    PointCollider start;
    PointCollider end;

    inline LineCollider() {}
    inline LineCollider(const PointCollider& s, const PointCollider& e) : start(s), end(e) {}
} LineCollider;
float Length(const LineCollider& line) {
    return Magnitude(line.start - line.end);
}
float LengthSq(const LineCollider& line) {
    return MagnitudeSq(line.start - line.end);
}

// -------- Ray --------
typedef struct RayCollider{
    PointCollider origin;
    vec3 direction;
    inline RayCollider() : direction(0.0f,0.0f,1.0f) {}
    inline RayCollider(const PointCollider& o, const vec3& d) : origin(o), direction(d) {NormalizedDirection();};
    inline void NormalizedDirection() {
        Normalized(direction);
    }
} RayCollider;
RayCollider FromPoints(const PointCollider& from, const PointCollider& to){
    return RayCollider(from, Normalized(to - from));
}

// -------- Shapes --------
// -------- Sphere --------
typedef struct SphereCollider : BaseCollider {
    PointCollider position;
    float radius;
    inline SphereCollider() : radius(1.0f) {}
    inline SphereCollider(const PointCollider& p, float r) : position(p), radius(r) {}
} SphereCollider;

// -------- Axis Aligned Bounding Box --------
typedef struct AABBCollider : BaseCollider {
    PointCollider origin;
    vec3 size;
    inline AABBCollider() : size(1.0f, 1.0f, 1.0f) {}
    inline AABBCollider(const PointCollider& o, const vec3& s) : origin(o), size(s) {}
} AABBCollider;
vec3 GetMin(const AABBCollider& aabb) {
    vec3 p1 = aabb.origin + aabb.size;
    vec3 p2 = aabb.origin - aabb.size;
    return vec3(fminf(p1.x, p2.x), fminf(p1.y, p2.y), fminf(p1.z, p2.z));
}
vec3 GetMax(const AABBCollider& aabb) {
    vec3 p1 = aabb.origin + aabb.size;
    vec3 p2 = aabb.origin - aabb.size;
    return vec3(fmaxf(p1.x, p2.x), fmaxf(p1.y, p2.y), fmaxf(p1.z, p2.z));
}
AABBCollider FromMinMax(const vec3& min, const vec3& max) {
    return AABBCollider((min + max) * 0.5f, (max - min) * 0.5f);
}

// -------- Oriented Bounding Box --------
typedef struct OBBCollider : BaseCollider {
    PointCollider position;
    vec3 size;
    mat3 oritentation;
    inline OBBCollider() : size(1.0f, 1.0f, 1.0f), oritentation(1.0f) {}
    inline OBBCollider(const PointCollider& p, const vec3& s) : position(p), size(s), oritentation(1.0f) {}
    inline OBBCollider(const PointCollider& p, const vec3& s, const mat3& o) : position(p), size(s), oritentation(o) {}
} OBBCollider;

// -------- Plane --------
typedef struct PlaneCollider : BaseCollider {
    vec3 normal;
    float distance;

    inline PlaneCollider() : normal(0.0f, 1.0f, 0.0f), distance(0.0f) {}
    inline PlaneCollider(const vec3& n, float d) : normal(n), distance(d) {}
} PlaneCollider;
float PlaneEquation(const PointCollider& point, const PlaneCollider& plane){
    return glm::dot(point, plane.normal) - plane.distance;
}

// -------- Triangle --------
typedef struct Triangle{
    union {
        struct {
            PointCollider a;
            PointCollider b;
            PointCollider c;
        };
        PointCollider points[3];
        float values[9];
    };
    inline Triangle() {}
    inline Triangle(const PointCollider& p1, const PointCollider& p2, const PointCollider& p3) : a(p1), b(p2), c(p3) {}
} Triangle;

// -------- 3D Point Tests --------
// Sphere
bool PointInSphere(const PointCollider& point, const SphereCollider& sphere){
    float magSq = MagnitudeSq(point - sphere.position);
    float radiusSq = sphere.radius * sphere.radius;
    return magSq < radiusSq;
}
PointCollider ClosestPoint(const SphereCollider& sphere, const PointCollider& point){
    vec3 sphereToPoint = point - sphere.position;
    Normalize(sphereToPoint);
    sphereToPoint = sphereToPoint * sphere.radius;
    return sphere.position + sphereToPoint;
}

// AABB
bool PointInAABB(const PointCollider& point, const AABBCollider& aabb){
    PointCollider min = GetMin(aabb);
    PointCollider max = GetMax(aabb);
    if (point.x<min.x || point.y<min.y || point.z<min.z) {
        return false;
    }
    if (point.x>max.x || point.y>max.y || point.z>max.z) {
        return false;
    }
    return true;
}
PointCollider ClosestPoint(const AABBCollider& aabb, const PointCollider& point){
    PointCollider result = point;
    PointCollider min = GetMin(aabb);
    PointCollider max = GetMax(aabb);
    result.x = (result.x<min.x) ? min.x : result.x;
    result.y = (result.y<min.x) ? min.y : result.y;
    result.z = (result.z<min.x) ? min.z : result.z;
    result.x = (result.x>max.x) ? max.x : result.x;
    result.y = (result.y>max.x) ? max.y : result.y;
    result.z = (result.z>max.x) ? max.z : result.z;
    return result;
}

// OBB
bool PointInOBB(const PointCollider& point, const OBBCollider& obb){
    vec3 dir = point - obb.position;
    for(int i = 0; i < 3; i++){
        vec3 axis = vec3(obb.oritentation[0][i], obb.oritentation[1][i], obb.oritentation[2][i]);
        float distance = glm::dot(dir, axis);
        if (distance > obb.size[i] || distance < -obb.size[i]) {
            return false;
        }
    }
    return true;
}
PointCollider ClosestPoint(const OBBCollider& obb, const PointCollider& point){
    vec3 dir = point - obb.position;
    PointCollider result = obb.position;
    for(int i = 0; i < 3; i++){
        vec3 axis = vec3(obb.oritentation[0][i], obb.oritentation[1][i], obb.oritentation[2][i]);
        float distance = glm::dot(dir, axis);
        if (distance > obb.size[i]) {
            distance = obb.size[i];
        }
        if (distance < -obb.size[i]) {
            distance = -obb.size[i];
        }
        result = result + axis * distance;
    }
    return result;
}

// Plane
bool PointOnPlane(const PointCollider& point, const PlaneCollider& plane){
    float dot = glm::dot(point, plane.normal);
    return dot - plane.distance == 0.0f;
}
PointCollider ClosestPoint(const PlaneCollider& plane, const PointCollider& point){
    float dot = glm::dot(point, plane.normal);
    float distance = dot - plane.distance;
    return point - plane.normal * distance;
}

// Line
PointCollider ClosestPoint(const LineCollider& line, const PointCollider& point){
    vec3 lVec = line.end - line.start;
    float t = glm::dot(point - line.start, lVec) / glm::dot(lVec, lVec);
    t = fmaxf(t, 0.0f); // Clamp to 0
    t = fminf(t, 1.0f); // Clamp t
    return line.start + lVec * t;
}
bool PointOnLine(const PointCollider& point, const LineCollider& line){
    PointCollider closest = ClosestPoint(line, point);
    float distSq = MagnitudeSq(closest - point);
    return distSq == 0.0f;
    vec3 ap = point - line.start;
}

// Ray
bool PointOnRay(const PointCollider& point, const RayCollider& ray){
    if(point == ray.origin){
        return true;
    }
    vec3 norm = point - ray.origin;
    Normalize(norm);
    float diff = glm::dot(norm, ray.direction);
    return CMP(diff, 1.0f);
}
PointCollider ClosestPoint(const RayCollider& ray, const PointCollider& point){
    float t = glm::dot(point - ray.origin, ray.direction);
    t = fmaxf(t, 0.0f); // Clamp to 0
    return PointCollider(ray.origin + ray.direction * t);
}
// ------ 3D Shape Intersections ------
// Sphere-Sphere
bool SphereSphere(const SphereCollider& a, const SphereCollider& b){
    float radiusSum = a.radius + b.radius;
    float distSq = MagnitudeSq(a.position - b.position);
    return distSq < (radiusSum * radiusSum);
}
// Sphere-AABB
bool SphereAABB(const SphereCollider& sphere, const AABBCollider& aabb){
    PointCollider closest = ClosestPoint(aabb, sphere.position);
    float distSq = MagnitudeSq(sphere.position - closest);
    return distSq < (sphere.radius * sphere.radius);
}
#define AABBSphere(aabb, sphere) SphereAABB(sphere, aabb)

// Sphere-OBB
bool SphereOBB(const SphereCollider& sphere, const OBBCollider& obb){
    PointCollider closest = ClosestPoint(obb, sphere.position);
    float distSq = MagnitudeSq(sphere.position - closest);
    return distSq < (sphere.radius * sphere.radius);
}
#define OBBSphere(obb, sphere) SphereOBB(sphere, obb)

//Sphere-Plane
bool SpherePlane(const SphereCollider& sphere, const PlaneCollider& plane){
    PointCollider closest = ClosestPoint(plane, sphere.position);
    float distSq = MagnitudeSq(sphere.position - closest);
    float radiusSq = sphere.radius * sphere.radius;
    return distSq < radiusSq;
}
#define PlaneSphere(plane, sphere) SpherePlane(sphere, plane)

// AABB-AABB
bool AABBAABB(const AABBCollider& a, const AABBCollider& b){
    vec3 aMin = GetMin(a);
    vec3 aMax = GetMax(a);
    vec3 bMin = GetMin(b);
    vec3 bMax = GetMax(b);
    return (aMin.x <= bMax.x && bMin.x <= aMax.x) &&
           (aMin.y <= bMax.y && bMin.y <= aMax.y) &&
           (aMin.z <= bMax.z && bMin.z <= aMax.z);
}

//AABB-OBB
typedef struct Interval{
    float min;
    float max;
} Interval;
Interval GetInterval(const AABBCollider& aabb, const vec3& axis){
    vec3 i = GetMin(aabb);
    vec3 a = GetMax(aabb);
    vec3 verts[] = {
        vec3(i.x, a.y, a.z),
        vec3(i.x, a.y, i.z),
        vec3(i.x, i.y, a.z),
        vec3(i.x, i.y, i.z),
        vec3(a.x, a.y, a.z),
        vec3(a.x, a.y, i.z),
        vec3(a.x, i.y, a.z),
        vec3(a.x, i.y, i.z)
    };
    Interval result;
    result.min = result.max = glm::dot(axis, verts[0]);
    for(int i = 1; i < 8; i++){
        float projection = glm::dot(axis, verts[i]);
        result.min = (projection<result.min)?projection :result.min;
        result.max = (projection>result.max)?projection :result.max;
    }   
    return result;
}
Interval GetInterval(const OBBCollider& obb, const vec3& axis){
    vec3 vertex[8];
    vec3 C = obb.position;
    vec3 E = obb.size;
    vec3 A[] = {
        vec3(obb.oritentation[0][0], obb.oritentation[1][0], obb.oritentation[2][0]),
        vec3(obb.oritentation[0][1], obb.oritentation[1][1], obb.oritentation[2][1]),
        vec3(obb.oritentation[0][2], obb.oritentation[1][2], obb.oritentation[2][2])
    };
    vertex[0] = C + A[0] * E.x + A[1] * E.y + A[2] * E.z;
    vertex[1] = C + A[0] * E.x + A[1] * E.y - A[2] * E.z;
    vertex[2] = C + A[0] * E.x - A[1] * E.y + A[2] * E.z;
    vertex[3] = C + A[0] * E.x - A[1] * E.y - A[2] * E.z;
    vertex[4] = C - A[0] * E.x + A[1] * E.y + A[2] * E.z;
    vertex[5] = C - A[0] * E.x + A[1] * E.y - A[2] * E.z;
    vertex[6] = C - A[0] * E.x - A[1] * E.y + A[2] * E.z;
    vertex[7] = C - A[0] * E.x - A[1] * E.y - A[2] * E.z;
    Interval result;
    result.min = result.max = glm::dot(axis, vertex[0]);
    for(int i = 1; i < 8; i++){
        float projection = glm::dot(axis, vertex[i]);
        result.min = (projection<result.min)?projection :result.min;
        result.max = (projection>result.max)?projection :result.max;
    }
    return result;
}
bool OverlapOnAxis(const AABBCollider& aabb, const OBBCollider& obb, const vec3& axis){
    Interval a = GetInterval(aabb, axis);
    Interval b = GetInterval(obb, axis);
    return ((b.min <= a.max) && (a.min <= b.max));
}
bool AABBOBB(const AABBCollider& aabb, const OBBCollider& obb){
    vec3 test[15] = {
        vec3(1.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        vec3(0.0f, 0.0f, 1.0f),
        vec3(obb.oritentation[0][0], obb.oritentation[1][0], obb.oritentation[2][0]),
        vec3(obb.oritentation[0][1], obb.oritentation[1][1], obb.oritentation[2][1]),
        vec3(obb.oritentation[0][2], obb.oritentation[1][2], obb.oritentation[2][2]),
    };
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            test[6 + i * 3 + j] = glm::cross(vec3(test[i].x, test[i].y, test[i].z), vec3(test[3 + j].x, test[3 + j].y, test[3 + j].z));
        }
    }
    for(int i = 0; i < 15; i++){
        if(!OverlapOnAxis(aabb, obb, test[i])){
            return false;
        }
    }
    return true;
}
#define OBBAABB(obb, aabb) AABBOBB(aabb, obb)
//AABB-Plane
bool AABBPlane(const AABBCollider& aabb, const PlaneCollider& plane){
    float pLen = aabb.size.x * fabs(plane.normal.x) +
                 aabb.size.y * fabs(plane.normal.y) +
                 aabb.size.z * fabs(plane.normal.z);
    float dot = glm::dot(aabb.origin, plane.normal);
    float dist = dot - plane.distance;
    return fabs(dist) <= pLen;
}
#define PlaneAABB(plane, aabb) AABBPlane(aabb, plane)
//OBB-OBB
bool OverlapOnAxis(const OBBCollider& obb1, const OBBCollider& obb2, const vec3& axis){
    Interval a = GetInterval(obb1, axis);
    Interval b = GetInterval(obb2, axis);
    return ((b.min <= a.max) && (a.min <= b.max));
}
bool OBBOBB(const OBBCollider& obb1, const OBBCollider& obb2){
    vec3 test[15] = {
        vec3(obb1.oritentation[0][0], obb1.oritentation[1][0], obb1.oritentation[2][0]),
        vec3(obb1.oritentation[0][1], obb1.oritentation[1][1], obb1.oritentation[2][1]),
        vec3(obb1.oritentation[0][2], obb1.oritentation[1][2], obb1.oritentation[2][2]),
        vec3(obb2.oritentation[0][0], obb2.oritentation[1][0], obb2.oritentation[2][0]),
        vec3(obb2.oritentation[0][1], obb2.oritentation[1][1], obb2.oritentation[2][1]),
        vec3(obb2.oritentation[0][2], obb2.oritentation[1][2], obb2.oritentation[2][2]),
    };
    for(int i = 0; i < 3; i++){
        for(int j = 0; j < 3; j++){
            test[6 + i * 3 + j] = glm::cross(vec3(test[i].x, test[i].y, test[i].z), vec3(test[3 + j].x, test[3 + j].y, test[3 + j].z));
        }
    }
    for(int i = 0; i < 15; i++){
        if(!OverlapOnAxis(obb1, obb2, test[i])){
            return false;
        }
    }
    return true;
}
//OBB-Plane
bool OBBPlane(const OBBCollider& obb, const PlaneCollider& plane){
    vec3 rot[] = {
        vec3(obb.oritentation[0][0], obb.oritentation[1][0], obb.oritentation[2][0]),
        vec3(obb.oritentation[0][1], obb.oritentation[1][1], obb.oritentation[2][1]),
        vec3(obb.oritentation[0][2], obb.oritentation[1][2], obb.oritentation[2][2])
    };
    vec3 normal = plane.normal;
    float pLen = obb.size.x * fabs(glm::dot(normal, rot[0])) +
                 obb.size.y * fabs(glm::dot(normal, rot[1])) +
                 obb.size.z * fabs(glm::dot(normal, rot[2]));
    float dot = glm::dot(obb.position, plane.normal);
    float dist = dot - plane.distance;
    return fabs(dist) <= pLen;
}
#define PlaneOBB(plane, obb) OBBPlane(obb, plane)
// Plane-Plane
bool PlanePlane(const PlaneCollider& a, const PlaneCollider& b){
    vec3 cross = glm::cross(a.normal, b.normal);
    return glm::dot(cross, cross) != 0.0f;
}
// ------ 3D Line Intersections ------
// Raycast Sphere
float Raycast(const SphereCollider& sphere, const RayCollider& ray){
    vec3 e = sphere.position - ray.origin;
    float rSq = sphere.radius * sphere.radius;
    float eSq = MagnitudeSq(e);
    float a = glm::dot(e, ray.direction);
    float bSq = eSq - a * a;
    float f = sqrt(rSq - bSq);
    if(rSq  - (eSq - a * a) < 0.0f){
        return -1.0f;
    } else if(eSq < rSq){
        return a + f;
    } else {
        return a - f;
    }
}
// Raycast AABB
float Raycast(const AABBCollider& aabb, const RayCollider& ray){
    vec3 min = GetMin(aabb);
    vec3 max = GetMax(aabb);
    float t1 = (min.x - ray.origin.x) / ray.direction.x;
    float t2 = (max.x - ray.origin.x) / ray.direction.x;
    float t3 = (min.y - ray.origin.y) / ray.direction.y;
    float t4 = (max.y - ray.origin.y) / ray.direction.y;
    float t5 = (min.z - ray.origin.z) / ray.direction.z;
    float t6 = (max.z - ray.origin.z) / ray.direction.z;
    float tmin = fmaxf(fmaxf(fminf(t1, t2), fminf(t3, t4)), fminf(t5, t6));
    float tmax = fminf(fminf(fmaxf(t1, t2), fmaxf(t3, t4)), fmaxf(t5, t6));
    if(tmax < 0.0f || tmin > tmax){
        return -1.0f;
    }
    if(tmin < 0.0f){
        return tmax;
    }
    return tmin;
}
// Raycast OBB
float Raycast(const OBBCollider& obb, const RayCollider& ray){
    vec3 X = vec3(obb.oritentation[0][0], obb.oritentation[1][0], obb.oritentation[2][0]);
    vec3 Y = vec3(obb.oritentation[0][1], obb.oritentation[1][1], obb.oritentation[2][1]);
    vec3 Z = vec3(obb.oritentation[0][2], obb.oritentation[1][2], obb.oritentation[2][2]);
    vec3 p = obb.position - ray.origin;
    vec3 f = vec3(glm::dot(X, ray.direction), glm::dot(Y, ray.direction), glm::dot(Z, ray.direction));
    vec3 e = vec3(glm::dot(X, p), glm::dot(Y, p), glm::dot(Z, p));
    float t[6] = {0, 0, 0, 0, 0, 0};
    for(int i = 0; i < 3; i++){
        if(CMP(f[i], 0.0f)){
            if(-e[i] - obb.size[i] > 0.0f || -e[i] + obb.size[i] < 0.0f){
                return -1.0f;
            }
            f[i] = 0.00001f; // Avoid division by zero
        }
        t[i * 2 + 0] = ( e[i] + obb.size[i]) / f[i]; // t1
        t[i * 2 + 1] = ( e[i] - obb.size[i]) / f[i]; // t2
    }
    float tmin = fmaxf(fmaxf(fminf(t[0], t[1]), fminf(t[2], t[3])), fminf(t[4], t[5]));
    float tmax = fminf(fminf(fmaxf(t[0], t[1]), fmaxf(t[2], t[3])), fmaxf(t[4], t[5]));
    if(tmax < 0.0f || tmin > tmax){
        return -1.0f;
    }
    if(tmin < 0.0f){
        return tmax;
    }
    return tmin;
}

// Raycast Plane
float Raycast(const PlaneCollider& plane, const RayCollider& ray){
    float nd = glm::dot(ray.direction, plane.normal);
    float pn = glm::dot(ray.origin, plane.normal);
    if(CMP(nd, 0.0f)){
        return -1.0f;
    }
    float t = (plane.distance - pn) / nd;
    return (t >= 0.0f) ? t : -1.0f;
}

// -- Line tests --
//Sphere
bool Linetest(const SphereCollider& sphere, const LineCollider& line){
    PointCollider closest = ClosestPoint(line, sphere.position);
    float distSq = MagnitudeSq(sphere.position - closest);
    return distSq <= (sphere.radius * sphere.radius);
}

// AABB
bool Linetest(const AABBCollider& aabb, const LineCollider& line){
    RayCollider ray(line.start, line.end - line.start);
    ray.NormalizedDirection();
    float t = Raycast(aabb, ray);
    return (t >= 0.0f && t * t <= LengthSq(line));
}

// OBB
bool Linetest(const OBBCollider& obb, const LineCollider& line){
    RayCollider ray(line.start, line.end - line.start);
    ray.NormalizedDirection();
    float t = Raycast(obb, ray);
    return (t >= 0.0f && t * t <= LengthSq(line));
}

// Plane
bool Linetest(const PlaneCollider& plane, const LineCollider& line){
    vec3 ab = line.end - line.start;
    float nA = glm::dot(plane.normal, line.start);
    float nAB = glm::dot(plane.normal, ab);
    float t = (plane.distance - nA) / nAB;
    return (t >= 0.0f && t <= 1.0f);
}

// ------ Triangles and Meshes ------
bool PointInTriangles(const PointCollider& p, const Triangle& t){
    vec3 a = t.a - p;
    vec3 b = t.b - p;
    vec3 c = t.c - p;
    vec3 normPBC = glm::cross(b, c);
    vec3 normPCA = glm::cross(c, a);
    vec3 normPAB = glm::cross(a, b);
    if(glm::dot(normPBC, normPCA) < 0.0f) return false;
    if(glm::dot(normPBC, normPAB) < 0.0f) return false;
    return true;
}
PlaneCollider FromTriangle(const Triangle& t){
    PlaneCollider result;
    result.normal = Normalized(glm::cross(t.b - t.a, t.c - t.a));
    result.distance = glm::dot(result.normal, t.a);
    return result;
}
PointCollider ClosestPoint(const Triangle& t, const PointCollider& p){
    PlaneCollider plane = FromTriangle(t);
    PointCollider closest = ClosestPoint(plane, p);
    if(PointInTriangles(closest, t)){
        return closest;
    }
    PointCollider c1 = ClosestPoint(LineCollider(t.a, t.b), p);
    PointCollider c2 = ClosestPoint(LineCollider(t.b, t.c), p);
    PointCollider c3 = ClosestPoint(LineCollider(t.c, t.a), p);
    float magSq1 = MagnitudeSq(p - c1);
    float magSq2 = MagnitudeSq(p - c2);
    float magSq3 = MagnitudeSq(p - c3);
    if(magSq1 < magSq2 && magSq1 < magSq3){
        return c1;
    } else if(magSq2 < magSq3){
        return c2;
    } else {
        return c3;
    }
}
// Triangle-Sphere
bool TriangleSphere(const Triangle& t, const SphereCollider& sphere){
    PointCollider closest = ClosestPoint(t, sphere.position);
    float distSq = MagnitudeSq(sphere.position - closest);
    return distSq <= (sphere.radius * sphere.radius);
}
// Triangle-AABB
Interval GetInterval(const Triangle& tri, const vec3& axis){
    Interval result;
    result.min = result.max = glm::dot(axis, tri.points[0]);
    for(int i = 1; i < 3; i++){
        float projection = glm::dot(axis, tri.points[i]);
        result.min = (projection<result.min)?projection :result.min;
        result.max = (projection>result.max)?projection :result.max;
    }   
    return result;
}
bool OverlapOnAxis(const AABBCollider& aabb, const Triangle& tri, const vec3& axis){
    Interval a = GetInterval(aabb, axis);
    Interval b = GetInterval(tri, axis);
    return ((b.min <= a.max) && (a.min <= b.max));
}
bool TriangleAABB(const Triangle& t, const AABBCollider& a){
    vec3 f0 = t.b - t.a;
    vec3 f1 = t.c - t.b;
    vec3 f2 = t.a - t.c;
    vec3 u0 = vec3(1.0f, 0.0f, 0.0f);
    vec3 u1 = vec3(0.0f, 1.0f, 0.0f);
    vec3 u2 = vec3(0.0f, 0.0f, 1.0f);
    vec3 test[13] = {
        u0, u1, u2,
        glm::cross(f0, f1),
        glm::cross(u0, f0), glm::cross(u0, f1), glm::cross(u0, f2),
        glm::cross(u1, f0), glm::cross(u1, f1), glm::cross(u1, f2),
        glm::cross(u2, f0), glm::cross(u2, f1), glm::cross(u2, f2)
    };
    for(int i = 0; i < 13; i++){
        if(!OverlapOnAxis(a, t, test[i])){
            return false;
        }
    }
    return true;
}
#define AABBTriangle(aabb, tri) TriangleAABB(tri, aabb)
// Triangle-OBB
bool OverlapOnAxis(const OBBCollider& obb, const Triangle& tri, const vec3& axis){
    Interval a = GetInterval(obb, axis);
    Interval b = GetInterval(tri, axis);
    return ((b.min <= a.max) && (a.min <= b.max));
}
bool TriangleOBB(const Triangle& t, const OBBCollider& o){
    vec3 f0 = t.b - t.a;
    vec3 f1 = t.c - t.b;
    vec3 f2 = t.a - t.c;
    vec3 u0 = vec3(o.oritentation[0][0], o.oritentation[1][0], o.oritentation[2][0]);
    vec3 u1 = vec3(o.oritentation[0][1], o.oritentation[1][1], o.oritentation[2][1]);
    vec3 u2 = vec3(o.oritentation[0][2], o.oritentation[1][2], o.oritentation[2][2]);
    vec3 test[13] = {
        u0, u1, u2,
        glm::cross(f0, f1),
        glm::cross(u0, f0), glm::cross(u0, f1), glm::cross(u0, f2),
        glm::cross(u1, f0), glm::cross(u1, f1), glm::cross(u1, f2),
        glm::cross(u2, f0), glm::cross(u2, f1), glm::cross(u2, f2)
    };
    for(int i = 0; i < 13; i++){
        if(!OverlapOnAxis(o, t, test[i])){
            return false;
        }
    }
    return true;
}
#define OBBTriangle(obb, tri) TriangleOBB(tri, obb)
// Triangle-Plane
bool TrianglePlane(const Triangle& t, const PlaneCollider& p){
    float side1 = PlaneEquation(t.a, p);
    float side2 = PlaneEquation(t.b, p);
    float side3 = PlaneEquation(t.c, p);
    if(CMP(side1, 0.0f) && CMP(side2, 0.0f) && CMP(side3, 0.0f)){
        return true;
    }
    if((side1 > 0.0f && side2 > 0.0f && side3 > 0.0f) ||
       (side1 < 0.0f && side2 < 0.0f && side3 < 0.0f)){
        return false;
    }
    return true;
}
#define PlaneTriangle(plane, tri) TrianglePlane(tri, plane)
// Triangle-Triangle
bool OverlapOnAxis(const Triangle& tri1, const Triangle& tri2, const vec3& axis){
    Interval a = GetInterval(tri1, axis);
    Interval b = GetInterval(tri2, axis);
    return ((b.min <= a.max) && (a.min <= b.max));
}
bool TriangleTriangle(const Triangle& t1, const Triangle& t2){
    vec3 t1_f0 = t1.b - t1.a;
    vec3 t1_f1 = t1.c - t1.b;
    vec3 t1_f2 = t1.a - t1.c;
    vec3 t2_f0 = t2.b - t2.a;
    vec3 t2_f1 = t2.c - t2.b;
    vec3 t2_f2 = t2.a - t2.c;
    vec3 axisToTest[] = {
        glm::cross(t1_f0, t1_f1),
        glm::cross(t2_f0, t2_f1),
        glm::cross(t2_f0, t1_f0), glm::cross(t2_f0, t1_f1),
        glm::cross(t2_f0, t1_f2), glm::cross(t2_f1, t1_f0),
        glm::cross(t2_f1, t1_f1), glm::cross(t2_f1, t1_f2),
        glm::cross(t2_f2, t1_f0), glm::cross(t2_f2, t1_f1),
        glm::cross(t2_f2, t1_f2),
    };
    for(int i = 0; i < 11; i++){
        if(!OverlapOnAxis(t1, t2, axisToTest[i])){
            return false;
        }
    }
    return true;
}

// Raycast Triangle
vec3 Barycentric(const PointCollider& p, const Triangle& t){
    vec3 ap = p - t.a;
    vec3 bp = p - t.b;
    vec3 cp = p - t.c;
    vec3 ab = t.b - t.a;
    vec3 ac = t.c - t.a;
    vec3 bc = t.c - t.b;
    vec3 cb = t.b - t.c;
    vec3 ca = t.a - t.c;
    vec3 v = ab - Project(ab, cb);
    float a = 1.0f - glm::dot(v, ap) / glm::dot(v, ab);
    v = bc - Project(bc, ac);
    float b = 1.0f - glm::dot(v, bp) / glm::dot(v, bc);
    v = ca - Project(ca, ab);
    float c = 1.0f - glm::dot(v, cp) / glm::dot(v, ca);
    return vec3(a, b, c);
}
float Raycast(const Triangle& triangle, const RayCollider& ray){
    PlaneCollider plane = FromTriangle(triangle);
    float t = Raycast(plane, ray);
    if(t < 0.0f){
        return -1.0f;
    }
    PointCollider result = ray.origin + ray.direction * t;
    vec3 barycentric = Barycentric(result, triangle);
    if(barycentric.x >= 0.0f && barycentric.y >= 0.0f && barycentric.z >= 0.0f &&
       barycentric.x <= 1.0f && barycentric.y <= 1.0f && barycentric.z <= 1.0f){
        return t;
    }
    return -1.0f;
}
//Linetest Triangle
bool Linetest(const Triangle& triangle, const LineCollider& line){
    RayCollider ray(line.start, Normalized(line.end - line.start));
    float t = Raycast(triangle, ray);
    return (t >= 0.0f && t * t <= LengthSq(line));
}
//BVH
typedef struct BVHNode {
    AABBCollider bounds;
    BVHNode* children;
    int numTriangles;
    int* triangles;
    BVHNode() : children(nullptr), numTriangles(0), triangles(nullptr) {}
} BVHNode;
// Mesh Structure
typedef struct MeshCollider : BaseCollider {
    int numTriangles;
    Triangle* triangles;
    PointCollider* vertices;
    float* values;
    BVHNode* accelerator;
    MeshCollider() : numTriangles(0), triangles(nullptr), vertices(nullptr), values(nullptr), accelerator(nullptr) {}
} MeshCollider;
void SplitBVHNode(BVHNode* node, const MeshCollider& model, int depth){
    if(depth--==0){
        return;
    }
    if(node->children == 0){
        if(node->numTriangles >0){
            node->children = new BVHNode[8];
            vec3 c = node->bounds.origin;
            vec3 e = node->bounds.size*0.5f;
            for(int i = 0; i < 8; i++){
                vec3 offset = vec3((i&1)?e.x:-e.x, (i&2)?e.y:-e.y, (i&4)?e.z:-e.z);
                node->children[i].bounds = FromMinMax(c + offset - e*0.5f, c + offset + e*0.5f);
            }
        }
        if(node->children != 0 && node->numTriangles>0){
            for(int i =0; i< 8; i++){
                node->children[i].numTriangles = 0;
                for(int j=0;j<node->numTriangles;j++){
                    Triangle tri = model.triangles[node->triangles[j]];
                    if(AABBTriangle(node->children[i].bounds, tri)){
                        node->children[i].numTriangles++;
                    }
                }
                if(node->children[i].numTriangles==0){
                    continue;
                }
                node->children[i].triangles = new int[node->children[i].numTriangles];
                int index =0;
                for(int j=0;j<node->numTriangles;j++){
                    Triangle tri = model.triangles[node->triangles[j]];
                    if(AABBTriangle(node->children[i].bounds, tri)){
                        node->children[i].triangles[index++] = node->triangles[j];
                    }
                }
            }
            node->numTriangles =0; // Clear triangles from parent node
            delete[] node->triangles;
            node->triangles = nullptr;
            for(int i =0; i< 8; i++){
                SplitBVHNode(&node->children[i], model, depth);
            }
        }
    }
}
void AccelerateMesh(MeshCollider& mesh){
if(mesh.accelerator != nullptr){
        return; // Already accelerated
    }
    // Placeholder for BVH construction
    vec3 min = mesh.vertices[0];
    vec3 max = mesh.vertices[0];
    for(int i = 1; i < mesh.numTriangles * 3; i++){
        min.x = fminf(min.x, mesh.vertices[i].x);
        min.y = fminf(min.y, mesh.vertices[i].y);
        min.z = fminf(min.z, mesh.vertices[i].z);
        max.x = fmaxf(max.x, mesh.vertices[i].x);
        max.y = fmaxf(max.y, mesh.vertices[i].y);
        max.z = fmaxf(max.z, mesh.vertices[i].z);
    }
    mesh.accelerator = new BVHNode();
    mesh.accelerator->bounds = FromMinMax(min, max);
    mesh.accelerator->numTriangles = mesh.numTriangles;
    mesh.accelerator->triangles = new int[mesh.numTriangles];
    for(int i = 0; i < mesh.numTriangles; i++){
        mesh.accelerator->triangles[i] = i;
    }
    SplitBVHNode(mesh.accelerator, mesh, 3);
}

void FreeBVHNode(BVHNode* node){
    if(node->children != nullptr){
        for(int i = 0; i < 8; i++){
            FreeBVHNode(&node->children[i]);
        }
        delete[] node->children;
        node->children = nullptr;
    }
    if(node->triangles != nullptr || node->numTriangles != 0){
        delete[] node->triangles;
        node->triangles = nullptr;
        node->numTriangles = 0;
    }
}
void UpdateMeshTransform(MeshCollider& mesh, const glm::mat4& transform) {
    // Applique la transformation à chaque vertex
    for (int i = 0; i < mesh.numTriangles * 3; ++i) {
        mesh.vertices[i] = glm::vec3(transform * glm::vec4(mesh.vertices[i], 1.0f));
    }
    // Reconstruit les triangles
    for (int i = 0; i < mesh.numTriangles; ++i) {
        mesh.triangles[i].a = mesh.vertices[i * 3 + 0];
        mesh.triangles[i].b = mesh.vertices[i * 3 + 1];
        mesh.triangles[i].c = mesh.vertices[i * 3 + 2];
    }
    // Libère l'ancien BVH
    if (mesh.accelerator) {
        FreeBVHNode(mesh.accelerator);
        mesh.accelerator = nullptr;
    }
    // Reconstruit le BVH
    AccelerateMesh(mesh);
}
//Mesh operations
float MeshRay(const MeshCollider& mesh, const RayCollider& ray){
    if(mesh.accelerator == nullptr){
        for(int i = 0; i < mesh.numTriangles; i++){
            float t = Raycast(mesh.triangles[i], ray);
            if(t >= 0.0f){
                return t;
            }
        }
    } else {
        std::list<BVHNode*> toProcess;
        toProcess.push_front(mesh.accelerator);
        while(!toProcess.empty()){
            BVHNode* iterator = *(toProcess.begin());
            toProcess.erase(toProcess.begin());
            if(iterator->numTriangles >=0){
                for(int i = 0; i < iterator->numTriangles; i++){
                    float t = Raycast(mesh.triangles[iterator->triangles[i]], ray);
                    if(t >= 0.0f){
                        return t;
                    }
                }
            }
            if(iterator->children != nullptr){
                for(int i = 0; i < 8; i++){
                    float t = Raycast(iterator->children[i].bounds, ray);
                    if(t >= 0.0f){
                        toProcess.push_front(&iterator->children[i]);
                    }
                }
            }
        }   
    }
    return -1.0f;
}

bool MeshAABB(const MeshCollider& mesh, const AABBCollider& aabb){
    if(mesh.accelerator == nullptr){
        for(int i = 0; i < mesh.numTriangles; i++){
            if(TriangleAABB(mesh.triangles[i], aabb)){
                return true;
            }
        }
    } else {
        std::list<BVHNode*> toProcess;
        toProcess.push_front(mesh.accelerator);
        while(!toProcess.empty()){
            BVHNode* iterator = *(toProcess.begin());
            toProcess.erase(toProcess.begin());
            if(iterator->numTriangles >=0){
                for(int i = 0; i < iterator->numTriangles; i++){
                    if(TriangleAABB(mesh.triangles[iterator->triangles[i]], aabb)){
                        return true;
                    }
                }
            }
            if(iterator->children != nullptr){
                for(int i = 0; i < 8; i++){
                    if(AABBAABB(iterator->children[i].bounds, aabb)){
                        toProcess.push_front(&iterator->children[i]);
                    }
                }
            }
        }   
    }
    return false;
}
#define AABBMesh(aabb, mesh) MeshAABB(mesh, aabb)
bool SphereMesh(const SphereCollider& s, const MeshCollider& m){
    if(m.accelerator!=0){
        std::list<BVHNode*> toProcess;
        toProcess.push_front(m.accelerator);
        while(!toProcess.empty()){
            BVHNode* iterator = *(toProcess.begin());
            toProcess.erase(toProcess.begin());
            if(iterator->numTriangles >=0){
                for(int i = 0; i < iterator->numTriangles; i++){
                    if(TriangleSphere(m.triangles[iterator->triangles[i]], s)){
                        return true;
                    }
                }
            }
            if(iterator->children != nullptr){
                for(int i = 0; i < 8; i++){
                    if(SphereAABB(s, iterator->children[i].bounds)){
                        toProcess.push_front(&iterator->children[i]);
                    }
                }
            }
        }   
    } else {
        for(int i = 0; i < m.numTriangles; i++){
            if(TriangleSphere(m.triangles[i], s)){
                return true;
            }
        }
    }
}
#define MeshSphere(mesh, sphere) SphereMesh(sphere, mesh)
bool OBBMesh(const OBBCollider& obb, const MeshCollider& mesh){
    if(mesh.accelerator == nullptr){
        for(int i = 0; i < mesh.numTriangles; i++){
            if(TriangleOBB(mesh.triangles[i], obb)){
                return true;
            }
        }
    } else {
        std::list<BVHNode*> toProcess;
        toProcess.push_front(mesh.accelerator);
        while(!toProcess.empty()){
            BVHNode* iterator = *(toProcess.begin());
            toProcess.erase(toProcess.begin());
            if(iterator->numTriangles >=0){
                for(int i = 0; i < iterator->numTriangles; i++){
                    if(TriangleOBB(mesh.triangles[iterator->triangles[i]], obb)){
                        return true;
                    }
                }
            }
            if(iterator->children != nullptr){
                for(int i = 0; i < 8; i++){
                    if(OBBAABB(obb, iterator->children[i].bounds)){
                        toProcess.push_front(&iterator->children[i]);
                    }
                }
            }
        }   
    }
    return false;
}
#define MeshOBB(mesh, obb) OBBMesh(obb, mesh)
bool MeshPlane(const MeshCollider& mesh, const PlaneCollider& plane){
    if(mesh.accelerator == nullptr){
        for(int i = 0; i < mesh.numTriangles; i++){
            if(TrianglePlane(mesh.triangles[i], plane)){
                return true;
            }
        }
    } else {
        std::list<BVHNode*> toProcess;
        toProcess.push_front(mesh.accelerator);
        while(!toProcess.empty()){
            BVHNode* iterator = *(toProcess.begin());
            toProcess.erase(toProcess.begin());
            if(iterator->numTriangles >=0){
                for(int i = 0; i < iterator->numTriangles; i++){
                    if(TrianglePlane(mesh.triangles[iterator->triangles[i]], plane)){
                        return true;
                    }
                }
            }
            if(iterator->children != nullptr){
                for(int i = 0; i < 8; i++){
                    if(PlaneAABB(plane, iterator->children[i].bounds)){
                        toProcess.push_front(&iterator->children[i]);
                    }
                }
            }
        }   
    }
    return false;
}
#define PlaneMesh(plane, mesh) MeshPlane(mesh, plane)
bool MeshMesh(const MeshCollider& m1, const MeshCollider& m2){
    if(m1.accelerator != nullptr && m2.accelerator != nullptr){
        std::list<std::pair<BVHNode*, BVHNode*>> toProcess;
        toProcess.push_front(std::make_pair(m1.accelerator, m2.accelerator));
        while(!toProcess.empty()){
            std::pair<BVHNode*, BVHNode*> iterator = *(toProcess.begin());
            toProcess.erase(toProcess.begin());
            BVHNode* node1 = iterator.first;
            BVHNode* node2 = iterator.second;
            if(node1->numTriangles >=0 && node2->numTriangles >=0){
                for(int i = 0; i < node1->numTriangles; i++){
                    for(int j = 0; j < node2->numTriangles; j++){
                        if(TriangleTriangle(m1.triangles[node1->triangles[i]], m2.triangles[node2->triangles[j]])){
                            return true;
                        }
                    }
                }
            }
            if(node1->children != nullptr){
                for(int i = 0; i < 8; i++){
                    if(node2->children != nullptr){
                        for(int j = 0; j < 8; j++){
                            if(AABBAABB(node1->children[i].bounds, node2->children[j].bounds)){
                                toProcess.push_front(std::make_pair(&node1->children[i], &node2->children[j]));
                            }
                        }
                    }
                }
            }
        }
    } else if(m1.accelerator != nullptr) {
        std::list<BVHNode*> toProcess;
        toProcess.push_front(m1.accelerator);
        while(!toProcess.empty()){
            BVHNode* iterator = *(toProcess.begin());
            toProcess.erase(toProcess.begin());
            if(iterator->numTriangles >=0){
                for(int i = 0; i < iterator->numTriangles; i++){
                    for(int j = 0; j < m2.numTriangles; j++){
                        if(TriangleTriangle(m1.triangles[iterator->triangles[i]], m2.triangles[j])){
                            return true;
                        }
                    }
                }
            }
            if(iterator->children != nullptr){
                for(int i = 0; i < 8; i++){
                    toProcess.push_front(&iterator->children[i]);
                }
            }
        }
    } else if(m2.accelerator != nullptr){
        std::list<BVHNode*> toProcess;
        toProcess.push_front(m2.accelerator);
        while(!toProcess.empty()){
            BVHNode* iterator = *(toProcess.begin());
            toProcess.erase(toProcess.begin());
            if(iterator->numTriangles >=0){
                for(int i = 0; i < iterator->numTriangles; i++){
                    for(int j = 0; j < m1.numTriangles; j++){
                        if(TriangleTriangle(m2.triangles[iterator->triangles[i]], m1.triangles[j])){
                            return true;
                        }
                    }
                }
            }
            if(iterator->children != nullptr){
                for(int i = 0; i < 8; i++){
                    toProcess.push_front(&iterator->children[i]);
                }
            }
        }
    } else {
        for(int i = 0; i < m1.numTriangles; i++){
            for(int j = 0; j < m2.numTriangles; j++){
                if(TriangleTriangle(m1.triangles[i], m2.triangles[j])){
                    return true;
                }
            }
        }
    }
    return false;
}
#endif