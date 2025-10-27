#pragma once

#include <string>
#include <glm/glm.hpp>

namespace momoengine {
	class Sprite {
	public: 
		Sprite() = default;		//default constructor

		std::string image_name;		//name of the texture
		glm::vec3 position;			//translation in the world space
		glm::vec2 scale;			//scale factor

		float z;	//depth for sorting
		int width;	//pixel width of the image
		int height; //pixel height of the image

		//copy constructor
		Sprite(const std::string& name,
			const glm::vec3& pos = glm::vec3(0.0f),
			const glm::vec2& scl = glm::vec2(1.0f),
			float depth = 0.0f,
			int w = 1,
			int h = 1)
			: image_name(name), position(pos), scale(scl), z(depth), width(w), height(h) {
		}
	};
}