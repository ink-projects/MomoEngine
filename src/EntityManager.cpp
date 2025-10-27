#include "EntityManager.h"
#include "Types.h"
#include "Sprite.h"
#include "spdlog/spdlog.h"

//creates a new entity and its ID
EntityManager::Entity EntityManager::CreateEntity() {
	Entity id = nextEntityID++;
	spdlog::info("Created entity {}", id);
	return id;
}

//removes entity from every component table
void EntityManager::DestroyEntity(Entity id) {
	for (auto& [type, table] : components) {
		auto it = table.find(id);
		if (it != table.end()) {
			delete static_cast<char*>(it->second);	//generic delete
			table.erase(it);
		}
	}
	spdlog::info("Destroyed entity {}", id);
}

//explicit template instantiations for all component types
//template void EntityManager::AddComponent<struct Position>(Entity, const Position&);
//template void EntityManager::AddComponent<struct Velocity>(Entity, const Velocity&);
//template void EntityManager::AddComponent<struct Gravity>(Entity, const Gravity&);
//template void EntityManager::AddComponent<momoengine::Sprite>(Entity, const momoengine::Sprite&);
//template void EntityManager::AddComponent<struct Health>(Entity, const Health&);
//template void EntityManager::AddComponent<Script>(Entity, const Script&);
//
//template Position& EntityManager::GetComponent<Position>(Entity);
//template Velocity& EntityManager::GetComponent<Velocity>(Entity);
//template Gravity& EntityManager::GetComponent<Gravity>(Entity);
//template momoengine::Sprite& EntityManager::GetComponent<momoengine::Sprite>(Entity);
//template Health& EntityManager::GetComponent<Health>(Entity);
//template Script& EntityManager::GetComponent<Script>(Entity);
//
//template void EntityManager::RemoveComponent<Position>(Entity);
//template void EntityManager::RemoveComponent<Velocity>(Entity);
//template void EntityManager::RemoveComponent<Gravity>(Entity);
//template void EntityManager::RemoveComponent<momoengine::Sprite>(Entity);
//template void EntityManager::RemoveComponent<Health>(Entity);
//template void EntityManager::RemoveComponent<Script>(Entity);