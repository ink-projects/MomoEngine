#pragma once
#include <string>
#include <unordered_map>
#include <sol/sol.hpp>

class ScriptManager {
public:
	bool Startup();
	void Shutdown();
	bool LoadScript(const std::string& name, const std::string& path);
	void Update();
	
private:
	struct Impl;
	Impl* impl = nullptr;
};