
#include <fstream>
#include <cerrno>
#include <cstring>

#include <spdlog/spdlog.h>

#include <game/config.h>

static std::shared_ptr<spdlog::logger> _logger = spdlog::stdout_color_mt("config");

using namespace game;

std::string Config::_project_name = "bsp-loader";
std::string Config::_logger_name = "game";
std::string Config::_window_title = "game";
std::string Config::_data_path = "data/";
std::string Config::_map_name = "q3dm1";
std::string Config::_shader_path = Config::_data_path + "shaders/";
size_t Config::_window_width = 1280;
size_t Config::_window_height = 720;

bool Config::load(const std::string &filename) {
    std::ifstream f(filename);
    if(!f.is_open()) {
        _logger->error("Failed to open file {}: {}", filename, strerror(errno));
        return false;
    }

}
