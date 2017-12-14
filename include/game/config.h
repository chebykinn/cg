#pragma once

#include <string>

namespace game {
class Config {
private:
    static std::string _project_name;
    static std::string _logger_name;
    static std::string _window_title;
    static std::string _data_path;
    static std::string _shader_path;
    static std::string _map_name;
    static size_t _window_width;
    static size_t _window_height;
public:
    static bool load(const std::string &filename);
    static const auto &data_path() { return _data_path; };
    static void data_path(const std::string &val) { _data_path = val; };

    static const auto &shader_path() { return _shader_path; };
    static void shader_path(const std::string &val) { _shader_path = val; };

    static const auto &project_name() { return _project_name; };
    static void project_name(const std::string &val) { _project_name = val; };

    static const auto &logger_name() { return _logger_name; };
    static void logger_name(const std::string &val) { _logger_name = val; };

    static const auto &window_title() { return _window_title; };
    static void window_title(const std::string &val) { _window_title = val; };

    static const auto &window_width() { return _window_width; };
    static void window_width(const size_t val) { _window_width = val; };

    static const auto &window_height() { return _window_height; };
    static void window_height(const size_t val) { _window_height = val; };

    static const auto &map_name() { return _map_name; };
    static void map_name(const std::string &val) { _map_name = val; };
};
}
