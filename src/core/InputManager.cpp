#include "core/InputManager.h"
#include "core/Window.h"
#include <imgui.h>
#include <iostream>

InputManager::InputManager(Window *owner) : owner(owner)
{
    lastX = 0.0f;
    lastY = 0.0f;
    firstMouse = true;
    mouseCaptured = false; // Start with mouse visible (editor mode)

    // Ensure cursor is visible at start
    glfwSetInputMode(owner->GetNative(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void InputManager::BindKeyEvent(int key, int action, std::function<void()> callback)
{
    eventBindings[key].push_back({action, std::move(callback)});
}

void InputManager::BindKeyContinuous(int key, std::function<void(float)> callback)
{
    continuousBindings[key].push_back({std::move(callback)});
}

void InputManager::BindMouseMove(std::function<void(double, double)> callback)
{
    mouseCallbacks.push_back(std::move(callback));
}

void InputManager::BindScroll(std::function<void(double)> callback)
{
    scrollCallbacks.push_back(std::move(callback));
}

void InputManager::HandleKey(int key, int action, int mods)
{
    // Always track key state (for WASD while holding RMB)
    if (action == GLFW_PRESS)
        keysPressed[key] = true;
    else if (action == GLFW_RELEASE)
        keysPressed[key] = false;

    // If ImGui wants the keyboard (e.g. typing in a text box), don't process game shortcuts
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    // Call all event bindings that match this key+action
    auto it = eventBindings.find(key);
    if (it != eventBindings.end())
    {
        for (auto &bind : it->second)
        {
            if (bind.action == action)
            {
                bind.func();
            }
        }
    }
}

void InputManager::HandleMouseMove(double xpos, double ypos)
{
    if (!mouseCaptured)
        return;
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
        return; // no delta for first event
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; // inverted y

    lastX = xpos;
    lastY = ypos;

    for (auto &cb : mouseCallbacks)
        cb(xoffset, yoffset);
}

void InputManager::HandleScroll(double xoffset, double yoffset)
{
    if (!mouseCaptured)
        return;
    for (auto &cb : scrollCallbacks)
        cb(yoffset);
}

void InputManager::HandleMouseButton(int button, int action, int mods)
{
    // Right Mouse Button: Toggle mouse capture for camera rotation
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Only capture when not interacting with ImGui UI, or if already captured
        if (!ImGui::GetIO().WantCaptureMouse || mouseCaptured)
        {
            SetMouseCaptured(!mouseCaptured);
        }
    }
}

void InputManager::SetMouseCaptured(bool captured)
{
    if (captured && !mouseCaptured)
    {
        glfwSetInputMode(owner->GetNative(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        mouseCaptured = true;
        firstMouse = true;
    }
    else if (!captured && mouseCaptured)
    {
        glfwSetInputMode(owner->GetNative(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mouseCaptured = false;
    }
}

void InputManager::ToggleMouseCapture(bool forceEnable)
{
    SetMouseCaptured(forceEnable || !mouseCaptured);
}

void InputManager::Update(float deltaTime)
{
    if (!mouseCaptured)
        return;
    // For every key currently pressed, call its continuous bindings
    for (auto &p : keysPressed)
    {
        int key = p.first;
        bool pressed = p.second;
        if (!pressed)
            continue;

        auto it = continuousBindings.find(key);
        if (it == continuousBindings.end())
            continue;

        for (auto &bind : it->second)
        {
            bind.func(deltaTime);
        }
    }
}