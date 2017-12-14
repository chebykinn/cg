#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

#include <SDL2/SDL.h>

#include <game/config.h>
#include <game/render/gl_window.h>
#include <game/render/gl_viewport.h>
#include <game/render/bsp_render.h>
#include <game/input.h>
#include <game/sys/glsl.h>
#include <game/entity.h>
#include <game/camera.h>

#include <game/game.h>

using namespace game;
using namespace game::render;

Game::Game() {
    _logger = spdlog::get(Config::logger_name());
    if(_logger == nullptr)
        _logger = spdlog::stdout_color_mt(Config::logger_name());
    auto win  = new GLWindow(Config::window_width(), Config::window_height());
    auto view = new GLViewport({Config::window_width(), Config::window_height()}, *win);
    _window   = std::unique_ptr<Window>(win);
    _view     = std::unique_ptr<Viewport>(view);
}

Game::~Game() {
    SDL_QuitSubSystem(SDL_INIT_EVERYTHING);
    SDL_Quit();
}

int32_t Game::init() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        _logger->critical("SDL_Init failed: {}", SDL_GetError());
        return 2;
    }

    if (!_window->create()) {
        _logger->critical("Failed to create window: {}", _window->get_last_error());
        return 2;
    }

    if (!_view->init()) {
        _logger->critical("Failed to init view: {}", _view->get_last_error());
        return 2;
    }

    Input::init();

    Transform t({0,0,0});
    Entity e;
    e.transform(t);
    e.transform_mut().move({1,0,0});

    return 0;
}


void Game::main_loop() {
    SDL_Event event;
    bool is_running = true;
    size_t tick = 0;

    std::shared_ptr<Camera> camera(new Camera());

    _view->camera(camera);

    _view->camera()->position({230.087,91.6821,-84.9806});

    float speed = 320.0f;
    float jump_accel = 5.0f;
    float mouse_speed = 0.5f;
    // Get mouse position
    float xpos = 0.0f, ypos = 0.0f;
    SDL_SetRelativeMouseMode(SDL_TRUE);

    Transform t({0,0,0});
    Entity e;
    e.transform(t);
    e.renderer(std::unique_ptr<BspRender>(new BspRender(Config::map_name())));


    BspRender *r = dynamic_cast<decltype(r)>(e.renderer().get());
    assert(r != nullptr);
    bool noclip = true;

    glm::vec3 velocity = {0,0,0};


    while(is_running) {
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    is_running = false;
                break;
                case SDL_KEYDOWN: case SDL_KEYUP:
                    Input::handle(event.key.keysym.sym,
                                  event.type == SDL_KEYDOWN);
                break;
                case SDL_MOUSEMOTION:
                    xpos = event.motion.xrel;
                    ypos = event.motion.yrel;
                break;
            }
        }

        float mx = xpos * mouse_speed;
        float my = ypos * mouse_speed;

        auto old_pos = _view->camera()->position();

        if(Action::action("game.quit").is(Action::State::Active)){
            is_running = false;
        }

        if(Action::action("movement.forward").is(Action::State::Active)){
            _view->camera()->move(speed);
        }
        if(Action::action("movement.back").is(Action::State::Active)){
            _view->camera()->move(-speed);
        }
        if(Action::action("movement.right").is(Action::State::Active)){
            _view->camera()->strafe(speed);
        }
        if(Action::action("movement.left").is(Action::State::Active)){
            _view->camera()->strafe(-speed);
        }
        if(Action::action("movement.jump").is(Action::State::Active)){
            if(r->is_on_ground()) {
                velocity.y += jump_accel;
            }
        }
        if(Action::action("debug.noclip").is(Action::State::Activated)){
            noclip = !noclip;
            Action::action("debug.noclip").state(Action::State::None);
        }
        _view->start_render();

        if(!noclip) {

            auto new_pos = _view->camera()->position();
            new_pos.y = old_pos.y;
            _view->camera()->position(new_pos);

            velocity = _view->camera()->velocity(velocity);
            new_pos = _view->camera()->position();

            auto n_pos = r->trace_box(old_pos, new_pos);
            _view->camera()->position(n_pos);
            if(r->is_on_ground()){
                if(velocity.y < 0) {
                    velocity.y = 0;
                }
            }
        }
        _view->camera()->update(mx, my, _view->frame_interval());


        e.renderer()->render(*_view);
        _view->end_render();
        tick++;
        SDL_WarpMouseInWindow(_window->window(), 0, 0);
        xpos = ypos = 0.0f;

    }
}

int32_t Game::start() {
    auto rc = init();
    if(rc != 0) return rc;

    main_loop();
    return 0;
}
