#include "spectrogram_shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

namespace {

const std::string kVertexSource = R"(
layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 tex_coord_v;
out vec3 tex_coord_f;
layout(location = 0) uniform mat4 mvp;

void main() {
    gl_Position = mvp * vec4(pos, 0.0, 1.0);
    tex_coord_f = tex_coord_v;
})";

const std::string kFragmentSource = R"(
in vec3 tex_coord_f;
out vec4 color;
layout(location = 1) uniform sampler2DArray tex;

void main() {
    const float dB_min = -100.;
    const float dB_max = -20.;
    float dB = texture(tex, tex_coord_f).r;
    float s = (dB - dB_min) * (1. / (dB_max - dB_min));
    s = max(0., min(1., s));
    const vec4 c0 = vec4(0., 0., 0., 0.);
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

void SpectrogramShader::Draw(const glm::mat4& mvp) {
    glUseProgram(program);
    glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(mvp));
}
