#pragma once

#include <cstdint>
#include <string>

#include <game/render/render.h>

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>

namespace game {
namespace render {
class BspRender : public Render {
private:
    std::shared_ptr<spdlog::logger> _logger;
    void prepare();
    std::string _map_name;
public:
    BspRender(std::string map_name);

    bool is_collided();
    glm::vec3 trace_box(glm::vec3 start, glm::vec3 end);
    bool is_on_ground();

    ~BspRender() = default;
    void render(Viewport &view);

};
}
}
