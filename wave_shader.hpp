#ifndef WAVE_SHADER_HPP
#define WAVE_SHADET_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <glm/glm.hpp>

class WaveShader {
   public:
    void Init();
    void Terminate();
    void Draw(const glm::mat4& mvp, float samplerate);

   private:
    GLuint program = 0;
};

#endif
