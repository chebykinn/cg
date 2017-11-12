#pragma once

#include <memory>
#include <game/render/render.h>
#include <game/transform.h>

namespace game {
class Entity {
private:
	std::unique_ptr<render::Render> _renderer;
	Transform _transform;

public:
	Entity(){}

	void renderer(std::unique_ptr<render::Render> new_renderer) {
		_renderer = std::move(new_renderer);
	}

	const auto &renderer() const { return _renderer; }

	auto &transform_mut() { return _transform; }
	const auto &transform() const { return _transform; }
	void transform(const Transform &value) { _transform = value; }
};
};
