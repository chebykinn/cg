#pragma once

#include <spdlog/spdlog.h>

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include <game/render/viewport.h>
#include <game/render/window.h>

namespace game {
namespace render {
class GLViewport : public Viewport {
private:
    std::shared_ptr<spdlog::logger> _logger;
    glm::tvec2<size_t> _size;
    SDL_GLContext _context;
    Window &_window;
    const char *_last_error = nullptr;

    std::shared_ptr<Camera> _camera;
public:
    GLViewport(glm::tvec2<size_t> new_size, Window &window);
    ~GLViewport();
    bool init();
    void start_render();
    void end_render();

    virtual const char *get_last_error() { return _last_error; }
    const glm::tvec2<size_t> &size() const { return _size; }
    void size(glm::tvec2<size_t> new_size) { _size = new_size; }

    std::shared_ptr<Camera> camera() const { return _camera; }
    void camera(const std::shared_ptr<Camera> &new_camera) {
        _camera = new_camera;
    }
};
}
}
