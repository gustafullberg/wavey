#ifndef SPECTROGRAM_SHADER_HPP
#define SPECTROGRAM_SHADER_HPP

#include <glm/glm.hpp>
#include "shader.hpp"

class SpectrogramShader : public Shader {
   public:
    void Init();
    void Draw(const glm::mat4& mvp);
};

#endif
