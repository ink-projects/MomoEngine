#pragma once

#include <string>
#include <glm/glm.hpp>

struct Sprite {
    std::string texture_name;     //name of texture to look up in the GraphicsManager
    glm::vec2 position;           //screen position (top-left)
    glm::vec2 scale = { 1.0f, 1.0f }; //sprite scaling factor
    float z = 0.5f;               //used for draw order (0.0 = back, 1.0 = front)

    Sprite() = default;

    Sprite(const std::string& texture, const glm::vec2& pos,
        const glm::vec2& scl = { 1.0f, 1.0f }, float depth = 0.5f)
        : texture_name(texture), position(pos), scale(scl), z(depth) {
    }
};

