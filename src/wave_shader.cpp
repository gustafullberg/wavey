#include "wave_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const std::string kVertexSource = R"(
layout(location = 0) in float y;
layout(location = 0) uniform mat4 mvp;
layout(location = 1) uniform float sample_time;
layout(location = 2) uniform float vertical_zoom;
layout(location = 4) uniform bool db_vertical_scale;


void main() {
    float y_ = y;
    if(db_vertical_scale) {
        const float epsilon = 0.0001;
        const float log10 = log(10.0);
        if(y_ > 0.) {
            y_ = max(20. * log(y_ + epsilon) / log10, -60.) / 60.  + 1.;
        } else if (y_ < 0.) {
            y_ =  -1. * max(20. * log(-1. * y_ + epsilon) / log10, -60.) / 60. - 1.;
        } else {
            y_ = 0.;
        }
    }
    float y_scaled = clamp(y_ * vertical_zoom, -1., 1.);
    gl_Position = mvp * vec4(float(gl_VertexID) * sample_time, y_scaled, 0., 1.);
})";

const std::string kFragmentSource = R"(
out vec4 o;
layout(location = 3) uniform vec4 color;

void main() {
    o = color;
})";

}  // namespace

void WaveShader::Init(const glm::vec4& color) {
    Shader::Init(kVertexSource, kFragmentSource);
    glUseProgram(program);
    glUniform4fv(3, 1, glm::value_ptr(color));
}

void WaveShader::Draw(const glm::mat4& mvp,
                      float samplerate,
                      float vertical_zoom,
                      bool db_vertical_scale) {
    const float sample_time = 1.f / samplerate;
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(1, sample_time);
    glUniform1f(2, vertical_zoom);
    glUniform1f(4, db_vertical_scale);
}
