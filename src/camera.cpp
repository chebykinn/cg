#include <cassert>

#include <SDL2/SDL.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <game/camera.h>
#include <game/config.h>

static double current_time = 0.0f;
static double last_time    = SDL_GetTicks();
static float delta_time   = 0.0f;
static float horizontal_angle = 3.14f;
static float vertical_angle = 0.0f;

using namespace game;

Camera::Camera() {
    _logger = spdlog::get(Config::logger_name());
    if(_logger == nullptr)
        _logger = spdlog::stdout_color_mt(Config::logger_name());
}

void Camera::position(const glm::vec3 &new_pos) {
    _position = new_pos;
}

void Camera::move(float speed) {
    assert(delta_time > 0 && "Camera is not updating");
    _position += _direction * delta_time * speed;
}

void Camera::strafe(float speed) {
    _position += _right * delta_time * speed;
}

void Camera::update(float mx, float my) {
    current_time = SDL_GetTicks();
    delta_time = current_time - last_time;
    last_time = current_time;

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
    _projection = glm::perspective(glm::radians(_field_of_view), width / height, 0.1f, 100.0f);

    // Camera matrix
    _view = glm::lookAt(
        _position, // Camera is at (4,3,3), in World Space
        _position + _direction, // and looks at the origin
        up  // Head is up (set to 0,-1,0 to look upside-down)
        );
}
