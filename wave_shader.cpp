#include "wave_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const char* kVertexSrc =
    "#version 330\n"
    "#extension GL_ARB_explicit_uniform_location : require\n"
    "layout(location = 0) in float y;\n"
    "layout(location = 0) uniform mat4 mvp;\n"
    "layout(location = 1) uniform float sample_time;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(gl_VertexID * sample_time, y, 0.0, 1.0);\n"
    "}\n";

const char* kFragmentSrc =
    "#version 330\n"
    "out vec4 color;\n"
    "\n"
    "void main() {\n"
    "    color = vec4(.5, 0.9, 0.5, 1.0);\n"
    "}\n";

GLuint CompileShader(GLenum shader_type, const char* src) {
    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(log_len + 1, ' ');
        glGetShaderInfoLog(shader, log_len, nullptr, (GLchar*)log.c_str());
        std::cerr << "Shader: " << log;
    }
    return shader;
}

}  // namespace

void WaveShader::Init() {
    program = glCreateProgram();
    const GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, kVertexSrc);
    const GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, kFragmentSrc);
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void WaveShader::Terminate() {
    glDeleteProgram(program);
}

void WaveShader::Draw(const glm::mat4& mvp, float samplerate) {
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    float sample_time = 1.f / samplerate;
    glUniform1f(1, sample_time);
}
