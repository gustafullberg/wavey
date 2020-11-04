#ifndef GPU_WAVEFORM_HPP
#define GPU_WAVEFORM_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include "audio_buffer.hpp"

class GpuWaveform {
   public:
    GpuWaveform(const AudioBuffer& ab);
    ~GpuWaveform();
    void Draw(int channel, float start_time, float end_time, bool low_res);

   private:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint vbo_lod = 0;
    int num_channels = 0;
    int num_vertices = 0;
    int num_vertices_lod = 0;
    int samplerate = 0;
};

#endif
