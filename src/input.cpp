#include <cassert>
#include <game/input.h>

#define CHECK_INIT()                                                           \
    assert(_key_states.size() > 0 && "error: called before Input::init()");

using namespace game;

std::vector<Input::Binding> Input::_key_states;

void Input::init() {
    _key_states = std::vector<Binding>{
        {"",                 0, 0.0f},
        {"game.quit",        SDLK_ESCAPE, 0.0f},
        {"movement.forward", SDLK_w, 1.0f},
        {"movement.back",    SDLK_s, 1.0f},
        {"movement.left",    SDLK_a, 1.0f},
        {"movement.right",   SDLK_d, 1.0f},
        {"movement.jump",    SDLK_SPACE, 1.0f},
    };
}

Input::Binding &Input::get_binding(const std::string &name) {
    CHECK_INIT();
    if(name.empty()) return _key_states[0];
    for(auto &binding : _key_states) if(binding.action == name) return binding;
    return _key_states[0];
}

Input::Binding &Input::get_binding(const KeyCode &code) {
    CHECK_INIT();
    for(auto &binding : _key_states) if(binding.key == code) return binding;
    return _key_states[0];
}

bool Input::bind(const std::string &name, const KeyCode &code) {
    auto &action = Action::action(name);
    if(action.empty()) return false;

    auto &binding = get_binding(name);
    if(binding.action.empty()){
        Binding new_bind = {name, code, 0.0f};
        _key_states.emplace_back(new_bind);
        return true;
    }
    action.state(Action::State::None);
    binding.key = code;
    return true;
}

void Input::handle(const Input::KeyCode &code, bool is_pressed) {
    auto &binding = get_binding(code);
    if(binding.action.empty()) return;

    auto &action = Action::action(binding.action);
    assert(!action.empty() && "BUG: got binding to empty action");

    if(is_pressed) action.state(Action::State::Active);
    else action.state(Action::State::Activated);
}
