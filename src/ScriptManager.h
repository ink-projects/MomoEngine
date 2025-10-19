#pragma once
#include <string>
#include <unordered_map>
#include <sol/sol.hpp>

namespace momoengine {
	//to avoid circular dependencies
	class Engine;
	class InputManager;
	class GraphicsManager;

	class ScriptManager {
	public:
		bool Startup(Engine* eng, InputManager* inputMgr, GraphicsManager* gMgr);	//takes a pointer to InputManager
		void Shutdown();
		bool LoadScript(const std::string& name, const std::string& path);
		bool RunScript(const std::string& name);

		sol::state& GetLua() { return lua; }

	private:
		sol::state lua;
		std::unordered_map<std::string, sol::protected_function> scripts;
		InputManager* input = nullptr;	//to store InputManager pointer for Startup()
		Engine* engine = nullptr;	//so that scripts can shut down the game when necessary
		GraphicsManager* graphics = nullptr;
	};
}
