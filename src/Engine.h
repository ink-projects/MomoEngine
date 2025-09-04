#pragma once
#include "GraphicsManager.h"

namespace momoengine {

    class Engine {
    public:
        void Startup();     //initializes the engine
        void Shutdown();    //shuts down the engine
        void RunGameLoop(void (*UpdateCallback)());     //runs the main game loop
    
    private:
        GraphicsManager graphics;   //adds GraphicsManager window
    };
}
