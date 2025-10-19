#include "Engine.h"
#include "spdlog/spdlog.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace momoengine {

    void Engine::Startup() {
        spdlog::info("Engine started up.");

        bool success = graphics.Startup(800, 600, "Momo Engine", false);
        if (!success) {
            spdlog::error("Graphics startup failed");
        }

        input.Startup(graphics.GetWindow());    //gives input manager access to a window

        scripts.Startup(this, &input, &graphics);
    }

    void Engine::Shutdown() {
        scripts.Shutdown();
        input.Shutdown();
        graphics.Shutdown();
        spdlog::info("Engine shutting down.");
    }

    void Engine::Quit() {
        auto* window = graphics.GetWindow();
        if (window) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            spdlog::info("Quit requested by script.");
        }
    }

    void Engine::RunGameLoop(const UpdateCallback& callback) {
        auto* window = graphics.GetWindow();    //creates the window
        if (!window) {
            spdlog::error("RunGameLoop called with no valid window; Startup() may have failed.");
            return;
        }

        double previousTime = glfwGetTime();    //initial time in seconds
        double accumulatedTime = 0.0;   //stores amount of time between updates
        const double tickRate = 1.0 / 60.0;     //60 updates per second

        while (!glfwWindowShouldClose(window)) {    //while the window is open
            input.Update();     //uses glfwPollEvents(), which polls input events

            double currentTime = glfwGetTime();     //current time in seconds
            double elapsedTime = currentTime - previousTime;    //calculates time passed between loops
            previousTime = currentTime;     //updates the amount of time spent

            accumulatedTime += elapsedTime;     //adds time spent

            //if the elapsed time has somehow surpassed the tick rate
            while (accumulatedTime >= tickRate) {
                // InputManager::Update()
                callback();   //calls update function
                accumulatedTime -= tickRate;
            }
        }
    }


}
