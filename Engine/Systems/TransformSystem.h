#include <unordered_map>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../Components/TransformComponent.h"

class TransformSystem {
public:
    EntityManager* entityManager;

    TransformSystem(EntityManager* em) : entityManager(em) {}

    void update() {
        for (auto& [id, t] : entityManager->GetComponents<TransformComponent>()) {
            if (t.parent == UINT32_MAX) {
                updateWorldMatrix(id);
            }
        }
    }

    void setParent(uint32_t childId, uint32_t parentId) {
        auto& child = entityManager->GetComponent<TransformComponent>(childId);
        if (child.parent != UINT32_MAX) {
            auto& oldParent = entityManager->GetComponent<TransformComponent>(child.parent);
            oldParent.children.erase(
                std::remove(oldParent.children.begin(), oldParent.children.end(), childId),
                oldParent.children.end()
            );
        }
        child.parent = parentId;
        if (parentId != UINT32_MAX) {
            entityManager->GetComponent<TransformComponent>(parentId).children.push_back(childId);
        }
    }

    void removeParent(uint32_t childId) {
        setParent(childId, UINT32_MAX);
    }

private:
    void updateWorldMatrix(uint32_t id) {
        auto& t = entityManager->GetComponent<TransformComponent>(id);
        t.localMatrix = glm::translate(glm::mat4(1.0f), t.position)
                * glm::mat4_cast(glm::quat(glm::radians(t.rotation)))
                * glm::scale(glm::mat4(1.0f), t.scale);

        if (t.parent != UINT32_MAX) {
            t.worldMatrix = entityManager->GetComponent<TransformComponent>(t.parent).worldMatrix * t.localMatrix;
        } else {
            t.worldMatrix = t.localMatrix;
        }
        for (uint32_t childId : t.children) {
            updateWorldMatrix(childId);
        }
    }
};
