#pragma once

#include <cstdint>

#include <game/render/render.h>

#include <spdlog/spdlog.h>

namespace game {
namespace render {
class BspRender : public Render {
private:
    std::shared_ptr<spdlog::logger> _logger;
    void prepare();
public:
    BspRender();

    ~BspRender() = default;
    void render(Viewport &view);

};
}
}
