#pragma once
#include "GraphicsManager.h"
#include "InputManager.h"
#include "ScriptManager.h"
#include "EntityManager.h"
#include <functional>

namespace momoengine {

    class Engine {
    public:
        void Startup();     //initializes the engine
        void Shutdown();    //shuts down the engine
        void Quit();    //allows other windows to quit

        using UpdateCallback = std::function<void()>;
        void RunGameLoop(const UpdateCallback& callback);     //runs the main game loop

        InputManager& GetInput() { return input; }
        GraphicsManager& GetGraphics() { return graphics; }
        ScriptManager& GetScripts() { return scripts;  }
        EntityManager& GetEntityManager() { return entities; }
    
    private:
        GraphicsManager graphics;   //adds GraphicsManager window
        InputManager input;          //grabs keyboard/mouse input
        ScriptManager scripts;
        EntityManager entities;
    };
}
