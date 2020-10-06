#include "quad_renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

namespace {

constexpr float vertices[] = {0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 0.f};

const char* kVertexSrc =
    "#version 330\n"
    "#extension GL_ARB_explicit_uniform_location : require\n"
    "layout(location = 0) in vec2 pos;\n"
    "layout(location = 0) uniform mat4 mvp;\n"
    "\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(pos, 0.0, 1.0);\n"
    "}\n";

const char* kFragmentSrc =
    "#version 330\n"
    "out vec4 color;\n"
    "\n"
    "void main() {\n"
    "    color = vec4(.5, 0.9, 0.5, 0.1);\n"
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

void QuadRenderer::Init() {
    // Shader.
    program = glCreateProgram();
    glAttachShader(program, CompileShader(GL_VERTEX_SHADER, kVertexSrc));
    glAttachShader(program, CompileShader(GL_FRAGMENT_SHADER, kFragmentSrc));
    glLinkProgram(program);

    // Vertex array object.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
}

void QuadRenderer::Terminate() {
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void QuadRenderer::Draw(float x, float y, float size_x, float size_y) {
    glm::mat4 m = glm::ortho(0.f, 1.f, 1.f, 0.f, -1.f, 1.f);
    m = glm::translate(m, glm::vec3(x, y, 0.0f));
    m = glm::scale(m, glm::vec3(size_x, size_y, 1.0f));

    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m));
    // glUniform4fv(shader.getColorLocation(), 1, glm::value_ptr(color));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(vertices) / (2 * sizeof(float)));
    glBindVertexArray(0);

    glUseProgram(0);
}
