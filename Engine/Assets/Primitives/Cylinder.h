#ifndef CYLINDER_H
#define CYLINDER_H
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



class Cylinder : public Mesh {
public:

    Cylinder() : Mesh() {}
    void build_arrays(std::vector<glm::vec3> & positions,
                  std::vector<glm::vec3> & normals,
                  std::vector<glm::vec2> & uvs,
                  std::vector<unsigned short> & indices, float r = 1.0f, float h = 1.0f, int segments = 32) 
    {
        positions.clear();
        normals.clear();
        uvs.clear();
        indices.clear();
        for(unsigned int i = 0; i<= segments; i++){
            float theta = (float)i / (float)segments * 2.0f * M_PI;
            float x = r * cos(theta);
            float z = r * sin(theta);
            // Bottom circle
            positions.push_back(glm::vec3(x, 0.0f, z));
            normals.push_back(glm::vec3(x, 0.0f, z)); // Normals pointing outwards
            uvs.push_back(glm::vec2((float)i / (float)segments, 0.0f));
            // Top circle
            positions.push_back(glm::vec3(x, h, z));
            normals.push_back(glm::vec3(x, 0.0f, z)); // Normals pointing outwards
            uvs.push_back(glm::vec2((float)i / (float)segments, 1.0f));
        }

        // Générer les indices pour les triangles
        for(unsigned int i = 0; i < segments; i++) {
            unsigned int base = i * 2;
            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 2);
            indices.push_back(base + 1);
            indices.push_back(base + 3);
        }

        unsigned int centerBottomIndex = positions.size();
        positions.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
        normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
        uvs.push_back(glm::vec2(0.5f, 0.5f)); 

        unsigned int centerTopIndex = positions.size();
        positions.push_back(glm::vec3(0.0f, h, 0.0f));
        normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        uvs.push_back(glm::vec2(0.5f, 0.5f));

        for(unsigned int i = 0; i < segments; i++) {
            unsigned int next = (i + 1) % segments;
            indices.push_back(centerBottomIndex);
            indices.push_back(i * 2);
            indices.push_back(next * 2);
            indices.push_back(centerTopIndex);
            indices.push_back(next * 2 + 1);
            indices.push_back(i * 2 + 1);
        }

    }


};


#endif
