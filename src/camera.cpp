#include <cassert>

#include <SDL.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <game/camera.h>
#include <game/config.h>

static float delta_time   = 0.0f;
static float horizontal_angle = 3.14f;
static float vertical_angle = 0.0f;

static float g = 9.8f;

using namespace game;

Camera::Camera() {
    _logger = spdlog::get(Config::logger_name());
    if(_logger == nullptr)
        _logger = spdlog::stdout_color_mt(Config::logger_name());
}

glm::vec3 Camera::velocity(const glm::vec3 &velocity) {
    auto n_v = velocity;
    n_v.y -= g * delta_time;
    _position += n_v;
    return n_v;
}

void Camera::position(const glm::vec3 &new_pos) {
    _position = new_pos;
}

void Camera::move(float speed) {
    _position += _direction * delta_time * speed;
}

void Camera::strafe(float speed) {
    _position += _right * delta_time * speed;
}

void Camera::update(float mx, float my, float frame_interval) {
    delta_time = frame_interval;

    // Compute new orientation
    horizontal_angle -= delta_time * mx;
    vertical_angle   -= delta_time * my;

    float width = static_cast<float>(Config::window_width());
    float height = static_cast<float>(Config::window_height());

    // Direction : Spherical coordinates to Cartesian coordinates conversion
    _direction = {
        cos(vertical_angle) * sin(horizontal_angle),
        sin(vertical_angle),
        cos(vertical_angle) * cos(horizontal_angle)
    };

    // Right vector
    _right = {
        sin(horizontal_angle - M_PI/2.0f),
        0,
        cos(horizontal_angle - M_PI/2.0f)
    };

    // Up vector : perpendicular to both direction and right
    glm::vec3 up = glm::cross( _right, _direction );

    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    gluPerspective(90.0f, width / height, 0.1f, 10000.0f);

    gluLookAt(_position.x, _position.y, _position.z,
    _position.x + _direction.x, _position.y + _direction.y, _position.z + _direction.z,
    up.x, up.y, up.z);


}
