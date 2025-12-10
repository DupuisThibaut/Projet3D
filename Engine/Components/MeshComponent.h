#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <common/objloader.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

enum class PrimitiveType {
    PLANE,
    CUBE,
    SPHERE,
    CYLINDER,
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
    GLuint boneIDVBO = 0;
    GLuint boneWeightVBO = 0;
    
    PrimitiveType type;
    std::string meshFilePath;

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
    glm::vec3 m_bottom_left;
    float rayon;

    bool update=false;
    int nb=-1;

        void loadFromFile(const nlohmann::json& entityData, uint32_t entityId, const std::string& gameFolder, glm::vec3& position) {
        if(entityData["entities"][entityId].contains("mesh")){
            if (entityData["entities"][entityId]["mesh"]["type"] == "primitive") {
                if (entityData["entities"][entityId]["mesh"]["mesh_type"] == "PLANE") {
                    glm::vec3 normal(0.0f, 1.0f, 0.0f);
                    if (entityData["entities"][entityId]["mesh"].contains("normal")) {
                        this->normal = glm::vec3(entityData["entities"][entityId]["mesh"]["normal"][0],
                                           entityData["entities"][entityId]["mesh"]["normal"][1],
                                           entityData["entities"][entityId]["mesh"]["normal"][2]);
                    }
                    float width = 1.0f;
                    int subdivisions = 1;
                    if (entityData["entities"][entityId]["mesh"].contains("subdivisions")) {
                        this->subdivisions = entityData["entities"][entityId]["mesh"]["subdivisions"];
                    }
                    this->loadPrimitive("PLANE");
                }
                else if (entityData["entities"][entityId]["mesh"]["mesh_type"] == "SPHERE") {
                    if(entityData["entities"][entityId]["mesh"].contains("subdivisions")) {
                        this->subdivisions = entityData["entities"][entityId]["mesh"]["subdivisions"];
                    }
                    this->loadPrimitive("SPHERE", position);
                }
                else if (entityData["entities"][entityId]["mesh"]["mesh_type"] == "BOX") {
                    this->loadPrimitive("BOX", position);
                }
                else if (entityData["entities"][entityId]["mesh"]["mesh_type"] == "CYLINDER") {
                    if (entityData["entities"][entityId]["mesh"].contains("subdivisions")) {
                        this->subdivisions = entityData["entities"][entityId]["mesh"]["subdivisions"];
                    }
                    if (entityData["entities"][entityId]["mesh"].contains("width")) {
                        this->width = entityData["entities"][entityId]["mesh"]["width"];
                    }
                    if (entityData["entities"][entityId]["mesh"].contains("height")) {
                        this->height = entityData["entities"][entityId]["mesh"]["height"];
                    }
                    this->loadPrimitive("CYLINDER", position);
                }
                else if (entityData["entities"][entityId]["mesh"]["mesh_type"] == "CONE") {
                    if (entityData["entities"][entityId]["mesh"].contains("subdivisions")) {
                        this->subdivisions = entityData["entities"][entityId]["mesh"]["subdivisions"];
                    }
                    if (entityData["entities"][entityId]["mesh"].contains("width")) {
                        this->width = entityData["entities"][entityId]["mesh"]["width"];
                    }
                    if (entityData["entities"][entityId]["mesh"].contains("height")) {
                        this->height = entityData["entities"][entityId]["mesh"]["height"];
                    }
                    this->loadPrimitive("CONE", position);
                }
                else if (entityData["entities"][entityId]["mesh"]["mesh_type"] == "CAPSULE") {
                    if (entityData["entities"][entityId]["mesh"].contains("subdivisions")) {
                        this->subdivisions = entityData["entities"][entityId]["mesh"]["subdivisions"];
                    }
                    if (entityData["entities"][entityId]["mesh"].contains("width")) {
                        this->width = entityData["entities"][entityId]["mesh"]["width"];
                    }
                    if (entityData["entities"][entityId]["mesh"].contains("height")) {
                        this->height = entityData["entities"][entityId]["mesh"]["height"];
                    }
                    this->loadPrimitive("CAPSULE", position);
                }
            } else if (entityData["entities"][entityId]["mesh"]["type"] == "file") {
                std::string meshPath = gameFolder + "/" + entityData["entities"][entityId]["mesh"]["path"].get<std::string>();
                this->meshFilePath = meshPath;
                // Récupère l'extension du fichier
                std::string ext = meshPath.substr(meshPath.find_last_of('.') + 1);
                if (ext == "off" || ext == "OFF") {
                    this->load_OFF(meshPath);
                } else if (ext == "fbx" || ext == "FBX") {
                    this->loadFBX(meshPath);
                } else {
                    std::cerr << "Format de mesh non supporté : " << ext << std::endl;
                }
            }
        }
    }

    void load_OFF(const std::string& filename) {
        type=PrimitiveType::MESH;
        vertices.clear();
        indices.clear();
        triangles.clear();
        uvs.clear();
        normals.clear();

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

    void loadFBX(const std::string& filename) {
        type=PrimitiveType::MESH;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filename,
            aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);

        if (!scene || !scene->HasMeshes()) {
            std::cerr << "Failed to load FBX file: " << filename << std::endl;
            return;
        }

        vertices.clear();
        indices.clear();
        normals.clear();
        uvs.clear();

        const aiMesh* mesh = scene->mMeshes[0]; // Charge le premier mesh
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            vertices.push_back(glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            ));
            if (mesh->HasNormals()) {
                normals.push_back(glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                ));
            }
            if (mesh->HasTextureCoords(0)) {
                uvs.push_back(glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                ));
            }
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                indices.push_back(face.mIndices[j]);
            }
        }
        triangles.clear();
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];
            if (face.mNumIndices == 3) {
                std::vector<unsigned short> tri = {
                    static_cast<unsigned short>(face.mIndices[0]),
                    static_cast<unsigned short>(face.mIndices[1]),
                    static_cast<unsigned short>(face.mIndices[2])
                };
                triangles.push_back(tri);
            }
        }

        vertexCount = indices.size();

        // Génère les buffers OpenGL comme dans load_OFF
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenBuffers(1, &uvVBO);
        glGenBuffers(1, &normalVBO);
        glGenBuffers(1, &boneIDVBO);
        glGenBuffers(1, &boneWeightVBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), uvs.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);
        // BoneIDs
        glBindBuffer(GL_ARRAY_BUFFER, boneIDVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::ivec4), nullptr, GL_STATIC_DRAW);
        glVertexAttribIPointer(5, 4, GL_INT, 0, (void*)0);
        glEnableVertexAttribArray(5);

        // BoneWeights
        glBindBuffer(GL_ARRAY_BUFFER, boneWeightVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec4), nullptr, GL_STATIC_DRAW);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(6);


        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void loadPrimitive(const std::string& primitiveType, glm::vec3 right_vector=glm::vec3(1.0,0.0,0.0), glm::vec3 up_vector=glm::vec3(0.0,0.0,1.0), float rayon=1.0f) {
        vertices.clear();
        indices.clear();
        uvs.clear();
        normals.clear();
        if (primitiveType == "PLANE"){
            type=PrimitiveType::PLANE;
            centre=glm::vec3(0,0,0);
            m_bottom_left=glm::vec3(-0.5,0,-0.5);
            m_right_vector=right_vector;
            m_up_vector=up_vector;
            Plane plane(centre, right_vector, up_vector, glm::vec3(0,0,0));
            plane.createGridMesh(subdivisions, subdivisions, vertices, normals, uvs, indices);
        }
        else if (primitiveType == "SPHERE"){
            type=PrimitiveType::SPHERE;
            Sphere sphere(rayon);
            centre=glm::vec3(0,0,0);
            this->rayon=rayon;
            sphere.build_arrays(subdivisions, subdivisions, vertices, normals, uvs, indices);
        }
        // else if (primitiveType == "BOX"){
        //     type=PrimitiveType::MESH;
        //     Box box;
        //     box.build_arrays(vertices, normals, uvs, indices);
        // }
        // else if (primitiveType == "CYLINDER"){
        //     type=PrimitiveType::CYLINDER;
        //     Cylinder cylinder;
        //     cylinder.build_arrays(vertices, normals, uvs, indices, width, height, subdivisions);
        // }
        // else if (primitiveType == "CONE"){
        //     type=PrimitiveType::MESH;
        //     Cone cone;
        //     cone.build_arrays(vertices, normals, uvs, indices, width, height, subdivisions);
        // }
        // else if (primitiveType == "CAPSULE"){
        //     type=PrimitiveType::MESH;
        //     Capsule capsule;
        //     capsule.build_arrays(vertices, normals, uvs, indices, width, height, subdivisions);
        // }
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

    void clearGLBuffers() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
        if (uvVBO) glDeleteBuffers(1, &uvVBO);
        if (normalVBO) glDeleteBuffers(1, &normalVBO);
        if (boneIDVBO) glDeleteBuffers(1, &boneIDVBO);
        if (boneWeightVBO) glDeleteBuffers(1, &boneWeightVBO);
        VAO = VBO = EBO = uvVBO = normalVBO = boneIDVBO = boneWeightVBO = 0;
    }

    void renderEditor(){
        if(ImGui::CollapsingHeader("Mesh Component", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool needsRebuild = false;
            int meshTypeIdx = static_cast<int>(type);
            const char* meshTypes[] = { "PLANE", "CUBE", "SPHERE", "CYLINDER", "MESH" };
            if (ImGui::Combo("Type de mesh", &meshTypeIdx, meshTypes, IM_ARRAYSIZE(meshTypes))) {
                type = static_cast<PrimitiveType>(meshTypeIdx);
                clearGLBuffers();
                vertices.clear();
                indices.clear();
                normals.clear();
                uvs.clear();
                triangles.clear();
                meshFilePath.clear();
                switch(type) {
                    case PrimitiveType::PLANE:
                        loadPrimitive("PLANE", m_right_vector, m_up_vector, 1.0f);
                        break;
                    case PrimitiveType::SPHERE:
                        loadPrimitive("SPHERE", glm::vec3(1,0,0), glm::vec3(0,0,1), rayon);
                        break;
                    case PrimitiveType::MESH:
                        break;
                }
            }
            ImGui::Separator();
            
            switch(type) {
                case PrimitiveType::PLANE:
                    ImGui::Text("Plane Settings");
                    needsRebuild |= ImGui::DragFloat("Width", &width, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Height", &height, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Subdivisions", &subdivisions, 1.0f, 1.0f, 200.0f);
                    needsRebuild |= ImGui::DragFloat3("Right Vector", &m_right_vector.x, 0.01f);
                    needsRebuild |= ImGui::DragFloat3("Up Vector", &m_up_vector.x, 0.01f);
                    break;
                    
                case PrimitiveType::SPHERE:
                    ImGui::Text("Sphere Settings");
                    needsRebuild |= ImGui::DragFloat("Radius", &rayon, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Subdivisions", &subdivisions, 1.0f, 4.0f, 100.0f);
                    break;
                    
                case PrimitiveType::CYLINDER:
                    ImGui::Text("Cylinder Settings");
                    needsRebuild |= ImGui::DragFloat("Radius", &width, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Height", &height, 0.1f, 0.1f, 100.0f);
                    needsRebuild |= ImGui::DragFloat("Subdivisions", &subdivisions, 1.0f, 3.0f, 100.0f);
                    break;
                    
                case PrimitiveType::CUBE:
                    ImGui::Text("Cube (no parameters)");
                    break;
                    
                case PrimitiveType::MESH:
                    ImGui::Text("Mesh File (OFF/FBX):");
                    ImGui::BeginChild("MeshDropZone", ImVec2(0, 40), true, ImGuiWindowFlags_NoScrollbar);
                    ImGui::TextWrapped("%s", meshFilePath.empty() ? "Drag & drop a mesh file (.off/.fbx) here" : meshFilePath.c_str());
                    ImGui::EndChild();

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM")) {
                            const char* droppedPath = (const char*)payload->Data;
                            std::string ext = droppedPath;
                            ext = ext.substr(ext.find_last_of('.') + 1);
                            if (ext == "off" || ext == "OFF") {
                                meshFilePath = std::string(droppedPath);
                                type = PrimitiveType::MESH;
                                load_OFF(meshFilePath);
                                std::cout << "Loaded OFF file: " << meshFilePath << std::endl;
                            } else if (ext == "fbx" || ext == "FBX") {
                                meshFilePath = std::string(droppedPath);
                                type = PrimitiveType::MESH;
                                loadFBX(meshFilePath);
                                std::cout << "Loaded FBX file: " << meshFilePath << std::endl;
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    static char meshPathBuffer[256] = "";
                    strncpy(meshPathBuffer, meshFilePath.c_str(), sizeof(meshPathBuffer));
                    meshPathBuffer[sizeof(meshPathBuffer) - 1] = '\0';
                    ImGui::InputText("Mesh File Path", meshPathBuffer, sizeof(meshPathBuffer));
                    if (ImGui::Button("Load Mesh File")) {
                        std::string ext = meshPathBuffer;
                        ext = ext.substr(ext.find_last_of('.') + 1);
                        clearGLBuffers();
                        meshFilePath = std::string(meshPathBuffer);
                        if (ext == "off" || ext == "OFF") {
                            load_OFF(meshFilePath);
                            type = PrimitiveType::MESH;
                            std::cout << "Loaded OFF file: " << meshFilePath << std::endl;
                        } else if (ext == "fbx" || ext == "FBX") {
                            loadFBX(meshFilePath);
                            type = PrimitiveType::MESH;
                            std::cout << "Loaded FBX file: " << meshFilePath << std::endl;
                        }
                    }
                    break;
                default:
                    ImGui::Text("Unknown Primitive Type");
            }
            ImGui::Separator();
            ImGui::Text("Quick Load:");
            // if (ImGui::Button("Load Cube")) {
            //     loadPrimitive("BOX");
            //     type = PrimitiveType::CUBE;
            //     std::cout << "Loaded cube primitive" << std::endl;
            // }
            // ImGui::SameLine();
            
            if (ImGui::Button("Load Sphere")) {
                needsRebuild=true;
                type = PrimitiveType::SPHERE;
                rayon = 1.0f;
                subdivisions = 20.0f;
                std::cout << "Loaded sphere primitive" << std::endl;
            }
            ImGui::SameLine();
            
            if (ImGui::Button("Load Plane")) {
                needsRebuild=true;
                type = PrimitiveType::PLANE;
                m_right_vector = glm::vec3(1,0,0);
                m_up_vector = glm::vec3(0,0,1);
                width = 1.0f;
                height = 1.0f;
                subdivisions = 10.0f;
                std::cout << "Loaded plane primitive" << std::endl;
            }
            
            // if (ImGui::Button("Load Cylinder")) {
            //     loadPrimitive("CYLINDER");
            //     type = PrimitiveType::CYLINDER;
            //     width = 1.0f;
            //     height = 2.0f;
            //     subdivisions = 20.0f;
            //     std::cout << "Loaded cylinder primitive" << std::endl;
            // }
            // ImGui::SameLine();
            
            // if (ImGui::Button("Load Cone")) {
            //     loadPrimitive("CONE");
            //     type = PrimitiveType::MESH;
            //     width = 1.0f;
            //     height = 2.0f;
            //     subdivisions = 20.0f;
            //     std::cout << "Loaded cone primitive" << std::endl;
            // }
            // ImGui::SameLine();
            
            // if (ImGui::Button("Load Capsule")) {
            //     loadPrimitive("CAPSULE");
            //     type = PrimitiveType::MESH;
            //     width = 0.5f;
            //     height = 2.0f;
            //     subdivisions = 20.0f;
            //     std::cout << "Loaded capsule primitive" << std::endl;
            // }
            
            ImGui::Separator();
            
            if (needsRebuild) {
                std::cout << "Rebuilding mesh with new parameters..." << std::endl;
                
                switch(type) {
                    case PrimitiveType::PLANE:
                        clearGLBuffers();
                        loadPrimitive("PLANE", m_right_vector, m_up_vector, 1.0f);
                        break;
                    case PrimitiveType::SPHERE:
                        clearGLBuffers();
                        loadPrimitive("SPHERE", glm::vec3(1,0,0), glm::vec3(0,0,1), rayon);
                        std::cout << "Rebuilt sphere primitive" << std::endl;
                        std::cout << "Radius: " << rayon << ", Subdivisions: " << subdivisions << std::endl;
                        std::cout << "Vertices count: " << vertices.size() << std::endl;
                        break;
                    default:
                        break;
                }
            }
            ImGui::Separator();
            ImGui::Text("Mesh Statistics:");
            ImGui::Text("Type: %s", 
                type == PrimitiveType::PLANE ? "PLANE" :
                // type == PrimitiveType::CUBE ? "CUBE" :
                type == PrimitiveType::SPHERE ? "SPHERE" :
                // type == PrimitiveType::CYLINDER ? "CYLINDER" :
                type == PrimitiveType::MESH ? "MESH" : "UNKNOWN"
            );
            ImGui::Text("Mesh File Path: %s", meshFilePath.c_str());
            ImGui::Text("Vertices: %zu", vertices.size());
            ImGui::Text("Indices: %zu", indices.size());
            ImGui::Text("Triangles: %zu", indices.size() / 3);
            ImGui::Text("VAO: %u", VAO);

        }
    }

    json toJson() const {
        json j;
        j["type"] = (type == PrimitiveType::PLANE || type == PrimitiveType::CUBE || type == PrimitiveType::SPHERE || type == PrimitiveType::CYLINDER) ? "primitive" : "file";
        if (type == PrimitiveType::PLANE) {
            j["mesh_type"] = "PLANE";
        } else if (type == PrimitiveType::CUBE) {
            j["mesh_type"] = "CUBE";
        } else if (type == PrimitiveType::SPHERE) {
            j["mesh_type"] = "SPHERE";
        } else if (type == PrimitiveType::CYLINDER) {
            j["mesh_type"] = "CYLINDER";
        } else if (type == PrimitiveType::MESH) {
            j["mesh_type"] = "MESH";
        }
        j["path"] = meshFilePath;
        j["width"] = width;
        j["height"] = height;
        j["subdivisions"] = subdivisions;
        j["normal"] = { normal.x, normal.y, normal.z };
        return j;
    }
};
#endif // MESH_COMPONENT_H