#include "core/InputManager.h"
#include "core/Window.h"
#include <iostream>

InputManager::InputManager(Window* owner) : owner(owner)
{
	lastX = 0.0f;
	lastY = 0.0f;
	firstMouse = true;
    mouseCaptured = true;
}

void InputManager::BindKeyEvent(int key, int action, std::function<void()> callback) {
	eventBindings[key].push_back({ action, std::move(callback) });
}

void InputManager::BindKeyContinuous(int key, std::function<void(float)> callback) {
	continuousBindings[key].push_back({ std::move(callback) });
}

void InputManager::BindMouseMove(std::function<void(double, double)> callback) {
	mouseCallbacks.push_back(std::move(callback));
}

void InputManager::BindScroll(std::function<void(double)> callback) {
	scrollCallbacks.push_back(std::move(callback));
}

void InputManager::HandleKey(int key, int action, int mods) {
    if (!mouseCaptured) return;
    // maintain pressed state for continuous handling
    if (action == GLFW_PRESS) keysPressed[key] = true;
    else if (action == GLFW_RELEASE) keysPressed[key] = false;

    // call all event bindings that match this key+action
    auto it = eventBindings.find(key);
    if (it != eventBindings.end()) {
        for (auto& bind : it->second) {
            if (bind.action == action) {
                bind.func();
            }
        }
    }
}

void InputManager::HandleMouseMove(double xpos, double ypos) {
    if (!mouseCaptured) return;
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return; // no delta for first event
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; // inverted y

    lastX = xpos;
    lastY = ypos;

    for (auto& cb : mouseCallbacks) cb(xoffset, yoffset);
}

void InputManager::HandleScroll(double xoffset, double yoffset) {
    if (!mouseCaptured) return;
    for (auto& cb : scrollCallbacks) cb(yoffset);
}

void InputManager::HandleMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !mouseCaptured) {
        ToggleMouseCapture(true); // recapture
    }

    if (!mouseCaptured)
        return;
}

void InputManager::ToggleMouseCapture(bool forceEnable) {
    if (forceEnable || !mouseCaptured) {
        glfwSetInputMode(owner->GetNative(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
        firstMouse = true;
        std::cout << "Mouse captured\n";
    }
    else {
        glfwSetInputMode(owner->GetNative(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured = false;
        std::cout << "Mouse released\n";
    }
}

void InputManager::Update(float deltaTime) {
    if (!mouseCaptured) return;
    // For every key currently pressed, call its continuous bindings
    for (auto& p : keysPressed) {
        int key = p.first;
        bool pressed = p.second;
        if (!pressed) continue;

        auto it = continuousBindings.find(key);
        if (it == continuousBindings.end()) continue;

        for (auto& bind : it->second) {
            bind.func(deltaTime);
        }
    }
}