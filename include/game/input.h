#pragma once

#include <vector>
#include <string>
#include <SDL2/SDL.h>

#include <game/action.h>

namespace game {
class Input {
public:
    using KeyCode = SDL_Keycode;
    struct Binding {
        std::string action;
        KeyCode key;
        float value;
    };

    static void init();

    static Binding &get_binding(const std::string &name);
    static Binding &get_binding(const KeyCode &code);
    static bool bind(const std::string &name, const KeyCode &code);

    static void handle(const KeyCode &code, bool is_pressed);
private:
    static std::vector<Binding> _key_states;
};

}
