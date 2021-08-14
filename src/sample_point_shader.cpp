#include "sample_point_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const std::string kVertexSource = R"(
layout(location = 0) in float y;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform float sample_time;
layout(location = 2) uniform float vertical_zoom;
out float y_scaled;

void main() {
    y_scaled = y * vertical_zoom;
    gl_Position = mvp * vec4(gl_VertexID * sample_time, y_scaled, 0.0, 1.0);
})";

const std::string kGeometrySource = R"(
layout(points) in;
layout(points, max_vertices = 1) out;
in float y_scaled[];

void main() {
    vec4 pos_sample = gl_in[0].gl_Position;

    if(y_scaled[0] <= 1. && y_scaled[0] >= -1.) {
        gl_Position = pos_sample;
        gl_PointSize = 3.;
        EmitVertex();
    }
})";

const std::string kFragmentSource = R"(
out vec4 color;

void main() {
    color = vec4(0.5, 0.9, 0.5, 1.0);
})";

}  // namespace

void SamplePointShader::Init() {
    Shader::Init(kVertexSource, kGeometrySource, kFragmentSource);
}

void SamplePointShader::Draw(const glm::mat4& mvp, float samplerate, float vertical_zoom) {
    const float sample_time = 1.f / samplerate;
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(1, sample_time);
    glUniform1f(2, vertical_zoom);
}
