#ifndef CONTROLLER_SYSTEM_H
#define CONTROLLER_SYSTEM_H

#include <unordered_map>
#include <GLFW/glfw3.h>
#include "../Components/ControllerComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/CameraComponent.h"
#include "../Components/ScriptComponent.h"
#include "../Components/LuaScriptComponent.h"
#include "../Components/InputEvent.h"
#include "../Systems/ScriptSystem.h"

class ControllerSystem {
public:
    InputEvent event;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool rightMouseDown = false;

    void onCreate(GLFWwindow* window){
        // Stocker un pointeur vers ce système pour les callbacks
        glfwSetWindowUserPointer(window, this);

        // Scroll callback
        glfwSetScrollCallback(window, [](GLFWwindow* win, double xoffset, double yoffset){
            ControllerSystem* cs = static_cast<ControllerSystem*>(glfwGetWindowUserPointer(win));
            if(cs){
                cs->event.scroll += yoffset;
            }
        });
    }

    void update(GLFWwindow* window, float dt){
        event.dt = dt;
        event.buttons.clear();
        event.mouseDeltaX = 0.0;
        event.mouseDeltaY = 0.0;
        event.mouseMoved = false;
        // scrollY n’est pas remis à zéro ici, pour que le callback puisse l’ajouter avant lecture

        // Touches
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) event.buttons.push_back("Forward");
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) event.buttons.push_back("Backward");
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) event.buttons.push_back("Left");
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) event.buttons.push_back("Right");
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) event.buttons.push_back("C");
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) event.buttons.push_back("R");
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) event.buttons.push_back("F");
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) event.buttons.push_back("LeftMouse");

        // Souris clic droit + mouvement
        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS){
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            if(!rightMouseDown){
                rightMouseDown = true;
                lastMouseX = xpos;
                lastMouseY = ypos;
            }
            event.mouseDeltaX = xpos - lastMouseX;
            event.mouseDeltaY = lastMouseY - ypos;
            event.mouseMoved = (event.mouseDeltaX != 0.0 || event.mouseDeltaY != 0.0);

            lastMouseX = xpos;
            lastMouseY = ypos;

            event.buttons.push_back("RightMouse");
        } else {
            rightMouseDown = false;
        }
        if(scriptSystem) {
            scriptSystem->onInput(event);
        }
        if(renderSystem) {
            renderSystem->onInput(event);
        }
        dispatch(event);
        event.scroll = 0.0; // reset scroll after reading
    }

    void dispatch(const InputEvent& event){
        for(auto sub : subs){
            sub->onInput(event);
        }
    }

    void subscribe(ScriptComponent* script){
        subs.push_back(script);
    }
    void setScriptSystem(ScriptSystem* system) {
        scriptSystem = system;
    }
    void setRenderSystem(RenderSystem* system) {
        renderSystem = system;
    }
    private :
        std::vector<ScriptComponent*> subs;
        ScriptSystem* scriptSystem = nullptr;
        RenderSystem* renderSystem = nullptr;

    
};
#endif // CONTROLLER_SYSTEM_H