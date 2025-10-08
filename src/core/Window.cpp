#include "../includes/core/Window.h"
#include "../includes/core/InputManager.h"
#include <iostream>

Window::Window(float width, float height, const std::string& name)
	: width(width), height(height), name(name), inputManager(nullptr)
{ 
	if (!glfwInit()) {
		std::cerr << "Failed to init GLFW\n";
		return;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	win = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);



	if (!win) {
		glfwTerminate();
		throw std::runtime_error("Failed to create window");
	}

	glfwMakeContextCurrent(win);

	//	Initialize GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD\n";
		return;
	}

	inputManager = new InputManager(this);

	glfwSetWindowUserPointer(win, this);
	// register callbacks
	glfwSetKeyCallback(win, KeyCallback);
	glfwSetCursorPosCallback(win, MouseCallback);
	glfwSetScrollCallback(win, ScrollCallback);
	glfwSetMouseButtonCallback(win, MouseButtonCallback);
	glfwSetFramebufferSizeCallback(win, FramebufferSizeCallback);
	glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

	glEnable(GL_DEPTH_TEST);
}

Window::~Window()
{
	if (inputManager) { delete inputManager; inputManager = nullptr; }
	if (win) { glfwDestroyWindow(win); win = nullptr; }
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(win);
}

void Window::PollEvents() const
{
	return glfwPollEvents();
}

void Window::SwapBuffers() const
{
	return glfwSwapBuffers(win);
}

GLFWwindow* Window::GetNative() const { return win; }
int Window::Width() const { return width; }
int Window::Height() const { return height; }

void Window::FramebufferSizeCallback(GLFWwindow* win, int w, int h)
{
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
	if (self) 
	{
		self->width = static_cast<float>(w);
		self->height = static_cast<float>(h);
		glViewport(0, 0, w, h);
	};
}

void Window::KeyCallback(GLFWwindow* w, int key, int scancode, int action, int mods) {
	if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w))) {
		if (self->inputManager) self->inputManager->HandleKey(key, action, mods);
	}
}

void Window::MouseCallback(GLFWwindow* w, double xpos, double ypos) {
	if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w))) {
		if (self->inputManager) self->inputManager->HandleMouseMove(xpos, ypos);
	}
}

void Window::MouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {
	if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w))) {
		if (self->inputManager) self->inputManager->HandleMouseButton(button, action, mods);
	}
}

void Window::ScrollCallback(GLFWwindow* w, double xoffset, double yoffset) {
	if (auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w))) {
		if (self->inputManager) self->inputManager->HandleScroll(xoffset, yoffset);
	}
}