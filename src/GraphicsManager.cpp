#define GLFW_INCLUDE_NONE  //doesn't include OpenGL
#include "GLFW/glfw3.h"

#include "GraphicsManager.h"
#include "spdlog/spdlog.h"

namespace momoengine {

    bool GraphicsManager::Startup(int window_width, int window_height, const char* window_name, bool fullscreen) {
        if (!glfwInit()) {
            spdlog::error("Failed to initialize GLFW.");
            return false;
        }

        // We don't want GLFW to set up a graphics API.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Create the window.
        window = glfwCreateWindow(window_width, window_height, window_name, fullscreen ? glfwGetPrimaryMonitor() : 0, 0);
        if (!window) {  //error catching for if something happened with creating the window
            spdlog::error("Failed to create a window.");
            glfwTerminate();
            return false;
        }

        glfwSetWindowAspectRatio(window, window_width, window_height);
        spdlog::info("Window created: {}x{}", window_width, window_height);
        return true;
    }

    void GraphicsManager::Shutdown() {
        if (window) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
        spdlog::info("GLFW shutdown complete");
    }

}
