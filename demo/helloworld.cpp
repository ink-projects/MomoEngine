#include <iostream>
#include <Engine.h>

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
    engine.RunGameLoop(MyUpdateFunction);
    engine.Shutdown();
    return 0;
}