#pragma once
#include <functional>
#include <unordered_map>
#include <vector>

class Window;

class InputManager
{
public:
	explicit InputManager(Window* owner);

	// event-based binding: called from HandleKey when event matches (PRESS/RELEASE)
	void BindKeyEvent(int key, int action, std::function<void()> callback);

	// continuous binding: called each Update(dt) while key is pressed
	void BindKeyContinuous(int key, std::function<void(float)> callback);

	// mouse movement: callback gets (xoffset, yoffset)
	void BindMouseMove(std::function<void(double, double)> callback);

	// scroll: callback gets yoffset
	void BindScroll(std::function<void(double)> callback);

	// Called by Window static callbacks
	void HandleKey(int key, int action, int mods);
	void HandleMouseMove(double xpos, double ypos);
	void HandleMouseButton(int button, int action, int mods);
	void HandleScroll(double xoffset, double yoffset);


	void ToggleMouseCapture(bool forceEnable = false);

	// Call each frame to process continuous actions
	void Update(float deltaTime);

private:
	struct KeyEventBind { int action; std::function<void()> func; };
	struct KeyContinuousBind { std::function<void(float)> func; };

	Window* owner;

	// Event bindings: key -> list of (action, callback)
	std::unordered_map<int, std::vector<KeyEventBind>> eventBindings;

	// Continuous bindings: key -> list of callbacks (called with dt while key pressed)
	std::unordered_map<int, std::vector<KeyContinuousBind>> continuousBindings;

	// State of keys (pressed or not)
	std::unordered_map<int, bool> keysPressed;

	// Mouse callbacks (receives offsets)
	std::vector<std::function<void(double, double)>> mouseCallbacks;

	// Scroll callbacks
	std::vector<std::function<void(double)>> scrollCallbacks;
	

	double lastX;
	double lastY;
	bool mouseCaptured;
	bool firstMouse;

};