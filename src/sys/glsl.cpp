#include <cerrno>
#include <cstring>
#include <fstream>

#include <GL/glew.h>
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <game/config.h>
#include <game/sys/glsl.h>

using namespace game::sys;
std::shared_ptr<spdlog::logger> GLSL::_logger;
int32_t GLSL::_program_id = 0;

bool GLSL::load_file_to_string(const std::string &path, std::string &data) {
    std::ifstream stream(path, std::ios::in);
    if(!stream.is_open()) {
        _logger->error("Failed to open file: {}: {}", path, strerror(errno));
        return false;
    }
    std::string line = "";
    while(getline(stream, line)) data += "\n" + line;
    return true;
}

int32_t GLSL::compile_shader(uint32_t id, const char *const code) {
    GLint result = GL_FALSE;
    int32_t info_log_length;
    // Compile Vertex Shader
    glShaderSource(id, 1, &code , NULL);
    glCompileShader(id);

    // Check Vertex Shader
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(info_log_length > 0) {
        char err_msg_buff[info_log_length + 1];
        glGetShaderInfoLog(id, info_log_length, NULL, err_msg_buff);
        _logger->error("Failed to compile shader: {}", reinterpret_cast<char*>(err_msg_buff));
    }
    return result;
}

uint32_t GLSL::load_shaders(const std::string &vertex_path,
                            const std::string &fragment_path) {
    _logger = spdlog::get(Config::logger_name());
    if(_logger == nullptr)
        _logger = spdlog::stdout_color_mt(Config::logger_name());

    // Create the shaders
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string vertex_shader_code, fragment_shader_code;
    bool rc = false;
    rc = load_file_to_string(Config::shader_path() + vertex_path, vertex_shader_code);
    if(!rc) return 0;

    rc = load_file_to_string(Config::shader_path() + fragment_path, fragment_shader_code);
    if(!rc) return 0;



    _logger->info("Compiling vertex shader {} from file: {}",
                  vertex_shader_id, vertex_path);
    GLint result = compile_shader(vertex_shader_id, vertex_shader_code.c_str());
    if(result != GL_TRUE) return 0;

    _logger->info("Compiling fragment shader {} from file: {}",
                  fragment_shader_id, fragment_path);
    result = compile_shader(fragment_shader_id, fragment_shader_code.c_str());
    if(result != GL_TRUE) return 0;

    int32_t info_log_length;

    // Link the program
    _logger->info("Linking program");
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);

    // Check the program
    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
    if(info_log_length > 0) {
        char err_msg_buff[info_log_length + 1];
        glGetProgramInfoLog(program_id, info_log_length, NULL, err_msg_buff);
        _logger->error("Failed to link shaders: {}", reinterpret_cast<char*>(err_msg_buff));
    }

    glDetachShader(program_id, vertex_shader_id);
    glDetachShader(program_id, fragment_shader_id);

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    _program_id = program_id;
    return program_id;

}
