#pragma once

#include <game/render/viewport.h>

namespace game {
namespace render {
class Render {
public:
    virtual ~Render() = default;
    virtual void render(Viewport &view) = 0;
};
}
}
