#include "sample_point_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const std::string kVertexSource = R"(
layout(location = 0) in float y;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform float sample_time;
layout(location = 2) uniform float vertical_zoom;

void main() {
    float y_scaled = y * vertical_zoom;
    gl_Position = mvp * vec4(gl_VertexID * sample_time, y_scaled, 0.0, 1.0);

    if(y_scaled <= 1.0 && y_scaled >= -1.0) {
        gl_PointSize = 3.;
    } else {
        gl_PointSize = 1.;
    }
})";

const std::string kFragmentSource = R"(
out vec4 o;
layout(location = 3) uniform vec4 color;

void main() {
    o = color;
})";

}  // namespace

void SamplePointShader::Init(const glm::vec4& color) {
    Shader::Init(kVertexSource, kFragmentSource);
    glUseProgram(program);
    glUniform4fv(3, 1, glm::value_ptr(color));
}

void SamplePointShader::Draw(const glm::mat4& mvp, float samplerate, float vertical_zoom) {
    const float sample_time = 1.f / samplerate;
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(1, sample_time);
    glUniform1f(2, vertical_zoom);
}
