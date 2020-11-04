#include "primitive_renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>

namespace {

constexpr float vertices[] = {0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f};

const std::string kVertexSource = R"(
layout(location = 0) in vec2 pos;
layout(location = 0) uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(pos, 0.0, 1.0);
})";

const std::string kFragmentSource = R"(
layout(location = 1) uniform vec4 color;
out vec4 o;
   
void main() {
    o = color;
})";
}  // namespace

void PrimitiveRenderer::PrimitiveShader::Init() {
    Shader::Init(kVertexSource, kFragmentSource);
}

void PrimitiveRenderer::PrimitiveShader::Draw(const glm::mat4& mvp, const glm::vec4& color) {
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform4fv(1, 1, glm::value_ptr(color));
}

void PrimitiveRenderer::Init() {
    // Shader.
    shader.Init();

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
    shader.Terminate();
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
    shader.Draw(m, color);

    glBindVertexArray(vao);
    glDrawArrays(mode, 0, count);
    glBindVertexArray(0);
}
