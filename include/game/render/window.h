#pragma once

#include <SDL2/SDL.h>

namespace game {
namespace render {
class Window {
public:
    virtual ~Window() = default;

    virtual const char *get_last_error() = 0;

    virtual SDL_Window *window() const = 0;

    virtual const size_t &width() const = 0;
    virtual const size_t &height() const = 0;

    virtual const bool &is_fullscreen() const = 0;

    virtual bool create() = 0;
    virtual bool toggle_fullscreen() = 0;
    virtual void resize(const size_t new_width, const size_t new_height) = 0;

};
}
}
