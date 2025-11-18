#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <common/objloader.hpp>

enum class PrimitiveType {
    PLANE,
    CUBE,
    SPHERE,
    CYLINDER,
    CONE,
    CAPSULE,
    MESH
};
struct BoundingSphere {
    glm::vec3 center;
    float radius;
};

struct MeshComponent {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    GLuint normalVBO = 0;
    GLuint uvVBO = 0;
    GLuint vertexCount = 0;
    
    PrimitiveType type;
    std::string meshFilePath="";

    BoundingSphere boundingSphereFrustrumCulling;

    // Pour le plan
    glm::vec3 normal;
    float width = 1.0f;
    float height = 1.0f;
    float subdivisions = 100;

    std::vector<glm::vec3> vertices;
    std::vector<unsigned short> indices;
    std::vector<std::vector<unsigned short>> triangles;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    glm::vec3 centre;
    glm::vec3 m_right_vector;
    glm::vec3 m_up_vector;
    float rayon;

    void load_OFF(const std::string& filename) {
        type=PrimitiveType::MESH;

        if (!loadOFF(filename, vertices, indices, triangles)) {
            std::cerr << "Failed to load OFF file: " << filename << std::endl;
            return;
        }
        
        normals.resize(vertices.size(), glm::vec3(0.0f));
        for (size_t i = 0; i < triangles.size(); ++i) {
            const std::vector<unsigned short>& tri = triangles[i];
            if (tri.size() < 3) continue;

            glm::vec3 v0 = vertices[tri[0]];
            glm::vec3 v1 = vertices[tri[1]];
            glm::vec3 v2 = vertices[tri[2]];
            glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

            for (size_t j = 0; j < tri.size(); ++j) {
                normals[tri[j]] += normal;
            }
        }
        for (auto& n : normals) {
            if (glm::length(n) > 0.0f) n = glm::normalize(n);
        }

        for (auto& index : indices) {
            glm::vec3 vertex = vertices[index];
            float r = glm::length(vertex);
            if (r > 0.0f) {
                float theta = atan2(vertex.z, vertex.x);
                float phi = acos(glm::clamp(vertex.y / r, -1.0f, 1.0f));
                float u = theta / (2.0f * M_PI);
                float v = phi / M_PI;
                if (u < 0.0f) u += 1.0f;
                uvs.push_back(glm::vec2(u, v));
            } else {
                uvs.push_back(glm::vec2(0.0f, 0.0f));
            }
        }

        vertexCount = indices.size();

        // Calcul de la bounding sphere
        glm::vec3 center(0.0f);
        for (const auto& v : vertices) {
            center += v;
        }
        center /= static_cast<float>(vertices.size());

        float maxRadius = 0.0f;
        for (const auto& v : vertices) {
            float distance = glm::length(v - center);
            if (distance > maxRadius) {
                maxRadius = distance;
            }
        }
        if(!vertices.empty()){
            boundingSphereFrustrumCulling.center = center;
            boundingSphereFrustrumCulling.radius = maxRadius;
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenBuffers(1, &uvVBO);
        glGenBuffers(1, &normalVBO);

        glBindVertexArray(VAO);

        // positions
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        // UVs
        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), uvs.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
        glEnableVertexAttribArray(1);

        // normals
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(2);

        // indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    void loadPrimitive(const std::string& primitiveType, glm::vec3 right_vector=glm::vec3(0.0,0.0,1.0), glm::vec3 up_vector=glm::vec3(1.0,0.0,0.0), float rayon=1.0f) {
        if (primitiveType == "PLANE"){
            type=PrimitiveType::PLANE;
            centre=glm::vec3(0,0,0);
            m_right_vector=right_vector;
            m_up_vector=up_vector;
            Plane plane(centre, right_vector, up_vector);
            plane.createGridMesh(subdivisions, subdivisions, vertices, normals, uvs, indices);
        }
        else if (primitiveType == "SPHERE"){
            type=PrimitiveType::SPHERE;
            Sphere sphere(rayon);
            centre=glm::vec3(0,0,0);
            rayon=rayon;
            sphere.build_arrays(subdivisions, subdivisions, vertices, normals, uvs, indices);
        }
        else if (primitiveType == "BOX"){
            type=PrimitiveType::CUBE;
            Box box;
            box.build_arrays(vertices, normals, uvs, indices);
        }
        else if (primitiveType == "CYLINDER"){
            type=PrimitiveType::CYLINDER;
            Cylinder cylinder;
            cylinder.build_arrays(vertices, normals, uvs, indices, width, height, subdivisions);
        }
        else if (primitiveType == "CONE"){
            type=PrimitiveType::CONE;
            Cone cone;
            cone.build_arrays(vertices, normals, uvs, indices, width, height, subdivisions);
        }
        else if (primitiveType == "CAPSULE"){
            type=PrimitiveType::CAPSULE;
            Capsule capsule;
            capsule.build_arrays(vertices, normals, uvs, indices, width, height, subdivisions);
        }
        else {
            std::cerr << "Unknown primitive type: " << primitiveType << std::endl;
            return;
        }

        vertexCount = indices.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenBuffers(1, &uvVBO);
        glGenBuffers(1, &normalVBO);

        glBindVertexArray(VAO);

        // positions
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        // UVs
        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), uvs.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
        glEnableVertexAttribArray(1);

        // normals
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(2);

        // indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);

    }
};

#endif // MESH_COMPONENT_H