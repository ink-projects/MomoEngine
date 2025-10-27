#pragma once

#include <unordered_map>
#include <typeindex>	//for using typeid(T)
#include <functional>	//for ForEach()
#include <vector>
#include "spdlog/spdlog.h"

class EntityManager {
public:
	using Entity = int;		//defines Entity as a type alias for an integer ID

	Entity CreateEntity();
	void DestroyEntity(Entity id);

    //using generics for type safety
	template <typename T>   
	void AddComponent(Entity id, const T& c);

    template <typename T>
    T& GetComponent(Entity id);

    template <typename T>
    void RemoveComponent(Entity id);

    template <typename... Components, typename Func>
    void ForEach(Func func);

private:
    int nextEntityID = 0;

    //type -> (entity -> component pointer)
    std::unordered_map<std::type_index, std::unordered_map<Entity, void*>> components;

    //gets hash identifier for components for lookup
    template <typename T>
    static std::type_index Type() { return std::type_index(typeid(T)); }
};

//gets component for entity
template <typename T>
T& EntityManager::GetComponent(Entity id) {
	auto typeIndex = Type<T>();
	auto componentMapIterator = components.find(typeIndex);		//look for the component with typeIndex

	if (componentMapIterator == components.end()) {
		spdlog::error("Component type not registered in EntityManager");
		static T ret{};	//return value; never meant to be written to
		return ret;		//exits the function
	}

	auto& entityTable = componentMapIterator->second;	//reference the entity which has the components of typeIndex
	auto componentIterator = entityTable.find(id);

	if (componentIterator == entityTable.end()) {
		spdlog::error("Requested component not found for this entity.");
		static T ret{};	//return value; never meant to be written to
		return ret;		//exits the function
	}

	return *static_cast<T*>(componentIterator->second);
}

//stores a copy of component T for the entity
template <typename T>
void EntityManager::AddComponent(Entity id, const T& c) {
	auto& table = components[Type<T>()];
	table[id] = new T(c);
}

//removes component for entity
template <typename T>
void EntityManager::RemoveComponent(Entity id) {
	std::type_index typeIndex = Type<T>();
	auto componentMapIt = components.find(typeIndex);

	if (componentMapIt != components.end()) {
		auto& entityTable = componentMapIt->second;
		auto componentIt = entityTable.find(id);

		if (componentIt != entityTable.end()) {
			delete static_cast<T*>(componentIt->second);
			entityTable.erase(componentIt);
		}
	}
}

//ForEach helper function
template <typename... Components, typename Func>
void EntityManager::ForEach(Func func) {
	auto& firstTable = components[Type<std::tuple_element_t<0, std::tuple<Components...>>>()];	//gets the table for the first component type

	for (auto& [entity, firstComponentPtr] : firstTable) {
		bool hasAll = true;

		((hasAll = hasAll && components[Type<Components>()].count(entity) > 0), ...);	//check if the entity has all requested components

		if (hasAll) {
			func(entity, GetComponent<Components>(entity)...);
		}
	}
}