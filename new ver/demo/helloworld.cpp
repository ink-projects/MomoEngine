#include <iostream>
#include <Engine.h>
#include "spdlog/spdlog.h"
#include <GLFW/glfw3.h>

/*int main(int argc, const char* argv[]) {
    std::cout << "Hello, World!\n";
    return 0;
}*/

using namespace momoengine;

static int ticks = 0;

void MyUpdateFunction() {
    if (++ticks % 60 == 0) {
        std::cout << "Ticks so far: " << ticks << "\n";
    }
}

int main() {
    Engine engine;
    engine.Startup();

    engine.RunGameLoop( [&]() {
        auto& input = engine.GetInput();

        if (input.KeyIsPressed(GLFW_KEY_SPACE)) {   //when spacebar is pressed
            spdlog::info("Space is pressed!");
        }

        if (input.KeyIsPressed(GLFW_KEY_ESCAPE)) {  //when escape is pressed
            spdlog::info("Escape pressed -- quitting engine...");
            glfwSetWindowShouldClose(engine.GetGraphics().GetWindow(), true);
        }
    });

    engine.Shutdown();
    return 0;
}