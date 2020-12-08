#include "wave_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const std::string kVertexSource = R"(
layout(location = 0) in float y;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform float sample_time;
layout(location = 2) uniform float vertical_zoom;

void main() {
    float y_scaled = clamp(y * vertical_zoom, -1., 1.);
    gl_Position = mvp * vec4(gl_VertexID * sample_time, y_scaled, 0.0, 1.0);
})";

const std::string kFragmentSource = R"(
out vec4 color;

void main() {
    color = vec4(.5, 0.9, 0.5, 1.0);
})";

}  // namespace

void WaveShader::Init() {
    Shader::Init(kVertexSource, kFragmentSource);
}

void WaveShader::Draw(const glm::mat4& mvp, float samplerate, float vertical_zoom) {
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    float sample_time = 1.f / samplerate;
    glUniform1f(1, sample_time);
    glUniform1f(2, vertical_zoom);
}
