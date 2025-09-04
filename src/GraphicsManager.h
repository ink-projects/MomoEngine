#pragma once

struct GLFWwindow;

namespace momoengine {

    class GraphicsManager {
    public:
        bool Startup(int window_width, int window_height, const char* window_name, bool fullscreen);
        void Shutdown();

        GLFWwindow* GetWindow() const { return window; }

    private:
        GLFWwindow* window = nullptr;
    };

}
