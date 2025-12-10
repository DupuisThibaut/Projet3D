#ifndef ANIMATION_COMPONENT_H
#define ANIMATION_COMPONENT_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <common/json.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <map>

class AssimpGLMHelpers {
public:
    static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from) {
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    static inline glm::vec3 GetGLMVec(const aiVector3D& vec) {
        return glm::vec3(vec.x, vec.y, vec.z);
    }

    static inline glm::quat GetGLMQuat(const aiQuaternion& q) {
        return glm::quat(q.w, q.x, q.y, q.z);
    }
};

// Structure pour un bone
struct Bone {
    std::string name;
    int parentIndex;
    glm::mat4 offsetMatrix;
    glm::mat4 localTransform;
};

// Keyframes d'un bone dans une animation
class BoneAnimation {
    public:
        std::string boneName;
        std::vector<std::pair<double, glm::vec3>> positions;  // time, position
        std::vector<std::pair<double, glm::quat>> rotations;  // time, rotation
        std::vector<std::pair<double, glm::vec3>> scales;     // time, scale
        int m_numPosition;
        int m_numRotation;
        int m_numScaling;
        glm::mat4 m_localTransform;
        int m_ID;
        BoneAnimation() = default;
        BoneAnimation(const std::string& name) : boneName(name), m_numPosition(0), m_numRotation(0), m_numScaling(0) {}
        BoneAnimation(const std::string& name, int id, const aiNodeAnim* channel) : boneName(name), m_ID(id), m_localTransform(glm::mat4(1.0f))     {
        m_numPosition = channel->mNumPositionKeys;

        for (int positionIndex = 0; positionIndex < m_numPosition; ++positionIndex)
        {
            aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
            double timeStamp = channel->mPositionKeys[positionIndex].mTime;
            

            glm::vec3 position = AssimpGLMHelpers::GetGLMVec(aiPosition);
            positions.push_back({timeStamp, position});
        }

        m_numRotation = channel->mNumRotationKeys;
        for (int rotationIndex = 0; rotationIndex < m_numRotation; ++rotationIndex)
        {
            aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
            double timeStamp = channel->mRotationKeys[rotationIndex].mTime;
            glm::quat rotation = AssimpGLMHelpers::GetGLMQuat(aiOrientation);
            rotations.push_back({timeStamp, rotation});
        }

        m_numScaling = channel->mNumScalingKeys;
        for (int keyIndex = 0; keyIndex < m_numScaling; ++keyIndex)
        {
            aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
            double timeStamp = channel->mScalingKeys[keyIndex].mTime;
            glm::vec3 scaleVec = AssimpGLMHelpers::GetGLMVec(scale);
            scales.push_back({timeStamp, scaleVec});
        }
    }

    glm::mat4 GetLocalTransform() { return m_localTransform; }
    std::string GetBoneName() const { return boneName; }
    int GetBoneID() { return m_ID; }

    int GetPositionIndex(float animationTime) {
        if (m_numPosition <= 1) return 0;
        for (int index = 0; index < m_numPosition - 1; ++index)
            if (animationTime < positions[index + 1].first) return index;
        return m_numPosition - 1;
    }
    int GetRotationIndex(float animationTime) {
        if (m_numRotation <= 1) return 0;
        for (int index = 0; index < m_numRotation - 1; ++index)
            if (animationTime < rotations[index + 1].first) return index;
        return m_numRotation - 1;
    }
    int GetScaleIndex(float animationTime) {
        if (m_numScaling <= 1) return 0;
        for (int index = 0; index < m_numScaling - 1; ++index)
            if (animationTime < scales[index + 1].first) return index;
        return m_numScaling - 1;
    }

    /*interpolates  b/w positions,rotations & scaling keys based on the curren time of 
    the animation and prepares the local transformation matrix by combining all keys 
    tranformations*/
    void Update(float animationTime)
    {
        glm::mat4 translation = InterpolatePosition(animationTime);
        glm::mat4 rotation = InterpolateRotation(animationTime);
        glm::mat4 scale = InterpolateScaling(animationTime);
        m_localTransform = translation * rotation * scale;
    }

    
     /* Gets normalized value for Lerp & Slerp*/
    float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
    {
        float framesDiff = nextTimeStamp - lastTimeStamp;
        if (framesDiff == 0.0f) return 0.0f;
        float midWayLength = animationTime - lastTimeStamp;
        return midWayLength / framesDiff;
    }

     /*figures out which position keys to interpolate b/w and performs the interpolation 
    and returns the translation matrix*/
    glm::mat4 InterpolatePosition(float animationTime)
    {
        if (1 == m_numPosition)
            return glm::translate(glm::mat4(1.0f), positions[0].second);

        int p0Index = GetPositionIndex(animationTime);
        int p1Index = std::min(p0Index + 1, m_numPosition - 1);
        float scaleFactor = GetScaleFactor(positions[p0Index].first,
            positions[p1Index].first, animationTime);
        glm::vec3 finalPosition = glm::mix(positions[p0Index].second,
            positions[p1Index].second, scaleFactor);
        return glm::translate(glm::mat4(1.0f), finalPosition);
    }

    /*figures out which rotations keys to interpolate b/w and performs the interpolation 
    and returns the rotation matrix*/
    glm::mat4 InterpolateRotation(float animationTime)
    {
        if (1 == m_numRotation)
        {
            auto rotation = glm::normalize(rotations[0].second);
            return glm::toMat4(rotation);
        }

        int p0Index = GetRotationIndex(animationTime);
        int p1Index = std::min(p0Index + 1, m_numRotation - 1);
        float scaleFactor = GetScaleFactor(rotations[p0Index].first,
            rotations[p1Index].first, animationTime);
        glm::quat finalRotation = glm::slerp(rotations[p0Index].second,
            rotations[p1Index].second, scaleFactor);
        finalRotation = glm::normalize(finalRotation);
        return glm::toMat4(finalRotation);
    }

    /*figures out which scaling keys to interpolate b/w and performs the interpolation 
    and returns the scale matrix*/
    glm::mat4 InterpolateScaling(float animationTime)
    {
        if (1 == m_numScaling)
            return glm::scale(glm::mat4(1.0f), scales[0].second);

        int p0Index = GetScaleIndex(animationTime);
        int p1Index = std::min(p0Index + 1, m_numScaling - 1);
        float scaleFactor = GetScaleFactor(scales[p0Index].first,
            scales[p1Index].first, animationTime);
        glm::vec3 finalScale = glm::mix(scales[p0Index].second, scales[p1Index].second
            , scaleFactor);
        return glm::scale(glm::mat4(1.0f), finalScale);
    }

};

struct AssimpNodeData
{
    glm::mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

struct AnimationClip {
    std::string name;
    int start_frame;
    int end_frame;
    float duration;
    std::string fbxPath;
    int animationIndex;
};

struct Animation {
    std::string name;
    double duration;
    double ticksPerSecond;
    std::vector<BoneAnimation> boneAnimations;
    AssimpNodeData rootNode;
    std::map<std::string, Bone> BoneInfoMap;
    glm::mat4 globalInverseTransform = glm::mat4(1.0f);

    public :
    Animation() = default;

    BoneAnimation* FindBoneAnimation(const std::string& name)
    {
        auto iter = std::find_if(boneAnimations.begin(), boneAnimations.end(),
            [&](const BoneAnimation& boneAnim)
            {
                return boneAnim.GetBoneName() == name;
            }
        );
        if (iter == boneAnimations.end()) return nullptr;
        else return &(*iter);
    }

    inline float GetTicksPerSecond() { return ticksPerSecond; }

    inline float GetDuration() { return duration;}

    inline const AssimpNodeData& GetRootNode() { return rootNode; }
    inline const std::map<std::string,Bone>& GetBoneIDMap() 
    { 
        return BoneInfoMap;
    }

    void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
    {
        assert(src);

        dest.name = src->mName.data;
        dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
        dest.childrenCount = src->mNumChildren;

        for (int i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            ReadHierarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }


};

struct AnimationComponent {
    std::vector<Bone> bones;
    std::map<std::string, int> boneMapping;
    int m_numBones = 0;
    std::vector<Animation> animations;
    std::vector<AnimationClip> clips;
    std::string fbxPath;
    std::string gameFolder;
    std::vector<glm::mat4> finalBoneMatrices;
    
    std::vector<glm::ivec4> boneIDs;
    std::vector<glm::vec4> boneWeights;
    
    GLuint boneIDVBO = 0;
    GLuint boneWeightVBO = 0;
    GLuint bonesSSBO = 0;
    
    int currentAnimation = 0;
    int currentClip = -1;
    float animationTime = 0.0f;
    float animationSpeed = 1.0f;
    bool loop = true;
    bool isPlaying = false;

    auto& GetBoneMap(){return bones;}
    int& GetNumBones(){return m_numBones;}

    bool loadFromFBX(const nlohmann::json& entityData, const std::string& folder) {
        this->gameFolder = folder;
        bones.clear();
        animations.clear();
        boneMapping.clear();

        std::string meshPath;
        if(entityData.contains("mesh") && entityData["mesh"].contains("type") && entityData["mesh"]["type"] == "file") {
            meshPath = folder + "/" + entityData["mesh"]["path"].get<std::string>();
            std::string ext = meshPath.substr(meshPath.find_last_of('.') + 1);
            if (!(ext == "fbx" || ext == "FBX")) {
                std::cerr << "AnimationComponent: mesh is not FBX format." << std::endl;
                return false;
            }
        } else {
            std::cerr << "AnimationComponent: mesh path not found in JSON." << std::endl;
            return false;
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(meshPath,aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices);
        if (!scene) {
            std::cerr << "AnimationComponent: Failed to load FBX: " << meshPath << std::endl;
            return false;
        }

        // ═══════════════════════════════════════════════════════════════
        // 1. CHARGER LES BONES ET SKINNING
        // ═══════════════════════════════════════════════════════════════
        const aiMesh* mesh = scene->mMeshes[0];
        size_t numVertices = mesh->mNumVertices;

        std::cout << "AnimationComponent: Loading mesh 0 with " << numVertices << " vertices" << std::endl;

        boneIDs.resize(numVertices, glm::ivec4(-1, -1, -1, -1));
        boneWeights.resize(numVertices, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        // Charger les bones de ce mesh
        for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
            const aiBone* aiBone = mesh->mBones[i];
            std::string boneName = aiBone->mName.C_Str();
            
            int boneIndex;
            if (boneMapping.find(boneName) == boneMapping.end()) {
                boneIndex = bones.size();
                boneMapping[boneName] = boneIndex;
                
                Bone bone;
                bone.name = boneName;
                bone.parentIndex = -1;
                bone.offsetMatrix = glm::transpose(glm::make_mat4(&aiBone->mOffsetMatrix.a1));
                bone.localTransform = glm::mat4(1.0f);
                bones.push_back(bone);
            } else {
                boneIndex = boneMapping[boneName];
            }
            
            // Assigner les weights
            for (unsigned int j = 0; j < aiBone->mNumWeights; j++) {
                unsigned int vertexID = aiBone->mWeights[j].mVertexId;
                float weight = aiBone->mWeights[j].mWeight;
                
                for (int k = 0; k < 4; k++) {
                    if (boneIDs[vertexID][k] == -1) {
                        boneIDs[vertexID][k] = boneIndex;
                        boneWeights[vertexID][k] = weight;
                        break;
                    }
                }
            }
        }
        
        for (size_t v = 0; v < boneWeights.size(); ++v) {
            float sum = boneWeights[v].x + boneWeights[v].y + boneWeights[v].z + boneWeights[v].w;
            if (sum > 0.0f) {
                boneWeights[v] /= sum;
            } else {
                boneIDs[v]     = glm::ivec4(0, -1, -1, -1);
                boneWeights[v] = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            }
        }
                // DEBUG: verify bone distribution
        std::map<int, int> boneUsageCount;
        for (const auto& ids : boneIDs) {
            for (int k = 0; k < 4; ++k) {
                if (ids[k] >= 0) boneUsageCount[ids[k]]++;
            }
        }
        std::cout << "[BoneDistribution] Bones used:\n";
        int shown = 0;
        for (const auto& [boneID, count] : boneUsageCount) {
            std::cout << "  Bone " << boneID << ": " << count << " vertices\n";
            if (++shown >= 10) break; // limite à 10 pour ne pas spammer
        }

        
        if (!boneIDs.empty()) {
            std::cout << "AnimationComponent: Created VBOs for " << boneIDs.size() << " vertices" << std::endl;
            std::cout << "AnimationComponent: Loaded " << bones.size() << " bones" << std::endl;
        }
 
        // ═══════════════════════════════════════════════════════════════
        // 2. CHARGER LES ANIMATIONS ET KEYFRAMES
        // ═══════════════════════════════════════════════════════════════
        for (unsigned int animIdx = 0; animIdx < scene->mNumAnimations; ++animIdx) {
            const aiAnimation* aiAnim = scene->mAnimations[animIdx];
            
            Animation anim;
            anim.name = aiAnim->mName.C_Str();
            anim.duration = aiAnim->mDuration;
            anim.ticksPerSecond = aiAnim->mTicksPerSecond != 0 ? aiAnim->mTicksPerSecond : 30.0;
            anim.ReadHierarchyData(anim.rootNode, scene->mRootNode);
            glm::mat4 root = AssimpGLMHelpers::ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation);
            anim.globalInverseTransform = glm::inverse(root);
            
            for (unsigned int i = 0; i < aiAnim->mNumChannels; ++i) {
                const aiNodeAnim* channel = aiAnim->mChannels[i];
                BoneAnimation boneAnim(channel->mNodeName.C_Str(), i, channel);
                anim.boneAnimations.push_back(boneAnim);
            }
            anim.BoneInfoMap.clear();
            for (const auto& bone : bones) {
                anim.BoneInfoMap[bone.name] = bone;
                anim.BoneInfoMap[bone.name].localTransform = glm::mat4(1.0f);
                anim.BoneInfoMap[bone.name].offsetMatrix = bone.offsetMatrix;
                anim.BoneInfoMap[bone.name].parentIndex = bone.parentIndex;
                anim.BoneInfoMap[bone.name].name = bone.name;
            }
            
            animations.push_back(anim);
            std::cout << "AnimationComponent: Loaded animation '" << anim.name 
                      << "' with " << anim.boneAnimations.size() << " bone channels" << std::endl;
        }

        fbxPath = entityData["mesh"]["path"].get<std::string>();
        
        // ═══════════════════════════════════════════════════════════════
        // 3. CHARGER LES CLIPS
        // ═══════════════════════════════════════════════════════════════
        if (entityData.contains("animation") && entityData["animation"].contains("clips")) {
            clips.clear();
            double ticksPerSecond = animations.empty() ? 30.0 : animations[0].ticksPerSecond;
            
            for (const auto& clipData : entityData["animation"]["clips"]) {
                AnimationClip clip;
                clip.name = clipData["name"];
                
                if (clipData.contains("fbx_path")) {
                    clip.fbxPath = clipData["fbx_path"].get<std::string>();
                    clip.animationIndex = clipData.value("animation_index", 0);
                    clip.start_frame = 0;
                    clip.end_frame = 0;
                    
                    Assimp::Importer tempImporter;
                    const aiScene* tempScene = tempImporter.ReadFile(folder + "/" + clip.fbxPath, aiProcess_Triangulate);
                    if (tempScene && tempScene->mNumAnimations > clip.animationIndex) {
                        const aiAnimation* aiAnim = tempScene->mAnimations[clip.animationIndex];
                        double tps = aiAnim->mTicksPerSecond != 0 ? aiAnim->mTicksPerSecond : 30.0;
                        clip.duration = aiAnim->mDuration / tps;
                    }
                    
                    std::cout << "AnimationComponent: Clip '" << clip.name 
                              << "' from FBX: " << clip.fbxPath << std::endl;
                } else {
                    clip.start_frame = clipData["start_frame"];
                    clip.end_frame = clipData["end_frame"];
                    clip.duration = (clip.end_frame - clip.start_frame) / ticksPerSecond;
                    clip.fbxPath = "";
                    clip.animationIndex = 0;
                    
                    std::cout << "AnimationComponent: Clip '" << clip.name 
                              << "' [" << clip.start_frame << "-" << clip.end_frame << "]" << std::endl;
                }
                
                clips.push_back(clip);
            }
        }
        finalBoneMatrices.resize(bones.size(), glm::mat4(1.0f));
        
        std::cout << "AnimationComponent: Loaded " << animations.size() << " animation(s), " 
                  << bones.size() << " bones from FBX" << std::endl;
        return true;
    }
    void setupBoneVBOs(GLuint meshVAO) {
        if (boneIDs.empty() || boneWeights.empty()) return;
        glBindVertexArray(meshVAO);
        
        glGetVertexAttribiv(5, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, (GLint*)&boneIDVBO);
        glGetVertexAttribiv(6, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, (GLint*)&boneWeightVBO);

        glBindVertexArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, boneIDVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, boneIDs.size() * sizeof(glm::ivec4), boneIDs.data());
        glBindBuffer(GL_ARRAY_BUFFER, boneWeightVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, boneWeights.size() * sizeof(glm::vec4), boneWeights.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Créer le SSBO pour les matrices (vide pour l'instant)
        glGenBuffers(1, &bonesSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, bones.size() * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        std::cout << "AnimationComponent: VBO IDs - BoneIDs: " << boneIDVBO 
                << ", BoneWeights: " << boneWeightVBO 
                << ", SSBO: " << bonesSSBO << std::endl;
       
    }

    bool loadFromJson(const nlohmann::json& entityData, const std::string& folder) {
        if(!entityData.contains("animation")) {
            return false;
        }
        
        const auto& animData = entityData["animation"];
        currentAnimation = animData.value("current_animation", 0);
        currentClip = animData.value("current_clip", -1);
        isPlaying = animData.value("is_playing", false); 
        animationSpeed = animData.value("speed", 1.0f);
        loop = animData.value("loop", true);
        
        return loadFromFBX(entityData, folder);
    }
    
    ~AnimationComponent() {
        if (boneIDVBO != 0) glDeleteBuffers(1, &boneIDVBO);
        if (boneWeightVBO != 0) glDeleteBuffers(1, &boneWeightVBO);
    }


    void renderEditor() {
            ImGui::Text("Is Playing: %s", isPlaying ? "Yes" : "No");
            ImGui::Text("Animation Speed: %.2f", animationSpeed);
            ImGui::Text("Loop: %s", loop ? "Yes" : "No");
            ImGui::Text("Current Animation: %d", currentAnimation);
            ImGui::Text("Current Clip: %d", currentClip);
            ImGui::Separator();

            ImGui::Text("Clips:");
            for (size_t i = 0; i < clips.size(); ++i) {
                ImGui::PushID((int)i);
                ImGui::Separator();
                char nameBuf[128];
                strncpy(nameBuf, clips[i].name.c_str(), sizeof(nameBuf));
                if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
                    clips[i].name = std::string(nameBuf);
                }
                ImGui::InputInt("Start Frame", &clips[i].start_frame);
                ImGui::InputInt("End Frame", &clips[i].end_frame);

                if (ImGui::Button("Supprimer ce clip")) {
                    clips.erase(clips.begin() + i);
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }

            if (ImGui::Button("Ajouter un clip")) {
                AnimationClip newClip;
                newClip.name = "NewClip";
                newClip.start_frame = 0;
                newClip.end_frame = 0;
                newClip.duration = 0.0f;
                newClip.fbxPath = "";
                newClip.animationIndex = 0;
                clips.push_back(newClip);
            }
    }

    json toJson() {
        nlohmann::json animData;
        animData["current_animation"] = currentAnimation;
        animData["current_clip"] = currentClip;
        animData["is_playing"] = isPlaying;
        animData["speed"] = animationSpeed;
        animData["loop"] = loop;

        nlohmann::json clipsJson = nlohmann::json::array();
        for (const auto& clip : clips) {
            nlohmann::json clipJson;
            clipJson["name"] = clip.name;
            clipJson["start_frame"] = clip.start_frame;
            clipJson["end_frame"] = clip.end_frame;
            if (!clip.fbxPath.empty()) {
                clipJson["fbx_path"] = clip.fbxPath.substr(gameFolder.length() + 1); // relative path
                clipJson["animation_index"] = clip.animationIndex;
            }
            clipsJson.push_back(clipJson);
        }
        animData["clips"] = clipsJson;

        return animData;
    }
};

#endif // ANIMATION_COMPONENT_H