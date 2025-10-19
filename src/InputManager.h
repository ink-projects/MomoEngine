#pragma once
#include <GLFW/glfw3.h>

namespace momoengine {

    class InputManager {
    public:
        void Startup(GLFWwindow* w);  //stores a pointer to GLFWwindow
        void Shutdown();
        void Update();                     //calls glfwPollEvents()
        bool KeyIsPressed(int key) const;  //to use glfwGetKey()

    private:
        GLFWwindow* window = nullptr;      //gets input state
    };

}
