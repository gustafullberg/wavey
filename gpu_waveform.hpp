#ifndef GPU_WAVEFORM_HPP
#define GPU_WAVEFORM_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include "audio_buffer.hpp"

class GpuWaveform {
   public:
    GpuWaveform(const AudioBuffer& ab);
    ~GpuWaveform();
    void Draw(int channel);

   private:
    GLuint vao = 0;
    GLuint vbo = 0;
    int num_channels = 0;
    int num_vertices = 0;
};

#endif
