// Description: Basic vector operations and definitions
#ifndef _H_MATH_VECTORS
#define _H_MATH_VECTORS

#include <cmath>
#include <cfloat>
#include <algorithm>
#include <glm/glm.hpp>

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;

#define CMP(x,y) \
    (fabsf((x)-(y))<=FLT_EPSILON) * \
    fmax(1.0f, fmax(fabsf(x), fabsf(y))) \

// #define ABSOLUTE(x,y) (fabsf((x)-(y))<=FLT_EPSILON)
// #define RELATIVE(x,y) (fabsf((x)-(y))<=FLT_EPSILON * fmax(fabsf(x), fabsf(y)))

inline bool absolute_equal(float x, float y) {
    return std::fabs(x - y) <= FLT_EPSILON;
}

inline bool relative_equal(float x, float y) {
    return std::fabs(x - y) <= FLT_EPSILON * std::fmax(std::fabs(x), std::fabs(y));
}


// Magnitude équivalente à length
inline float Magnitude(const glm::vec2& v) {
    return sqrtf(glm::dot(v, v));
}
inline float Magnitude(const glm::vec3& v) {
    return sqrtf(glm::dot(v, v));
}
// Magnitude au carré (plus performant si on a pas besoin de la racine carrée)
inline float MagnitudeSq(const glm::vec2& v) {
    return glm::dot(v, v);
}
inline float MagnitudeSq(const glm::vec3& v) {
    return glm::dot(v, v);
}

void Normalize(glm::vec2& v){
    v = v * (1.0f / Magnitude(v));
}
void Normalize(glm::vec3& v){
    v = v * (1.0f / Magnitude(v));
}
vec2 Normalized(const glm::vec2& v){
    return v * (1.0f / Magnitude(v));
}
vec3 Normalized(const glm::vec3& v){
    return v * (1.0f / Magnitude(v));
}

float Angle(const vec2& l, const vec2& r){
    float m = sqrtf(MagnitudeSq(l) * MagnitudeSq(r));
    return acos(glm::dot(l, r) / m);
}
float Angle(const vec3& l, const vec3& r){
    float m = sqrtf(MagnitudeSq(l) * MagnitudeSq(r));
    return acos(glm::dot(l, r) / m);
}

#define RAD2DEG(x) ((x) * 57.295754f)
#define DEG2RAD(x) ((x) * 0.0174533f)

vec2 Project(const vec2& length, const vec2& direction){
    float dot = glm::dot(length, direction);
    float magSq = MagnitudeSq(direction);
    return direction * (dot / magSq);
}
vec3 Project(const vec3& length, const vec3& direction){
    float dot = glm::dot(length, direction);
    float magSq = MagnitudeSq(direction);
    return direction * (dot / magSq);
}
vec2 Perpendicular(const vec2& len, const vec2& dir){
    return len - Project(len, dir);
}
vec3 Perpendicular(const vec3& len, const vec3& dir){
    return len - Project(len, dir);
}

vec2 Reflection(const vec2& vec, const vec2& normal){
    float d = glm::dot(vec, normal);
    return vec - normal * (2.0f * d);
}
vec3 Reflection(const vec3& vec, const vec3& normal){
    float d = glm::dot(vec, normal);
    return vec - normal * (2.0f * d);
}
#endif