#include <thread>
#include <chrono>
#include <iostream>
#include <vector>

#include <game/game.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

#include <game/config.h>
#include <game/render/gl_window.h>
#include <game/render/gl_viewport.h>
#include <game/input.h>
#include <game/sys/glsl.h>

#include <game/entity.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

using namespace game;
using namespace game::render;

// Our vertices. Three consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
static const GLfloat g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f, // triangle 1 : begin
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f, // triangle 1 : end
    1.0f, 1.0f,-1.0f, // triangle 2 : begin
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f, // triangle 2 : end
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f,-1.0f,
    1.0f,-1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f,-1.0f, 1.0f
};

// One color for each vertex. They were generated randomly.
static const GLfloat g_color_buffer_data[] = {
    0.583f,  0.771f,  0.014f,
    0.609f,  0.115f,  0.436f,
    0.327f,  0.483f,  0.844f,
    0.822f,  0.569f,  0.201f,
    0.435f,  0.602f,  0.223f,
    0.310f,  0.747f,  0.185f,
    0.597f,  0.770f,  0.761f,
    0.559f,  0.436f,  0.730f,
    0.359f,  0.583f,  0.152f,
    0.483f,  0.596f,  0.789f,
    0.559f,  0.861f,  0.639f,
    0.195f,  0.548f,  0.859f,
    0.014f,  0.184f,  0.576f,
    0.771f,  0.328f,  0.970f,
    0.406f,  0.615f,  0.116f,
    0.676f,  0.977f,  0.133f,
    0.971f,  0.572f,  0.833f,
    0.140f,  0.616f,  0.489f,
    0.997f,  0.513f,  0.064f,
    0.945f,  0.719f,  0.592f,
    0.543f,  0.021f,  0.978f,
    0.279f,  0.317f,  0.505f,
    0.167f,  0.620f,  0.077f,
    0.347f,  0.857f,  0.137f,
    0.055f,  0.953f,  0.042f,
    0.714f,  0.505f,  0.345f,
    0.783f,  0.290f,  0.734f,
    0.722f,  0.645f,  0.174f,
    0.302f,  0.455f,  0.848f,
    0.225f,  0.587f,  0.040f,
    0.517f,  0.713f,  0.338f,
    0.053f,  0.959f,  0.120f,
    0.393f,  0.621f,  0.362f,
    0.673f,  0.211f,  0.457f,
    0.820f,  0.883f,  0.371f,
    0.982f,  0.099f,  0.879f
};



// This will identify our vertex buffer
GLuint vertexbuffer;
GLuint colorbuffer;

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
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &vertexbuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

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
	auto width = Config::window_width();
	auto height = Config::window_height();


	// position
	glm::vec3 position = glm::vec3( 0, 0, 5 );
	// horizontal angle : toward -Z
	float horizontalAngle = 3.14f;
	// vertical angle : 0, look at the horizon
	float verticalAngle = 0.0f;
	// Initial Field of View
	float initialFoV = 45.0f;

	float speed = 0.05f; // 3 units / second
	float mouseSpeed = 0.0005f;
	// Get mouse position
	float xpos = 0.0f, ypos = 0.0f;
	SDL_SetRelativeMouseMode(SDL_TRUE);

	double lastTime = SDL_GetTicks();
	//SDL_SetWindowGrab(_window->window(), SDL_TRUE);
	//SDL_ShowCursor(0);


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

	double currentTime = SDL_GetTicks();
	float deltaTime = static_cast<float>(currentTime - lastTime);
	lastTime = currentTime;

	//float rx = (float)(width / 2.0f) - (float)xpos;
	//float ry = (float)(height / 2.0f) - (float)ypos;
	float rx = xpos;
	float ry = ypos;

	// Compute new orientation
	horizontalAngle -= mouseSpeed * deltaTime * (rx);
	verticalAngle   -= mouseSpeed * deltaTime * (ry);

	_logger->info("m {} {} {} {} {} {} {}", xpos, ypos, deltaTime, rx, ry, horizontalAngle, verticalAngle);

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
    	cos(verticalAngle) * sin(horizontalAngle),
    	sin(verticalAngle),
    	cos(verticalAngle) * cos(horizontalAngle)
	);

	// Right vector
	glm::vec3 right = glm::vec3(
    	sin(horizontalAngle - M_PI/2.0f),
    	0,
    	cos(horizontalAngle - M_PI/2.0f)
	);

	if(Action::action("game.quit").is(Action::State::Active)){
		is_running = false;
	}

	if(Action::action("movement.forward").is(Action::State::Active)){
		position += direction * deltaTime * speed;
	}
	if(Action::action("movement.back").is(Action::State::Active)){
		position -= direction * deltaTime * speed;
	}
	if(Action::action("movement.right").is(Action::State::Active)){
		position += right * deltaTime * speed;
	}
	if(Action::action("movement.left").is(Action::State::Active)){
		position -= right * deltaTime * speed;
	}



	// Up vector : perpendicular to both direction and right
	glm::vec3 up = glm::cross( right, direction );


	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), (float) width / (float)height, 0.1f, 100.0f);

	// Or, for an ortho camera :
	//glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f); // In world coordinates

	// Camera matrix
	glm::mat4 View = glm::lookAt(
		position, // Camera is at (4,3,3), in World Space
		position + direction, // and looks at the origin
		up  // Head is up (set to 0,-1,0 to look upside-down)
		);

	// Model matrix : an identity matrix (model will be at the origin)
	glm::mat4 Model = glm::mat4(1.0f);
	// Our ModelViewProjection : multiplication of our 3 matrices
	glm::mat4 mvp = Projection * View * Model; // Remember, matrix multiplication is the other way around

	// Get a handle for our "MVP" uniform
	// Only during the initialisation
	GLuint MatrixID = glGetUniformLocation(sys::GLSL::program_id(), "MVP");

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// This is done in the main loop since each model will have a different MVP matrix (At least for the M part)
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);

		_view->start_render();
		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,         // attribute 0. No particular reason for 0, but must match the layout in the shader.
			3,         // size
			GL_FLOAT,  // type
			GL_FALSE,  // normalized?
			0,         // stride
			(void*)0   // array buffer offset
		);
		// 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
    		1,         // attribute. No particular reason for 1, but must match the layout in the shader.
    		3,         // size
    		GL_FLOAT,  // type
    		GL_FALSE,  // normalized?
    		0,         // stride
    		(void*)0   // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, sizeof(g_vertex_buffer_data)); // Starting from vertex 0; 3 vertices total -> 1 triangle
		glDisableVertexAttribArray(0);
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
