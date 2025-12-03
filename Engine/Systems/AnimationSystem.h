#ifndef ANIMATION_SYSTEM_H
#define ANIMATION_SYSTEM_H

#include "EntityManager.h"
#include "../Components/AnimationComponent.h"
#include "../Components/MeshComponent.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/gtx/quaternion.hpp>
#include <GL/glew.h>
#include <iostream>
#include <algorithm>

class AnimationSystem {
public:
    EntityManager* entityManager = nullptr;

    AnimationSystem() = default;
    explicit AnimationSystem(EntityManager* em) : entityManager(em) {}

    void update(float deltaTime) {
        if (!entityManager) return;

        const auto& comps = entityManager->GetComponents<AnimationComponent>();
        for (const auto& [entity, _] : comps) {
            auto& animComp = entityManager->GetComponent<AnimationComponent>(entity);

            if (!animComp.isPlaying || animComp.animations.empty()) continue;
            if (animComp.currentAnimation < 0 || animComp.currentAnimation >= (int)animComp.animations.size()) continue;

            if (animComp.currentClip >= 0 && animComp.currentClip < (int)animComp.clips.size()) {
                UpdateClip(animComp, deltaTime);
            } else {
                UpdateFullAnimation(animComp, deltaTime);
            }

            CalculateBoneMatrices(animComp);
        }
    }

    void PlayClip(AnimationComponent& animComp, const std::string& clipName) {
        for (size_t i = 0; i < animComp.clips.size(); ++i) {
            if (animComp.clips[i].name == clipName) {
                PlayClip(animComp, (int)i);
                return;
            }
        }
        std::cerr << "AnimationSystem: Clip '" << clipName << "' not found\n";
    }

    void PlayClip(AnimationComponent& animComp, int clipIndex) {
        if (clipIndex < 0 || clipIndex >= (int)animComp.clips.size()) return;

        animComp.currentClip = clipIndex;
        const auto& clip = animComp.clips[clipIndex];

        animComp.currentAnimation = clip.animationIndex;
        if (!animComp.animations.empty() &&
            clip.animationIndex >= 0 &&
            clip.animationIndex < (int)animComp.animations.size()) {
            animComp.animationTime = (float)clip.start_frame; // ticks
        } else {
            animComp.animationTime = 0.0f;
        }

        animComp.isPlaying = true;
        std::cout << "AnimationSystem: Playing clip '" << clip.name << "'\n";
    }

private:
    void UpdateClip(AnimationComponent& animComp, float dt) {
        const auto& clip = animComp.clips[animComp.currentClip];
        const auto& anim = animComp.animations[animComp.currentAnimation];

        // Advance time in ticks
        animComp.animationTime += dt * animComp.animationSpeed * (float)anim.ticksPerSecond;

        const float clipStart = (float)clip.start_frame;
        const float clipEnd   = (float)clip.end_frame;

        // Clamp/loop inside clip range
        if (animComp.animationTime > clipEnd) {
            if (animComp.loop) animComp.animationTime = clipStart;
            else { animComp.animationTime = clipEnd; animComp.isPlaying = false; }
        } else if (animComp.animationTime < clipStart) {
            animComp.animationTime = clipStart;
        }
    }

    void UpdateFullAnimation(AnimationComponent& animComp, float deltaTime) {
        const auto& anim = animComp.animations[animComp.currentAnimation];

        animComp.animationTime += deltaTime * animComp.animationSpeed * (float)anim.ticksPerSecond;

        if (animComp.animationTime > (float)anim.duration) {
            if (animComp.loop) {
                animComp.animationTime = fmod(animComp.animationTime, (float)anim.duration);
            } else {
                animComp.animationTime = (float)anim.duration;
                animComp.isPlaying = false;
            }
        }
    }

    void CalculateBoneMatrices(AnimationComponent& animComp) {
        if (animComp.bones.empty()) return;
        if (animComp.currentAnimation < 0 || animComp.currentAnimation >= (int)animComp.animations.size()) return;

        Animation& anim = animComp.animations[animComp.currentAnimation];
        static bool printed = false;
        if (!printed) {
            std::cout << "[Anim] boneMapping size=" << animComp.boneMapping.size() << "\n";
            int n=0; for (auto& kv: animComp.boneMapping) { if(n++>10) break; std::cout << "  map: " << kv.first << "\n"; }
            const Animation& anim = animComp.animations[animComp.currentAnimation];
            n=0; for (auto& ch: anim.boneAnimations) { if(n++>10) break; std::cout << "  chan: " << ch.boneName << "\n"; }
            printed = true;
        }
        // Ensure final buffer size
        if (animComp.finalBoneMatrices.size() != animComp.bones.size()) {
            animComp.finalBoneMatrices.assign(animComp.bones.size(), glm::mat4(1.0f));
        } else {
            std::fill(animComp.finalBoneMatrices.begin(), animComp.finalBoneMatrices.end(), glm::mat4(1.0f));
        }

        // Traverse Assimp hierarchy
        glm::mat4 identity(1.0f);
        TraverseHierarchy(animComp, anim, &anim.rootNode, identity, animComp.animationTime);

        // Upload SSBO
        if (animComp.bonesSSBO != 0 && !animComp.finalBoneMatrices.empty()) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, animComp.bonesSSBO);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                            0,
                            animComp.finalBoneMatrices.size() * sizeof(glm::mat4),
                            animComp.finalBoneMatrices.data());
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, animComp.bonesSSBO); // binding must match shader
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }

    void TraverseHierarchy(AnimationComponent& animComp,
                        Animation& anim,
                        const AssimpNodeData* node,
                        const glm::mat4& parentGlobal,
                        float time) {
        if (!node) return;

        glm::mat4 nodeLocal = node->transformation;

        // Met à jour via channel si dispo
        BoneAnimation* chan = anim.FindBoneAnimation(node->name);
        if (chan) {
            chan->Update(time);
            nodeLocal = chan->GetLocalTransform();
        }

        glm::mat4 global = parentGlobal * nodeLocal;

        // Écrit la matrice finale si ce node correspond à un bone du mesh
        auto it = animComp.boneMapping.find(node->name);
        if (it != animComp.boneMapping.end()) {
            int idx = it->second;
            if (idx >= 0 && idx < (int)animComp.bones.size()) {
                animComp.finalBoneMatrices[idx] = global * animComp.bones[idx].offsetMatrix;
            }
        }

        for (int i = 0; i < node->childrenCount; ++i) {
            TraverseHierarchy(animComp, anim, &node->children[i], global, time);
        }
    }
};

#endif // ANIMATION_SYSTEM_H