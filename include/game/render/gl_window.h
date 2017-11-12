#pragma once

#include <spdlog/spdlog.h>

#include <SDL2/SDL.h>
#include <string>
#include <memory>

#include <game/render/window.h>

namespace game {
namespace render {
class GLWindow : public Window {
private:
	std::shared_ptr<spdlog::logger> _logger;
	SDL_Window *_win = nullptr;
	size_t _width = 0, _height = 0;
	bool _fullscreen = false;
public:
	GLWindow(size_t new_width, size_t new_height);
	GLWindow() = default;
	~GLWindow();

	const char *get_last_error() { return SDL_GetError(); }

	SDL_Window *window() const { return _win; }

	const size_t &width() const { return _width; }
	const size_t &height() const { return _height; }

	const bool &is_fullscreen() const { return _fullscreen; }

	bool create();
	bool toggle_fullscreen();
	void resize(const size_t new_width, const size_t new_height);

};
}
}
