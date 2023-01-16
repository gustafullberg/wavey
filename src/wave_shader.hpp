#ifndef WAVE_SHADER_HPP
#define WAVE_SHADER_HPP

#include <glm/glm.hpp>
#include "shader.hpp"

class WaveShader : public Shader {
   public:
    void Init(const glm::vec4& color);
    void Draw(const glm::mat4& mvp,
              float samplerate,
              float vertical_zoom,
              bool db_vertical_scale = false);
};

#endif
