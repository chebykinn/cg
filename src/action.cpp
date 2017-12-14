#include <game/action.h>

using namespace game;

std::vector<Action> Action::_actions = {
    {"none",             Action::State::None, 0.0f},
    {"game.quit",        Action::State::None, 0.0f},
    {"movement.forward", Action::State::None, 1.0f},
    {"movement.back",    Action::State::None, 1.0f},
    {"movement.left",    Action::State::None, 1.0f},
    {"movement.right",   Action::State::None, 1.0f},
    {"movement.jump",    Action::State::None, 1.0f},
    {"debug.noclip",     Action::State::None, 1.0f},
};

Action::Action(const std::string &new_name) : _name(new_name) {
    _state = Action::State::None;
    _value = 0.0f;
}

Action::Action(const std::string &new_name, const State new_state, float val) :
    Action(new_name) {
    _state = new_state;
    _value = val;
}

Action &Action::action(const std::string &action_name) {
    for(auto &action : _actions) {
        if(action.name() == action_name) return action;
    }
    return _actions[0];
}

bool Action::add(Action new_action) {
    auto &check = action(new_action.name());
    if(check.name() == new_action.name()) return false;
    _actions.emplace_back(new_action);
    return true;
}
