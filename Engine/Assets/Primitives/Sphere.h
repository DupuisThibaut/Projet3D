#ifndef Sphere_H
#define Sphere_H
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

// struct RaySphereIntersection{
//     bool intersectionExists;
//     float t,u,v;
//     float theta,phi;
//     glm::vec3 intersection;
//     glm::vec3 secondintersection;
//     glm::vec3 normal;
// };

// static
// Vec3 SphericalCoordinatesToEuclidean( Vec3 ThetaPhiR ) {
//     return ThetaPhiR[2] * Vec3( cos(ThetaPhiR[0]) * cos(ThetaPhiR[1]) , sin(ThetaPhiR[0]) * cos(ThetaPhiR[1]) , sin(ThetaPhiR[1]) );
// }
static
glm::vec3 SphericalCoordinatesToEuclidean( float theta , float phi ) {
    return glm::vec3( cos(theta) * cos(phi) , sin(theta) * cos(phi) , sin(phi) );
}

// static
// Vec3 EuclideanCoordinatesToSpherical( Vec3 xyz ) {
//     float R = xyz.length();
//     float phi = asin( xyz[2] / R );
//     float theta = atan2( xyz[1] , xyz[0] );
//     return Vec3( theta , phi , R );
// }



class Sphere : public Mesh {
public:
   glm::vec3 m_center;
    float m_radius;

    Sphere() : Mesh() {}
    Sphere(float r) : Mesh() , m_center(glm::vec3(0.0f)) , m_radius(r) {}

// ...existing code...
    void build_arrays(int nTheta , int nPhi, std::vector<glm::vec3> & positions, std::vector<glm::vec3> & normals, std::vector<glm::vec2> & uvs, std::vector<unsigned short> & indices ) {
        const unsigned int nVert = nTheta * nPhi;
        positions.resize(nVert);
        normals.resize(nVert);
        uvs.resize(nVert);
        indices.clear();
        indices.reserve( 6 * (nTheta - 1) * (nPhi - 1) );
         for( unsigned int thetaIt = 0 ; thetaIt < nTheta ; ++thetaIt ) {
             float u = (float)(thetaIt) / (float)(nTheta-1);
             float theta = u * 2 * M_PI;
             for( unsigned int phiIt = 0 ; phiIt < nPhi ; ++phiIt ) {
                 unsigned int vertexIndex = thetaIt + phiIt * nTheta;
                 float v = (float)(phiIt) / (float)(nPhi-1);
                 float phi = - M_PI/2.0 + v * M_PI;
                 glm::vec3 xyz = SphericalCoordinatesToEuclidean( theta , phi );
                positions[ vertexIndex ] = m_center + m_radius * xyz;
                normals[ vertexIndex ] = glm::normalize(xyz);
                uvs[ vertexIndex ] = glm::vec2(u, v);
             }
         }
         triangles_array.clear();
        for( unsigned int thetaIt = 0 ; thetaIt < nTheta - 1 ; ++thetaIt ) {
            for( unsigned int phiIt = 0 ; phiIt < nPhi - 1 ; ++phiIt ) {
                unsigned int v00 = thetaIt + phiIt * nTheta;
                unsigned int v10 = thetaIt + 1 + phiIt * nTheta;
                unsigned int v01 = thetaIt + (phiIt+1) * nTheta;
                unsigned int v11 = thetaIt + 1 + (phiIt+1) * nTheta;
                // two triangles per quad
                indices.push_back((unsigned short)v00);
                indices.push_back((unsigned short)v10);
                indices.push_back((unsigned short)v11);
                indices.push_back((unsigned short)v00);
                indices.push_back((unsigned short)v11);
                indices.push_back((unsigned short)v01);
                // also fill helper triangles_array for CPU-side ops if needed
                triangles_array.push_back(v00);
                triangles_array.push_back(v10);
                triangles_array.push_back(v11);
                triangles_array.push_back(v00);
                triangles_array.push_back(v11);
                triangles_array.push_back(v01);
            }
        }
     }

    // // Calcul de l'intersection entre le rayon et la sphère :
    // RaySphereIntersection intersect(const Ray &ray) const {
    //     RaySphereIntersection intersection;
    //     glm::vec3 oc = ray.origin() - m_center;
    //     float a = glm::dot(ray.direction(), ray.direction());
    //     float b = 2.0f * glm::dot(ray.direction(), oc);
    //     float c = glm::dot(oc, oc) - (m_radius * m_radius);
    //     float discriminant = (b * b) - (4.0f * a * c);
        
    //     // Si le discriminant est négatif, il n'y a pas d'intersection
    //     if (discriminant < 0) {
    //         intersection.intersectionExists = false;
    //         return intersection;
    //     }
    //     float sqrtDiscriminant = sqrt(discriminant);
    //     float t1 = (-b - sqrtDiscriminant) / (2.0f * a);
    //     float t2 = (-b + sqrtDiscriminant) / (2.0f * a);
    //     // Trouver le plus petit t positif
    //     float t = FLT_MAX;
    //     if (t1 > 0) {
    //         t = t1;
    //         intersection.intersection=ray.origin()+t1*ray.direction();
    //         intersection.normal = glm::vec3((intersection.intersection-m_center)/m_radius);
    //     }
    //     if (t2 > 0 && t2 < t) {t = t2;intersection.secondintersection=ray.origin()+t2*ray.direction();} 
    //     intersection.t = t;
    //     // Vérifier si t est toujours infini, ce qui signifie qu'il n'y a pas d'intersection
    //     if (intersection.t == FLT_MAX) {
    //         intersection.intersectionExists = false;
    //         return intersection;
    //     }
    //     intersection.normal = intersection.intersection - m_center;
    //     intersection.normal = glm::normalize(intersection.normal);
    //     intersection.intersectionExists = true;
    //     float theta = atan2(intersection.normal[2], intersection.normal[0]);
    //      float phi = acos(intersection.normal[1]); 
    //      intersection.u = (theta + M_PI) / (2.0 * M_PI); 
    //      intersection.v = phi / M_PI;
    //     return intersection;
    // }

};

#endif
