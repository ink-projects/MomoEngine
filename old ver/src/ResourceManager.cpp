#include "ResourceManager.h"
#include <filesystem>

namespace momoengine {
	ResourceManager::ResourceManager () {
		rootPath = "assets";	//sets default path to "assets" folder
	}

	void ResourceManager::setRootPath (const std::filesystem::path& newRoot) {
		rootPath = newRoot;
	}

	//called to look up files and appends the path of a file to the designated rootPath
	std::filesystem::path ResourceManager::ResolvePath(const std::filesystem::path& relativePath) const {
		return rootPath / relativePath;
	}
}