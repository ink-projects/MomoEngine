#pragma once

#include <string>
#include <filesystem>

namespace momoengine {
	class ResourceManager {
	public:
		ResourceManager();

		void setRootPath(const std::filesystem::path& newRoot);		//allows users to change the root directory (command line argument)
		std::filesystem::path ResolvePath(const std::filesystem::path& relativePath) const;		//resolves a relative file path against the root (textures/image.png)

	private:
		std::filesystem::path rootPath;
	};
}