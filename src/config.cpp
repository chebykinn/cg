#include <game/config.h>

using namespace game;

std::string Config::_project_name = "bsp-loader";
std::string Config::_logger_name = "game";
std::string Config::_window_title = "game";
std::string Config::_data_path = "data/";
std::string Config::_shader_path = Config::_data_path + "shaders/";
size_t Config::_window_width = 1280;
size_t Config::_window_height = 720;
