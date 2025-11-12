#ifndef CONE_H
#define CONE_H
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
#include <GL/glut.h>



class Cone : public Mesh {
public:

    Cone() : Mesh() {}
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
            positions.push_back(glm::vec3(x, 0.0f, z));
            normals.push_back(glm::vec3(x, 0.0f, z));
            uvs.push_back(glm::vec2((float)i / (float)segments, 0.0f));
        }

        unsigned int centerTopIndex = positions.size();
        positions.push_back(glm::vec3(0.0f, h, 0.0f));
        normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
        uvs.push_back(glm::vec2(0.5f, 0.5f));
        for(unsigned int i = 0; i < segments; i++) {
            unsigned int base = segments + 1;
            indices.push_back(i);
            indices.push_back(base);
            indices.push_back(i + 1);
        }

        unsigned int centerBottomIndex = positions.size();
        positions.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
        normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
        uvs.push_back(glm::vec2(0.5f, 0.5f)); 

        for(unsigned int i = 0; i < segments; i++) {
            unsigned int next = (i + 1) % segments;
            indices.push_back(centerBottomIndex);
            indices.push_back(i * 2);
            indices.push_back(next * 2);
        }

    }


};


#endif
