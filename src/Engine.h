#pragma once
#include "GraphicsManager.h"
#include "InputManager.h"
#include <functional>

namespace momoengine {

    class Engine {
    public:
        void Startup();     //initializes the engine
        void Shutdown();    //shuts down the engine

        using UpdateCallback = std::function<void()>;
        void RunGameLoop(const UpdateCallback& callback);     //runs the main game loop

        InputManager& GetInput() { return input; }
        GraphicsManager& GetGraphics() { return graphics; }
    
    private:
        GraphicsManager graphics;   //adds GraphicsManager window
        InputManager input;          //grabs keyboard/mouse input
    };
}
