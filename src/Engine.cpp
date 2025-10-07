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
    }

    void Engine::Shutdown() {
        input.Shutdown();
        graphics.Shutdown();
        spdlog::info("Engine shutting down.");
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

            //GraphicsManager::Draw()
            std::vector<Sprite> sprites;

            Sprite s{};
            s.texture_name = "momo";                 //names the loaded texture
            s.position = glm::vec2(100, 100);        //centers the texture
            s.scale = glm::vec2(200, 200);        //scaled normally
            s.z = 0.5f;                             //right in the middle of the layer

            sprites.push_back(s);

            graphics.Draw(sprites);//draw to the screen
            spdlog::info("Draw() called. Sprite count: {}", sprites.size());
        }
    }
}
