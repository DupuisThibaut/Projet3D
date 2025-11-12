#ifndef CAPSULE_H
#define CAPSULE_H

#include <vector>
#include <cmath>
#include <glm/glm.hpp>

class Capsule : public Mesh {
public:
    Capsule() : Mesh() {}

    void build_arrays(std::vector<glm::vec3>& positions,
                      std::vector<glm::vec3>& normals,
                      std::vector<glm::vec2>& uvs,
                      std::vector<unsigned short>& indices,
                      float r = 1.0f, float h = 1.0f,
                      int segments = 32)
    {
        positions.clear();
        normals.clear();
        uvs.clear();
        indices.clear();

        int rings = segments / 2;

        // --- Cylindre ---
        int cylStart = positions.size();
        for (int i = 0; i <= segments; i++) {
            float theta = (float)i / segments * 2.0f * M_PI;
            float x = r * cos(theta);
            float z = r * sin(theta);
            // bas
            positions.push_back(glm::vec3(x, 0.0f, z));
            normals.push_back(glm::normalize(glm::vec3(x, 0.0f, z)));
            uvs.push_back(glm::vec2((float)i / segments, 0.0f));
            // haut
            positions.push_back(glm::vec3(x, h, z));
            normals.push_back(glm::normalize(glm::vec3(x, 0.0f, z)));
            uvs.push_back(glm::vec2((float)i / segments, 1.0f));
        }

        // Indices cylindre
        for (int i = 0; i < segments; i++) {
            int base = cylStart + i * 2;
            indices.push_back(base);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 2);
            indices.push_back(base + 1);
            indices.push_back(base + 3);
        }

        // --- Hémisphère bas ---
        int hemiBottomStart = positions.size();
        for (int phiIt = 0; phiIt <= rings; phiIt++) {
            float phi = (float)phiIt / rings * (M_PI / 2.0f) - M_PI / 2.0f; // -π/2 -> 0
            for (int thetaIt = 0; thetaIt <= segments; thetaIt++) {
                float theta = (float)thetaIt / segments * 2.0f * M_PI;
                glm::vec3 xyz;
                xyz.x = r * cos(phi) * cos(theta);
                xyz.y = r * sin(phi); // vertical
                xyz.z = r * cos(phi) * sin(theta);
                positions.push_back(glm::vec3(xyz.x, xyz.y, xyz.z));
                normals.push_back(glm::normalize(glm::vec3(xyz.x, xyz.y, xyz.z)));
                uvs.push_back(glm::vec2((float)thetaIt / segments, (float)phiIt / rings * 0.5f));
            }
        }

        for (int phiIt = 0; phiIt < rings; phiIt++) {
            for (int thetaIt = 0; thetaIt < segments; thetaIt++) {
                int current = hemiBottomStart + phiIt * (segments + 1) + thetaIt;
                int next = current + segments + 1;
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }

        // --- Hémisphère haut ---
        int hemiTopStart = positions.size();
        for (int phiIt = 0; phiIt <= rings; phiIt++) {
            float phi = (float)phiIt / rings * (M_PI / 2.0f); // 0 -> π/2
            for (int thetaIt = 0; thetaIt <= segments; thetaIt++) {
                float theta = (float)thetaIt / segments * 2.0f * M_PI;
                glm::vec3 xyz;
                xyz.x = r * cos(phi) * cos(theta);
                xyz.y = r * sin(phi) + h; // décalé en haut
                xyz.z = r * cos(phi) * sin(theta);
                positions.push_back(glm::vec3(xyz.x, xyz.y, xyz.z));
                normals.push_back(glm::normalize(glm::vec3(xyz.x, xyz.y - h, xyz.z))); // normals depuis le centre du haut
                uvs.push_back(glm::vec2((float)thetaIt / segments, 0.5f + (float)phiIt / rings * 0.5f));
            }
        }

        for (int phiIt = 0; phiIt < rings; phiIt++) {
            for (int thetaIt = 0; thetaIt < segments; thetaIt++) {
                int current = hemiTopStart + phiIt * (segments + 1) + thetaIt;
                int next = current + segments + 1;
                indices.push_back(current + 1);
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(next + 1);
                indices.push_back(current + 1);
                indices.push_back(next);
            }
        }
    }
};

#endif
