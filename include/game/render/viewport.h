#pragma once

#include <glm/glm.hpp>

#include <game/render/window.h>

namespace game {
namespace render {
class Viewport {
public:
	virtual ~Viewport() = default;
	virtual bool init() = 0;
	virtual const char *get_last_error() = 0;
	virtual const glm::tvec2<size_t> &size() const = 0;
	virtual void size(glm::tvec2<size_t> new_size) = 0;
	virtual void start_render() = 0;
	virtual void end_render() = 0;
};
}
}