#ifndef Box_H
#define Box_H
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



class Box : public Mesh {
public:
    glm::vec3 centre;

    Box() : Mesh() {}
    void build_arrays(std::vector<glm::vec3> & positions,
                  std::vector<glm::vec3> & normals,
                  std::vector<glm::vec2> & uvs,
                  std::vector<unsigned short> & indices) 
    {
        positions.clear();
        normals.clear();
        uvs.clear();
        indices.clear();

        // Les 8 sommets du cube
        glm::vec3 verts[8] = {
            {0, 0, 0},
            {1.0, 0, 0},
            {1.0, 1.0, 0},
            {0, 1.0, 0},
            {0, 0, 1.0},
            {1.0, 0, 1.0},
            {1.0, 1.0, 1.0},
            {0, 1.0, 1.0}
        };

        // Les normales pour chaque face (6 faces)
        glm::vec3 faceNormals[6] = {
            { 0, 0,-1}, // face avant
            { 0, 0, 1}, // face arrière
            {-1, 0, 0}, // gauche
            { 1, 0, 0}, // droite
            { 0, 1, 0}, // haut
            { 0,-1, 0}  // bas
        };

        // Chaque face = 4 indices dans verts pour un quad
        unsigned short faceIndices[6][4] = {
            {0, 1, 2, 3}, // avant
            {5, 4, 7, 6}, // arrière
            {4, 0, 3, 7}, // gauche
            {1, 5, 6, 2}, // droite
            {3, 2, 6, 7}, // haut
            {4, 5, 1, 0}  // bas
        };

        // Générer les sommets
        for (int f = 0; f < 6; f++) {
            for (int i = 0; i < 4; i++) {
                positions.push_back(verts[faceIndices[f][i]]);
                normals.push_back(faceNormals[f]);
            }
            // UVs simples
            uvs.push_back({0.0f, 0.0f});
            uvs.push_back({1.0f, 0.0f});
            uvs.push_back({1.0f, 1.0f});
            uvs.push_back({0.0f, 1.0f});
        }

        // Générer les indices pour triangles (2 triangles par face)
        for (unsigned short f = 0; f < 6; f++) {
            unsigned short base = f * 4;
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 1);

            indices.push_back(base + 0);
            indices.push_back(base + 3);
            indices.push_back(base + 2);
        }
    }


};

#endif
