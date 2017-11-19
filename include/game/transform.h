#pragma once

#include <glm/glm.hpp>

namespace game {
class Transform {
public:
    Transform() = default;
    Transform(const glm::tvec3<float> &new_position) : _position(new_position) {}
    glm::tvec3<float> move(glm::tvec3<float> delta) {
        return position(_position + delta);
    }

    const auto position() const { return _position; }
    glm::tvec3<float> position(const glm::tvec3<float> &value) {
        _position = value;
        return _position;
    }
private:
    glm::tvec3<float> _position = { 0.0f, 0.0f, 0.0f };
};
}
