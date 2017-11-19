#include <game/config.h>
#include <game/render/gl_window.h>

using namespace game;
using namespace game::render;

GLWindow::GLWindow(size_t new_width, size_t new_height) {
    _width = new_width;
    _height = new_height;
    _logger = spdlog::get(Config::logger_name());
    if(_logger == nullptr)
        _logger = spdlog::stdout_color_mt(Config::logger_name());
}

GLWindow::~GLWindow() {
    SDL_DestroyWindow(_win);
}

bool GLWindow::create() {
    _logger->info("Creating window: title: {}, size: {}x{}",
                  Config::window_title(), _width, _height);
    _win = SDL_CreateWindow(Config::window_title().c_str(),
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            _width, _height,
                            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    return _win != nullptr;
}

bool GLWindow::toggle_fullscreen() {
    _logger->info("Toggle fullscreen");
    _fullscreen = !_fullscreen;
}

void GLWindow::resize(const size_t new_width, const size_t new_height) {
    _logger->info("Resizing window to {}x{}", new_width, new_height);
    _width = new_width;
    _height = new_height;
}
