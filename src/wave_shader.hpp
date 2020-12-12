#ifndef WAVE_SHADER_HPP
#define WAVE_SHADER_HPP

#include <glm/glm.hpp>
#include "shader.hpp"

class WaveShader : public Shader {
   public:
    void Init();
    void Draw(const glm::mat4& mvp, float start_time, float samplerate, float vertical_zoom);
};

#endif
