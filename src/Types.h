#pragma once

#include <string>


//types of components to be used by Lua; the constructors make it easier to create these objects
struct Position { 
	float x, y;
	Position(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}	//constructor
};

struct Velocity {
	float x, y;
	Velocity(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}	//constructor
};

struct Gravity {
	float meters_per_second;
	Gravity(float mps = 9.8f) : meters_per_second(mps) {}	//constructor
};

struct SpriteComponent {
	std::string image;
	float size;
	SpriteComponent(const std::string& img = "", float s = 1.0f) : image(img), size(s) {}	//constructor
};

struct Health {
	float percent;
	Health(float p = 100.0f) : percent(p) {}	//constructor
};

struct Script {
	std::string name;
	Script(const std::string& n = "") : name(n) {}		//constructor
};