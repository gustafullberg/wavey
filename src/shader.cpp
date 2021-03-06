#include "shader.hpp"
#include <iostream>

namespace {
const std::string kShaderHeader =
    R"(#version 330 core
#extension GL_ARB_explicit_uniform_location : require
)";

GLuint CompileShader(GLenum shader_type, const std::string& source) {
    GLuint shader = glCreateShader(shader_type);
    const char* src = source.c_str();
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

void Shader::Init(const std::string& vertex_shader_source,
                  const std::string& fragment_shader_source) {
    program = glCreateProgram();
    const GLuint vertex_shader =
        CompileShader(GL_VERTEX_SHADER, kShaderHeader + vertex_shader_source);
    const GLuint fragment_shader =
        CompileShader(GL_FRAGMENT_SHADER, kShaderHeader + fragment_shader_source);
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void Shader::Terminate() {
    glDeleteProgram(program);
}
