#ifndef PLANE_H
#define PLANE_H
#include "../../common/Line.h"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

class Plane {
public:
    public:
    glm::vec3 m_normal;
    glm::vec3 m_bottom_left;
    glm::vec3 m_right_vector;
    glm::vec3 m_up_vector;

    Plane(){}
    Plane(glm::vec3 const & bottomLeft , glm::vec3 const & rightVector , glm::vec3 const & upVector , float width=1. , float height=1. ,
           float uMin = 0.f , float uMax = 1.f , float vMin = 0.f , float vMax = 1.f){
            m_right_vector = rightVector;
            m_up_vector = upVector;
            m_normal = glm::normalize(glm::cross(rightVector , upVector));
            m_bottom_left = bottomLeft;
    }

    void createGridMesh(int nx, int ny,
                        std::vector<glm::vec3> & outVertices,
                        std::vector<glm::vec3> & outNormals,
                        std::vector<glm::vec2> & outUVs,
                        std::vector<unsigned short> & outIndices,
                        std::vector<std::vector<unsigned short>> * outTriangles = nullptr,
                        float width = 1.0f, float height = 1.0f,
                        float uMin = 0.0f, float uMax = 1.0f, float vMin = 0.0f, float vMax = 1.0f)
    {
        outVertices.clear();
        outNormals.clear();
        outUVs.clear();
        outIndices.clear();
        if (outTriangles) outTriangles->clear();

        if (nx < 1) nx = 1;
        if (ny < 1) ny = 1;

        glm::vec3 rightDir = m_right_vector;
        glm::vec3 upDir = m_up_vector;

        // normalize directions but keep orientation
        if (glm::length(rightDir) > 0.0f) rightDir = glm::normalize(rightDir);
        if (glm::length(upDir) > 0.0f) upDir = glm::normalize(upDir);

        // originBase is computed so that the plane is centered at (0,0,0)
        // m_bottom_left is interpreted as the plane center by default; we offset to get bottom-left.
        glm::vec3 originBase = m_bottom_left - rightDir * (width * 0.5f) - upDir * (height * 0.5f);

        glm::vec3 rightStep = rightDir * (width);
        glm::vec3 upStep = upDir * (height);

        // We'll build a (nx+1) x (ny+1) grid of vertices
        for (int j = 0; j <= ny; ++j) {
            float v = (ny == 0) ? 0.0f : float(j) / float(ny);
            for (int i = 0; i <= nx; ++i) {
                float u = (nx == 0) ? 0.0f : float(i) / float(nx);
                glm::vec3 pos = originBase + rightStep * u + upStep * v;
                outVertices.push_back(pos);
                outNormals.push_back(glm::normalize(m_normal));
                float uu = glm::mix(uMin, uMax, u);
                float vv = glm::mix(vMin, vMax, v);
                outUVs.push_back(glm::vec2(uu, vv));
            }
        }

        // indices and triangles
        int vertsPerRow = nx + 1;
        for (int j = 0; j < ny; ++j) {
            for (int i = 0; i < nx; ++i) {
                unsigned short v0 = (unsigned short)(j * vertsPerRow + i);
                unsigned short v1 = (unsigned short)(v0 + 1);
                unsigned short v2 = (unsigned short)(v0 + vertsPerRow);
                unsigned short v3 = (unsigned short)(v2 + 1);

                // two triangles: (v0, v1, v3) and (v0, v3, v2)
                outIndices.push_back(v0);
                outIndices.push_back(v1);
                outIndices.push_back(v3);

                outIndices.push_back(v0);
                outIndices.push_back(v3);
                outIndices.push_back(v2);

                if (outTriangles) {
                    outTriangles->push_back(std::vector<unsigned short>{v0, v1, v3});
                    outTriangles->push_back(std::vector<unsigned short>{v0, v3, v2});
                }
            }
        }
    }

    // void setQuad( glm::vec3 const & bottomLeft , glm::vec3 const & rightVector , glm::vec3 const & upVector , float width=1. , float height=1. ,
    //               float uMin = 0.f , float uMax = 1.f , float vMin = 0.f , float vMax = 1.f) {
    //     m_right_vector = rightVector;
    //     m_up_vector = upVector;
    //     m_normal = Vec3::cross(rightVector , upVector);
    //     m_bottom_left = bottomLeft;

    //     m_normal.normalize();
    //     m_right_vector.normalize();
    //     m_up_vector.normalize();

    //     m_right_vector = m_right_vector*width;
    //     m_up_vector = m_up_vector*height;

    //     vertices.clear();
    //     vertices.resize(4);
    //     vertices[0].position = bottomLeft;                                      vertices[0].u = uMin; vertices[0].v = vMin;
    //     vertices[1].position = bottomLeft + m_right_vector;                     vertices[1].u = uMax; vertices[1].v = vMin;
    //     vertices[2].position = bottomLeft + m_right_vector + m_up_vector;       vertices[2].u = uMax; vertices[2].v = vMax;
    //     vertices[3].position = bottomLeft + m_up_vector;                        vertices[3].u = uMin; vertices[3].v = vMax;
    //     vertices[0].normal = vertices[1].normal = vertices[2].normal = vertices[3].normal = m_normal;
    //     triangles.clear();
    //     triangles.resize(2);
    //     triangles[0][0] = 0;
    //     triangles[0][1] = 1;
    //     triangles[0][2] = 2;
    //     triangles[1][0] = 0;
    //     triangles[1][1] = 2;
    //     triangles[1][2] = 3;


    // }

    // void rebuild(){
    //     m_bottom_left = vertices[0].position;
    //     m_right_vector = vertices[1].position - vertices[0].position;
    //     m_up_vector = vertices[3].position - vertices[0].position;
    //     m_normal = Vec3::cross(m_right_vector, m_up_vector);
    //     m_normal.normalize();
    // }
};

#endif