#pragma once

#include <string>
#include <vector>

namespace game {
class Action {
public:
    enum class State {
        None,
        Activated,
        Active,
    };
    Action(const std::string &new_name);
    Action(const std::string &new_name, const State new_state, float val);

    static Action &action(const std::string &action_name);

    static bool add(Action new_action);

    const auto &name() const { return _name; }
    const auto &state() const { return _state; }
    const auto &value() const { return _value; }

    void state(State new_state) { _state = new_state; }
    void value(float val) { _value = val; }

    bool is(State check_state) { return _state == check_state; }

    bool empty(){ return _name.empty() || _name == "none"; }
private:
    static std::vector<Action> _actions;
    const std::string _name;
    State _state;
    float _value;

};
}
