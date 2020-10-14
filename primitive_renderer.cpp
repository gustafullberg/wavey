#include "primitive_renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

namespace {

constexpr float vertices[] = {0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f};

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
    "#extension GL_ARB_explicit_uniform_location : require\n"
    "layout(location = 1) uniform vec4 color;\n"
    "out vec4 o;\n"
    "\n"
    "void main() {\n"
    "    o = color;\n"
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

void PrimitiveRenderer::Init() {
    // Shader.
    program = glCreateProgram();
    const GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, kVertexSrc);
    const GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, kFragmentSrc);
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

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

void PrimitiveRenderer::Terminate() {
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void PrimitiveRenderer::DrawLine(const glm::mat4& mvp,
                                 const glm::vec2& from,
                                 const glm::vec2& to,
                                 const glm::vec4& color) {
    Draw(mvp, from, to, color, GL_LINES, 2);
}

void PrimitiveRenderer::DrawQuad(const glm::mat4& mvp,
                                 const glm::vec2& from,
                                 const glm::vec2& to,
                                 const glm::vec4& color) {
    Draw(mvp, from, to, color, GL_TRIANGLES, 6);
}

void PrimitiveRenderer::Draw(const glm::mat4& mvp,
                             const glm::vec2& from,
                             const glm::vec2& to,
                             const glm::vec4& color,
                             GLenum mode,
                             int count) {
    const glm::mat4 m = glm::scale(glm::translate(mvp, glm::vec3(from, 0.f)),
                                   glm::vec3(to.x - from.x, to.y - from.y, 1.f));
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(m));
    glUniform4fv(1, 1, glm::value_ptr(color));

    glBindVertexArray(vao);
    glDrawArrays(mode, 0, count);
    glBindVertexArray(0);

    glUseProgram(0);
}
