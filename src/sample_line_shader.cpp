#include "sample_line_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const std::string kVertexSource = R"(
layout(location = 0) in float y;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform float sample_time;
layout(location = 2) uniform float vertical_zoom;
out float y0;

void main() {
    float y_scaled = clamp(y * vertical_zoom, -1., 1.);
    gl_Position = mvp * vec4(gl_VertexID * sample_time, y_scaled, 0.0, 1.0);

    // Y-coord at amplitude 0.
    y0 = mvp[3].y;
})";

const std::string kGeometrySource = R"(
layout(points) in;
layout(line_strip, max_vertices = 2) out;
in float y0[];

void main() {
    vec4 pos_sample = gl_in[0].gl_Position;
    vec4 pos_base = pos_sample;
    pos_base.y = y0[0];

    gl_Position = pos_base;
    EmitVertex();

    gl_Position = pos_sample;
    EmitVertex();

    EndPrimitive();
})";

const std::string kFragmentSource = R"(
out vec4 color;

void main() {
    color = vec4(0.5, 0.9, 0.5, 0.3);
})";

}  // namespace

void SampleLineShader::Init() {
    Shader::Init(kVertexSource, kGeometrySource, kFragmentSource);
}

void SampleLineShader::Draw(const glm::mat4& mvp, float samplerate, float vertical_zoom) {
    const float sample_time = 1.f / samplerate;
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(1, sample_time);
    glUniform1f(2, vertical_zoom);
}
