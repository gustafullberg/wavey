#ifndef SAMPLE_LINE_SHADER_HPP
#define SAMPLE_LINE_SHADER_HPP

#include <glm/glm.hpp>
#include "shader.hpp"

class SampleLineShader : public Shader {
   public:
    void Init();
    void Draw(const glm::mat4& mvp, float samplerate, float vertical_zoom);
};

#endif
