#pragma once

#include <string>
#include <thirdparty/glad/glad.h>
#include <thirdparty/GLFW/glfw3.h>
#include <memory>
#include "state/appState.h"

class InputManager;

class Window
{
public:
	Window(float width = 800, float height = 600, const std::string& name = "window");
	~Window();

	bool ShouldClose() const;
	void PollEvents() const;
	void SwapBuffers() const;
	GLFWwindow* GetNative() const;
	int Width() const;
	int Height() const;

	void SetAppState(AppState* app) { appState = app; }
	AppState* GetAppState() const { return appState; }
	InputManager* GetInputManager() const { return inputManager; }


private:
	
	float width;
	float height;
	std::string name;
	GLFWwindow* win;
	AppState* appState = nullptr;
	InputManager* inputManager = nullptr;

	// Static callbacks for GLFW — forward to input manager / internal methods
	static void KeyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
	static void MouseCallback(GLFWwindow* win, double xpos, double ypos);
	static void ScrollCallback(GLFWwindow* win, double xoffset, double yoffset);
	static void MouseButtonCallback(GLFWwindow* w, int button, int action, int mods);


	static void FramebufferSizeCallback(GLFWwindow* window, int w, int h);
};