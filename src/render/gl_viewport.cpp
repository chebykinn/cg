#include <GL/glew.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <game/render/gl_viewport.h>
#include <game/sys/glsl.h>
#include <game/config.h>

using namespace game;
using namespace game::render;

GLViewport::GLViewport(glm::tvec2<size_t> new_size, Window &window) :
    _window(window) {
    size(new_size);
    _logger = spdlog::get(Config::logger_name());
    if(_logger == nullptr)
        _logger = spdlog::stdout_color_mt(Config::logger_name());
}

GLViewport::~GLViewport() {
    SDL_GL_DeleteContext(_context);
}

bool GLViewport::init() {
    _logger->info("Creating OpenGL context");
    _context = SDL_GL_CreateContext(_window.window());
    if(_context == nullptr) {
        _last_error = SDL_GetError();
        return false;
    }

    SDL_GL_LoadLibrary(NULL);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    _logger->info("GL info:");
    _logger->info("Version: {}", glGetString(GL_VERSION));
    _logger->info("Vendor: {}", glGetString(GL_VENDOR));
    _logger->info("Renderer: {}", glGetString(GL_RENDERER));

    // Use v-sync
    if(SDL_GL_SetSwapInterval(1) < 0) {
        _logger->warn("Unable to set VSync, SDL Error: {}", SDL_GetError());
    }

    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if( glewError != GLEW_OK ) {
        _last_error = reinterpret_cast<const char*>(glewGetErrorString(glewError));
        return false;
    }

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    glEnable(GL_TEXTURE_2D);

    // Enable front face culling, since that's what Quake3 does
    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, _size.x, _size.y);

    glMatrixMode(GL_PROJECTION);                        // Select The Projection Matrix
    glLoadIdentity();                                   // Reset The Projection Matrix

    glMatrixMode(GL_MODELVIEW);                         // Select The Modelview Matrix
    glLoadIdentity();                                   // Reset The Modelview Matrix

    //GLuint programID = sys::GLSL::load_shaders(Config::project_name() + ".vert", Config::project_name() + ".frag");
    //if(programID == 0) {
        //_last_error = "Failed to load shaders";
        //return false;
    //}
    //_logger->info("Program ID: {}", programID);
    //glUseProgram(programID);

    return true;
}

void GLViewport::start_render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

}

void GLViewport::update_frame_rate() {
    static double fps   = 0.0f;
    static double last_time  = 0.0f;

    static double frame_time = 0.0f;

    double current_time = SDL_GetTicks() * 0.001f;


    _frame_interval = current_time - frame_time + 0.005f;

    frame_time = current_time;

    ++fps;

    if( current_time - last_time > 1.0f ) {
        last_time = current_time;

        //_logger->debug("FPS: {}", static_cast<int>(fps));

        fps = 0;
    }
}


void GLViewport::end_render() {
    SDL_GL_SwapWindow(_window.window());
    update_frame_rate();
}
