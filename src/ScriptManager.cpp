#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "ScriptManager.h"
#include "Sprite.h"
#include "Engine.h"
#include "InputManager.h"
#include "GraphicsManager.h"
#include "EntityManager.h"
#include "Types.h"

#include "spdlog/spdlog.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using namespace momoengine;

bool ScriptManager::Startup(Engine* eng, InputManager* inputMgr, GraphicsManager* gMgr) {
	//store input
	input = inputMgr;

	//store engine pointer
	engine = eng;

	//store graphics pointer
	graphics = gMgr;
	
	//opens standard and debugging Lua libraries
	lua.open_libraries(sol::lib::os, sol::lib::string, sol::lib::io, sol::lib::debug);
	spdlog::info("Lua scripting initialized...");

	//override print() from Lua to spdlog
	lua.set_function("print", [](sol::variadic_args va) {
		std::string out;
		for (auto v : va) {
			out += v.get<std::string>() + " ";
		}
		spdlog::info("[Lua] {}", out);
		});

	//bind input manager functionality
	lua.set_function("KeyIsDown", [&](const int keycode) { return input->KeyIsPressed(keycode); });

	//quit functionality
	lua.set_function("Quit", [&]() { if (engine) engine->Quit(); });

	//LoadTexture() functionality
	lua.set_function("LoadTexture", [&](const std::string& name, const std::string& path) {
		if (graphics) return graphics->LoadTexture(name, path);
		return false;
		});

	//LoadScript() functionality
	lua.set_function("LoadScript", [&](const std::string& name, const std::string& path) {
		return this->LoadScript(name, path);
		});

	//key codes with Lua enum
	lua.new_enum<int>("KEYBOARD", {
		{ "SPACE", GLFW_KEY_SPACE },
		{ "ESCAPE", GLFW_KEY_ESCAPE },
		{ "W", GLFW_KEY_W },
		{ "A", GLFW_KEY_A },
		{ "S", GLFW_KEY_S },
		{ "D", GLFW_KEY_D }
	});
	spdlog::info("InputManager bound to Lua.");

	//vec3
	lua.new_usertype<glm::vec3>("vec3",
		sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z,
		// optional and fancy: operator overloading. see: https://github.com/ThePhD/sol2/issues/547
		sol::meta_function::addition, sol::overload([](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 + v2; }),
		sol::meta_function::subtraction, sol::overload([](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 - v2; }),
		sol::meta_function::multiplication, sol::overload(
			[](const glm::vec3& v1, const glm::vec3& v2) -> glm::vec3 { return v1 * v2; },
			[](const glm::vec3& v1, float f) -> glm::vec3 { return v1 * f; },
			[](float f, const glm::vec3& v1) -> glm::vec3 { return f * v1; }
		)
	);

	//vec2
	lua.new_usertype<glm::vec2>("vec2",
		sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
		"x", &glm::vec2::x,
		"y", &glm::vec2::y,

		sol::meta_function::addition, sol::overload([](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 + v2; }),
		sol::meta_function::subtraction, sol::overload([](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 - v2; }),
		sol::meta_function::multiplication, sol::overload(
			[](const glm::vec2& v1, const glm::vec2& v2) -> glm::vec2 { return v1 * v2; },
			[](const glm::vec2& v1, float f) -> glm::vec2 { return v1 * f; },
			[](float f, const glm::vec2& v1) -> glm::vec2 { return f * v1; }
		)
	);

	lua.new_usertype<momoengine::Sprite>("Sprite",
		sol::constructors<
		momoengine::Sprite(const std::string&,
			const glm::vec3&,
			const glm::vec2&,
			float,
			int,
			int),
		momoengine::Sprite(const std::string&) //allows just name
		>(),
		"image_name", &momoengine::Sprite::image_name,
		"position", &momoengine::Sprite::position,
		"scale", &momoengine::Sprite::scale,
		"z", &momoengine::Sprite::z,
		"width", &momoengine::Sprite::width,
		"height", &momoengine::Sprite::height
	);

	spdlog::info("Sprite exposed to Lua.");
	return true;
}

bool ScriptManager::LoadScript(const std::string& name, const std::string& path) {
	//load the script from the path
	sol::load_result loaded = lua.load_file(path);
	if (!loaded.valid()) {
		sol::error err = loaded;
		spdlog::error("Failed to load script {} from {}: {}", name, path, err.what());
		return false;
	}

	//create protected function
	sol::protected_function func = loaded;
	scripts[name] = func;

	sol::protected_function_result result = func();
	if (!result.valid()) {
		sol::error err = result;
		spdlog::error("Error running script '{}' top-level: {}", name, err.what());
	}
	spdlog::info("Loaded script '{}' -> {}", name, path);
	return true;
}

bool ScriptManager::RunScript(const std::string& name) {
	auto rs = scripts.find(name);
	if (rs == scripts.end()) {
		spdlog::error("RunScript failed: script '{}' not found.", name);
		return false;
	}

	sol::protected_function& func = rs->second;
	sol::protected_function_result result = func();
	if (!result.valid()) {
		sol::error err = result;
		spdlog::error("Error running script '{}': {}", name, err.what());
		return false;
	}
	return true;
}

void ScriptManager::Update(EntityManager& entities) {
	//iterate over all entities using Script component
	entities.ForEach<Script>([&](EntityManager::Entity id, Script& script) {
		auto it = scripts.find(script.name);
		if (it == scripts.end()) {
			spdlog::error("Entity {} has script '{}' that is not loaded.", id, script.name);
			return;
		}

		//set a global entity variable for current entity
		lua["entity"] = id;

		sol::function updateFunc = lua["Update"];
		if (updateFunc.valid()) {
			sol::protected_function_result result = updateFunc();

			if (!result.valid()) {
				sol::error err = result;
				spdlog::error("Lua error in Update() for entity {}: {}", id, err.what());
			}
		} else {
			spdlog::error("Script '{}' has no Update() function", script.name);
		}
	});
}

void ScriptManager::Shutdown() {
	scripts.clear();
	spdlog::info("Lua scripting shutting down...");
}