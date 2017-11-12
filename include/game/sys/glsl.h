#pragma once

#include <spdlog/spdlog.h>

#include <string>
#include <memory>

namespace game {
namespace sys {
class GLSL {
private:
	static int32_t _program_id;
	static std::shared_ptr<spdlog::logger> _logger;
	static bool load_file_to_string(const std::string &path, std::string &data);
	static int32_t compile_shader(uint32_t id, const char *const code);
public:
	static uint32_t load_shaders(const std::string &vertex_path,
								 const std::string &fragment_path);

	static const auto &program_id() { return _program_id; }
};

}
}
