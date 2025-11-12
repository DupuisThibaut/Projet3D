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
            m_normal = cross(rightVector , upVector);
            m_bottom_left = bottomLeft;
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