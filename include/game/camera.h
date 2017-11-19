#pragma once

#include <glm/glm.hpp>

#include <spdlog/spdlog.h>

namespace game {
class Camera {
private:
    std::shared_ptr<spdlog::logger> _logger;
    float _speed = 0.05f;
    float _field_of_view = 45.0f;
    glm::tvec3<float> _position = { 0.0f, 0.0f, 0.0f };

    glm::mat4 _view, _projection;

    glm::tvec3<float> _right = { 0.0f, 0.0f, 0.0f };
    glm::tvec3<float> _direction = { 0.0f, 0.0f, 0.0f };

public:
    Camera();

    void position(const glm::vec3 &new_pos);

    void move(float speed);
    void strafe(float speed);

    void update(float mx, float my);

    const glm::mat4 view() const { return _view; }
    const glm::mat4 projection() const { return _projection; }
    const glm::tvec3<float> position() const { return _position; }

    float field_of_view() const { return _field_of_view; }
    void field_of_view(const float val) { _field_of_view = val; }
};
}
