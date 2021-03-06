#include "label_renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

namespace {

constexpr float vertices[] = {0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f};

const std::string kVertexSource = R"(
layout(location = 0) in vec2 pos;
out vec2 tex_coord_f;
layout(location = 0) uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(pos, 0.0, 1.0);
    tex_coord_f = pos;
})";

const std::string kFragmentSource = R"(
in vec2 tex_coord_f;
out vec4 o;
layout(location = 1) uniform sampler2D tex;
layout(location = 2) uniform vec3 color;

void main() {
    float a = texture(tex, tex_coord_f).r;
    o = vec4(color, a);
})";
}  // namespace

void LabelRenderer::LabelShader::Init() {
    Shader::Init(kVertexSource, kFragmentSource);
    glUseProgram(program);
    glUniform1i(1, 0);
    glUseProgram(0);
}

void LabelRenderer::LabelShader::Draw(const glm::mat4& mvp, const glm::vec3& color) {
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(2, 1, glm::value_ptr(color));
}

void LabelRenderer::Init() {
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

void LabelRenderer::Terminate() {
    shader.Terminate();
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void LabelRenderer::Draw(const GpuTrackLabel& label,
                         float y,
                         float win_width,
                         float win_height,
                         float scale_factor,
                         bool selected) {
    glm::vec3 color_shadow = glm::vec3(.1f, 0.2f, 0.1f);
    glm::vec3 color_text = selected ? glm::vec3(.9f, 0.9f, 0.5f) : glm::vec3(.5f, 0.9f, 0.5f);
    glm::mat4 mvp = glm::ortho(0.f, win_width, win_height, 0.f, -1.f, 1.f);
    glm::mat4 mvp_shadow = glm::translate(mvp, glm::vec3(1.f, y + 1.f, 0.f));
    mvp_shadow = glm::scale(
        mvp_shadow, glm::vec3(label.Width() * scale_factor, label.Height() * scale_factor, 1.f));
    mvp = glm::translate(mvp, glm::vec3(0.f, y, 0.f));
    mvp = glm::scale(mvp,
                     glm::vec3(label.Width() * scale_factor, label.Height() * scale_factor, 1.f));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, label.Texture());
    glBindVertexArray(vao);
    shader.Draw(mvp_shadow, color_shadow);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    shader.Draw(mvp, color_text);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
