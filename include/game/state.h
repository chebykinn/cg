#pragma once

#include <string>
#include <memory>
#include <vector>

#include <game/window/window.h>
#include <game/common.h>

namespace game {
class State {
private:
    static std::shared_ptr<Window> _main_window;
public:
    static const auto &main_window() { return _main_window; }
    static void main_window(const std::shared_ptr<Window> &window) {
        _main_window = window;
    }
};
}
