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
#include "../Systems/Dispatcher.h"

class ControllerSystem {
public:
    InputEvent event;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool rightMouseDown = false;
    Dispatcher& dispatcher;
    

    ControllerSystem(Dispatcher& disp) : dispatcher(disp) {}

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

        // --- Touches principales (WASD, etc.) ---
        const std::vector<std::pair<int, std::string>> keyMap = {
            {GLFW_KEY_W, "Forward"}, {GLFW_KEY_S, "Backward"},
            {GLFW_KEY_A, "Left"},    {GLFW_KEY_D, "Right"},
            {GLFW_KEY_C, "C"},       {GLFW_KEY_R, "R"},
            {GLFW_KEY_F, "F"},       {GLFW_KEY_E, "E"},
            {GLFW_KEY_Q, "Q"},       {GLFW_KEY_SPACE, "Space"},
            {GLFW_KEY_LEFT_SHIFT, "Shift"}, {GLFW_KEY_RIGHT_SHIFT, "Shift"},
            {GLFW_KEY_LEFT_CONTROL, "Ctrl"}, {GLFW_KEY_RIGHT_CONTROL, "Ctrl"},
            {GLFW_KEY_LEFT_ALT, "Alt"}, {GLFW_KEY_RIGHT_ALT, "Alt"},
            {GLFW_KEY_TAB, "Tab"},   {GLFW_KEY_ESCAPE, "Escape"},
            {GLFW_KEY_UP, "Up"},     {GLFW_KEY_DOWN, "Down"},
            {GLFW_KEY_LEFT, "ArrowLeft"}, {GLFW_KEY_RIGHT, "ArrowRight"}
        };
        for(const auto& [key, name] : keyMap){
            if(glfwGetKey(window, key) == GLFW_PRESS)
                event.buttons.push_back({name, STATE::PRESSED});
            if(glfwGetKey(window, key) == GLFW_RELEASE)
                event.buttons.push_back({name, STATE::RELEASED});
            if(glfwGetKey(window, key) == GLFW_REPEAT)
                event.buttons.push_back({name, STATE::REPEAT});
        }

        // --- Toutes les touches alphanumériques (A-Z, 0-9) ---
        for(int k = GLFW_KEY_0; k <= GLFW_KEY_9; ++k) {
            if(glfwGetKey(window, k) == GLFW_PRESS)
                event.buttons.push_back(std::make_pair(std::string(1, '0' + (k - GLFW_KEY_0)), STATE::PRESSED));
            if(glfwGetKey(window, k) == GLFW_RELEASE)
                event.buttons.push_back(std::make_pair(std::string(1, '0' + (k - GLFW_KEY_0)), STATE::RELEASED));
            if(glfwGetKey(window, k) == GLFW_REPEAT)
                event.buttons.push_back(std::make_pair(std::string(1, '0' + (k - GLFW_KEY_0)), STATE::REPEAT));
        }
        for(int k = GLFW_KEY_A; k <= GLFW_KEY_Z; ++k) {
            if(glfwGetKey(window, k) == GLFW_PRESS)
                event.buttons.push_back(std::make_pair(std::string(1, 'A' + (k - GLFW_KEY_A)), STATE::PRESSED));
            if(glfwGetKey(window, k) == GLFW_RELEASE)
                event.buttons.push_back(std::make_pair(std::string(1, 'A' + (k - GLFW_KEY_A)), STATE::RELEASED));
            if(glfwGetKey(window, k) == GLFW_REPEAT)
                event.buttons.push_back(std::make_pair(std::string(1, 'A' + (k - GLFW_KEY_A)), STATE::REPEAT));
        }
        // --- Souris boutons ---
        const std::vector<std::pair<int, std::string>> mouseMap = {
            {GLFW_MOUSE_BUTTON_LEFT, "LeftMouse"},
            {GLFW_MOUSE_BUTTON_RIGHT, "RightMouse"},
            {GLFW_MOUSE_BUTTON_MIDDLE, "MiddleMouse"}
        };
        for(const auto& [btn, name] : mouseMap){
            if(glfwGetMouseButton(window, btn) == GLFW_PRESS)
                event.buttons.push_back({name, STATE::PRESSED});
            if(glfwGetMouseButton(window, btn) == GLFW_RELEASE)
                event.buttons.push_back({name, STATE::RELEASED});
            if(glfwGetMouseButton(window, btn) == GLFW_REPEAT)
                event.buttons.push_back({name, STATE::REPEAT});
        }

        // --- Mouvement souris (toujours détecté, même sans clic) ---
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        event.mouseDeltaX = xpos - lastMouseX;
        event.mouseDeltaY = lastMouseY - ypos;
        event.mouseMoved = (event.mouseDeltaX != 0.0 || event.mouseDeltaY != 0.0);
        lastMouseX = xpos;
        lastMouseY = ypos;

        // --- Scroll (déjà géré via callback) ---

        // --- Dispatch à tous les scripts abonnés ---
        dispatcher.dispatch(event);
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