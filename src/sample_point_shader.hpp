#ifndef SAMPLE_POINT_SHADER_HPP
#define SAMPLE_POINT_SHADER_HPP

#include <glm/glm.hpp>
#include "shader.hpp"

class SamplePointShader : public Shader {
   public:
    void Init(const glm::vec4& color);
    void Draw(const glm::mat4& mvp, float samplerate, float vertical_zoom);
};

#endif
