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

    ScriptManager scripts;
    scripts.Startup(&engine, &engine.GetInput(), &engine.GetGraphics());

    //run and load test script
    scripts.LoadScript("test", "assets/test.lua");
    auto& lua = scripts.GetLua();

    //load a texture at startup
    engine.GetGraphics().LoadTexture("momo", "assets/momo.png");

    //create the sprite using the texture
    std::vector<Sprite> sprites;
    sprites.emplace_back("momo", glm::vec3(0.0f, 0.0f, 0.0f));

    engine.RunGameLoop( [&]() {
        sol::function updateFunc = lua["Update"];
        if (updateFunc.valid()) { 
            sol::protected_function_result result = updateFunc(); 

            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("Lua Update() error: {}", err.what());
            }
        }

        //auto& input = engine.GetInput();

        //if (input.KeyIsPressed(GLFW_KEY_SPACE)) {   //when spacebar is pressed
            //spdlog::info("Space is pressed!");
        //}

        //if (input.KeyIsPressed(GLFW_KEY_ESCAPE)) {  //when escape is pressed
            //spdlog::info("Escape pressed -- quitting engine...");
            //glfwSetWindowShouldClose(engine.GetGraphics().GetWindow(), true);
        //}

        //draw sprites every frame
        engine.GetGraphics().Draw(sprites);
    });

    scripts.Shutdown();
    engine.Shutdown();
    return 0;
}