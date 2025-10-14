#include "InputManager.h"

namespace momoengine {
	void InputManager::Startup(GLFWwindow* w) {
		window = w;
	}

	void InputManager::Shutdown() {
		window = nullptr;
	}

	void InputManager::Update() {
		glfwPollEvents();
	}

	bool InputManager::KeyIsPressed(int key) const {
		if (!window) return false;

		return glfwGetKey(window, key) == GLFW_PRESS;
	}
}