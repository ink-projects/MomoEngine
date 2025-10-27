#include <iostream>
#include "spdlog/spdlog.h"
#include <GLFW/glfw3.h>

#include "Engine.h"
#include "EntityManager.h"
#include "Types.h"

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
    //auto& lua = scripts.GetLua();

    //load a texture at startup
    //engine.GetGraphics().LoadTexture("momo", "assets/momo.png");

    //create the sprite using the texture
    //std::vector<Sprite> sprites;
    //sprites.emplace_back("momo", glm::vec3(0.0f, 0.0f, 0.0f));

    //create entity manager
    EntityManager& em = engine.GetEntityManager();

    //create Momo's entity
    EntityManager::Entity momoEntity = em.CreateEntity();

    //attach components
    Sprite momoSprite { "momo", glm::vec3(0.0f, 0.0f, 0.0f) };
    Position momoPos { 0.0f, 0.0f };
    Script momoScript{ "test" };

    em.AddComponent(momoEntity, momoSprite);
    em.AddComponent(momoEntity, momoPos);
    em.AddComponent(momoEntity, momoScript);

    engine.RunGameLoop( [&]() {
        /*sol::function updateFunc = lua["Update"];
        if (updateFunc.valid()) { 
            sol::protected_function_result result = updateFunc(); 

            if (!result.valid()) {
                sol::error err = result;
                spdlog::error("Lua Update() error: {}", err.what());
            }
        }*/

        //auto& input = engine.GetInput();

        //if (input.KeyIsPressed(GLFW_KEY_SPACE)) {   //when spacebar is pressed
            //spdlog::info("Space is pressed!");
        //}

        //if (input.KeyIsPressed(GLFW_KEY_ESCAPE)) {  //when escape is pressed
            //spdlog::info("Escape pressed -- quitting engine...");
            //glfwSetWindowShouldClose(engine.GetGraphics().GetWindow(), true);
        //}
        
        //read the sprites table from Lua
        /*sol::table luaSprites = lua["sprites"];
        std::vector<Sprite> spritesToDraw;
        if (luaSprites.valid()) {
            for (auto& kv : luaSprites) {
                sol::object value = kv.second;
                if (value.is<Sprite>()) {
                    Sprite s = value.as<Sprite>();
                    spritesToDraw.push_back(s);
                }
            }
        }*/
        //run Lua scripts attached to entities
        scripts.Update(em);

        //draw sprites every frame
        engine.GetGraphics().Draw(em);
    });

    //testing RemoveComponent()
    spdlog::info("Testing RemoveComponent for Sprite...");
    em.RemoveComponent<Sprite>(momoEntity);

    Sprite& removedSprite = em.GetComponent<Sprite>(momoEntity);
    if (removedSprite.image_name.empty()) {
        spdlog::info("Sprite successfully removed (image_name is empty).");
    }
    else {
        spdlog::warn("Sprite still exists: {}", removedSprite.image_name);
    }

    //testing DestroyEntity()
    spdlog::info("Testing DestroyEntity on momoEntity...");
    em.DestroyEntity(momoEntity);

    Position& posCheck = em.GetComponent<Position>(momoEntity);
    if (posCheck.x == 0.0f && posCheck.y == 0.0f) {
        spdlog::info("Entity destroyed successfully and component not found.");
    }
    else {
        spdlog::warn("Entity still exists? Position returned something.");
    }

    scripts.Shutdown();
    engine.Shutdown();
    return 0;
}