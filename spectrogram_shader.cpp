#include "spectrogram_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const char* kVertexSrc =
    "#version 330\n"
    "#extension GL_ARB_explicit_uniform_location : require\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 1) in vec2 tex_coord_v;\n"
    "out vec2 tex_coord_f;\n"
    "layout(location = 0) uniform mat4 mvp;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(pos, 0.0, 1.0);\n"
    "    tex_coord_f = tex_coord_v;\n"
    "}\n";

const char* kFragmentSrc =
    "#version 330\n"
    "#extension GL_ARB_explicit_uniform_location : require\n"
    "in vec2 tex_coord_f;\n"
    "out vec4 color;\n"
    "layout(location = 1) uniform sampler2D tex;\n"
    "\n"
    "void main() {\n"
    "    float lum = texture(tex, tex_coord_f).r;\n"
    "    color = vec4(vec3(.5, 1., 0.5) * lum, 1.0);\n"
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

void SpectrogramShader::Init() {
    program = glCreateProgram();
    glAttachShader(program, CompileShader(GL_VERTEX_SHADER, kVertexSrc));
    glAttachShader(program, CompileShader(GL_FRAGMENT_SHADER, kFragmentSrc));
    glLinkProgram(program);
    glUseProgram(program);
    glUniform1i(1, 0);
    glUseProgram(0);
}

void SpectrogramShader::Terminate() {
    glDeleteProgram(program);
}

void SpectrogramShader::Draw(const glm::mat4& mvp) {
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
}
