#pragma once

#include <cstdint>

#include <spdlog/spdlog.h>

#include <game/render/window.h>
#include <game/render/viewport.h>

namespace game {
class Game {
private:
    std::shared_ptr<spdlog::logger> _logger;
    std::unique_ptr<render::Window> _window;
    std::unique_ptr<render::Viewport> _view;
    int32_t init();
    void main_loop();
public:
    Game();
    ~Game();
    int32_t start();
};
}
