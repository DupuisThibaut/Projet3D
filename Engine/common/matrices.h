#ifndef _H_MATH_MATRICES
#define _H_MATH_MATRICES
#include <glm/glm.hpp>
#include "vectors.h"
typedef glm::mat2 mat2;
typedef glm::mat3 mat3;
typedef glm::mat4 mat4;

#define CMP(x,y) \
    (fabsf((x)-(y))<=FLT_EPSILON) * \
    fmax(1.0f, fmax(fabsf(x), fabsf(y))) \

mat2 Cut(const mat3& mat, int row, int col) {
    mat2 result;
    int idx = 0;
    for (int i = 0; i < 3; i++) {
        if (i == row) continue;
        for (int j = 0; j < 3; j++) {
            if (j == col) continue;
            result[idx / 2][idx % 2] = mat[i][j];
            idx++;
        }
    }
    return result;
}
mat3 Cut(const mat4& mat, int row, int col) {
    mat3 result;
    int idx = 0;
    for (int i = 0; i < 4; i++) {
        if (i == row) continue;
        for (int j = 0; j < 4; j++) {
            if (j == col) continue;
            result[idx / 3][idx % 3] = mat[i][j];
            idx++;
        }
    }
    return result;
}

mat3 Minor(const mat3& mat) {
    mat3 result;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++){
            result [i][j] = glm::determinant(Cut(mat, i, j));
        }
    }
    return result;
}
mat2 Minor(const mat2& mat) {
    return mat2( mat[1][1], mat[1][0],
                 mat[0][1], mat[0][0]);
}
mat4 Minor(const mat4& mat) {
    mat4 result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++){
            result [i][j] = glm::determinant(Cut(mat, i, j));
        }
    }
    return result;
}

void Cofactor(mat3& out, const mat3& minor) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++){
            int t = 3 * j + i;
            int s = 3 * j + i; 
            float sign = powf(-1.0f, float(i + j));
            out[t] = sign * minor[s];
        }
    }
}
void Cofactor(mat2& out, const mat2& minor) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++){
            int t = 2 * j + i;
            int s = 2 * j + i; 
            float sign = powf(-1.0f, float(i + j));
            out[t] = sign * minor[s];
        }
    }
}
void Cofactor(mat4& out, const mat4& minor) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++){
            int t = 4 * j + i;
            int s = 4 * j + i; 
            float sign = powf(-1.0f, float(i + j));
            out[t] = sign * minor[s];
        }
    }
}
mat2 Cofactor(const mat2& minor){
    mat2 result;
    Cofactor(result, minor);
    return result;
}
mat3 Cofactor(const mat3& minor){
    mat3 result;
    Cofactor(result, minor);
    return result;
}
mat4 Cofactor(const mat4& minor){
    mat4 result;
    Cofactor(result, minor);
    return result;
}

mat2 Adjugate(const mat2& mat) {
    return glm::transpose(Cofactor(mat));
}
mat3 Adjugate(const mat3& mat) {
    return glm::transpose(Cofactor(mat));
}
mat4 Adjugate(const mat4& mat) {
    return glm::transpose(Cofactor(mat));
}

mat4 Translation(const vec3& pos){
    return mat4(1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                pos.x, pos.y, pos.z, 1.0f);
}
vec3 GetTranslation(const mat4& mat){
    return vec3(mat[3][0], mat[3][1], mat[3][2]);
}
mat4 Scale(float x, float y, float z){
    return mat4(x,    0.0f, 0.0f, 0.0f,
                0.0f, y,    0.0f, 0.0f,
                0.0f, 0.0f, z,    0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}
mat4 Scale(const vec3& s){
    return Scale(s.x, s.y, s.z);
}
vec3 GetScale(const mat4& mat){
    return vec3(mat[0][0], mat[1][1], mat[2][2]);
}
#define DEG2RAD(x) ((x) * 0.0174533f)
mat4 ZRotation(float angle){
    angle = DEG2RAD(angle);
    return mat4( cosf(angle), sinf(angle), 0.0f, 0.0f,
                -sinf(angle), cosf(angle), 0.0f, 0.0f,
                 0.0f,        0.0f,       1.0f, 0.0f,
                 0.0f,        0.0f,       0.0f, 1.0f);
}
mat3 ZRotation3x3(float angle){
    angle = DEG2RAD(angle);
    return mat3( cosf(angle), sinf(angle), 0.0f,
                -sinf(angle), cosf(angle), 0.0f,
                 0.0f,        0.0f,       1.0f);
}
mat4 XRotation(float angle){
    angle = DEG2RAD(angle);
    return mat4(1.0f, 0.0f,        0.0f,       0.0f,
                0.0f, cosf(angle), sinf(angle), 0.0f,
                0.0f,-sinf(angle), cosf(angle), 0.0f,
                0.0f, 0.0f,        0.0f,       1.0f);
}
mat3 XRotation3x3(float angle){
    angle = DEG2RAD(angle);
    return mat3(1.0f, 0.0f,        0.0f,
                0.0f, cosf(angle), sinf(angle),
                0.0f,-sinf(angle), cosf(angle));
}
mat4 YRotation(float angle){
    angle = DEG2RAD(angle);
    return mat4( cosf(angle), 0.0f, -sinf(angle), 0.0f,
                 0.0f,       1.0f,  0.0f,       0.0f,
                 sinf(angle), 0.0f, cosf(angle), 0.0f,
                 0.0f,       0.0f,  0.0f,       1.0f);
}
mat3 YRotation3x3(float angle){
    angle = DEG2RAD(angle);
    return mat3( cosf(angle), 0.0f, -sinf(angle),
                 0.0f,       1.0f,  0.0f,
                 sinf(angle), 0.0f, cosf(angle));
}
mat4 Rotation(float pitch, float yaw, float roll){
    return ZRotation(roll) * YRotation(yaw) * XRotation(pitch); 
}
mat3 Rotation3x3(float pitch, float yaw, float roll){
    return ZRotation3x3(yaw) *
    XRotation3x3(pitch) *
    YRotation3x3(roll);
}

mat4 AxisAngle(const vec3& axis, float angle){
    angle = DEG2RAD(angle);
    float c = cosf(angle);
    float s = sinf(angle);
    float t = 1.0f - c;

    float x = axis.x;
    float y = axis.y;
    float z = axis.z;
    if(!CMP(MagnitudeSq(axis), 1.0f)){
        float inv_len = 1.0f / Magnitude(axis);
        x *= inv_len;
        y *= inv_len;
        z *= inv_len;
    }
    return mat4(
        t*(x*x) + c, t*x*y + s*z, t*x*z - s*y, 0.0f,
        t*x*y - s*z, t*(y*y) + c, t*y*z + s*x, 0.0f,
        t*x*z + s*y, t*y*z - s*x, t*(z*z) + c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
        );
}

mat3 AxisAngle3x3(const vec3& axis, float angle){
    angle = DEG2RAD(angle);
    float c = cosf(angle);
    float s = sinf(angle);
    float t = 1.0f - c;

    float x = axis.x;
    float y = axis.y;
    float z = axis.z;
    if(!CMP(MagnitudeSq(axis), 1.0f)){
        float inv_len = 1.0f / Magnitude(axis);
        x *= inv_len;
        y *= inv_len;
        z *= inv_len;
    }
    return mat3(
        t*(x*x) + c, t*x*y + s*z, t*x*z - s*y,
        t*x*y - s*z, t*(y*y) + c, t*y*z + s*x,
        t*x*z + s*y, t*y*z - s*x, t*(z*z) + c
        );
}
#endif