#ifndef WAVE_SHADER_HPP
#define WAVE_SHADET_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

class WaveShader {
   public:
    void Init();
    void Terminate();
    void Draw(float left,
              float right,
              float top,
              float bottom,
              float track,
              float channel,
              float num_channels,
              float samplerate);

   private:
    GLuint program = 0;
};

#endif
