#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include "ScriptManager.h"
#include "spdlog/spdlog.h"

struct ScriptManager::Impl {
	sol::state lua;
	sol::protected_function update_fn;
};

bool ScriptManager::Startup() {
	impl = new Impl();

	//opens standard Lua libraries
	impl->lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table);


}