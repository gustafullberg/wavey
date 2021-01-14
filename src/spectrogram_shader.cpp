#include "spectrogram_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const std::string kVertexSource = R"(
layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 tex_coord_v;
out vec3 tex_coord_f;
layout(location = 0) uniform mat4 mvp;
layout(location = 5) uniform float start_time;

void main() {
    gl_Position = mvp * vec4(pos - vec2(start_time, 0.0), 0.0, 1.0);
    tex_coord_f = tex_coord_v;
})";

const std::string kFragmentSource = R"(
in vec3 tex_coord_f;
out vec4 color;
layout(location = 1) uniform sampler2DArray tex;
layout(location = 2) uniform float nyquist_freq;
layout(location = 3) uniform float bark_scaling;
layout(location = 4) uniform int use_bark;

void main() {
    const float dB_min = -100.;
    const float dB_max = -20.;
    float x = tex_coord_f.x;
    if(use_bark != 0) {
        x = 1960. * (bark_scaling * x + 0.53) / (nyquist_freq * (26.28 - bark_scaling * x));
    }
    float s = texture(tex, vec3(x, tex_coord_f.yz)).r;
    const vec4 c0 = vec4(0., 0., 0., 1.);
    const vec4 c1 = vec4(0., 0., .5, 1.);
    const vec4 c2 = vec4(1., 0., 0., 1.);
    const vec4 c3 = vec4(1., 1., 0., 1.);
    const vec4 c4 = vec4(1., 1., 1., 1.);
    if(s < 0.25) {
        color = mix(c0, c1, 4. * s);
    } else if(s < 0.5) {
        color = mix(c1, c2, 4. * (s-0.25));
    } else if(s < 0.75) {
        color = mix(c2, c3, 4. * (s-0.5));
    } else {
        color = mix(c3, c4, 4. * (s-0.75));
    }
})";

}  // namespace

void SpectrogramShader::Init() {
    Shader::Init(kVertexSource, kFragmentSource);
    glUseProgram(program);
    glUniform1i(1, 0);
    glUseProgram(0);
}

void SpectrogramShader::Draw(const glm::mat4& mvp, float start_time, float samplerate, bool bark) {
    float nyquist_freq = .5f * samplerate;
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform1f(2, nyquist_freq);
    glUniform1f(3, 26.81f * nyquist_freq / (1960.f + nyquist_freq) - 0.53f);
    glUniform1i(4, bark);
    glUniform1f(5, start_time);
}
