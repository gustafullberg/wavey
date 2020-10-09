#ifndef GPU_SPECTROGRAM_HPP
#define GPU_SPECTROGRAM_HPP

#include "spectrogram.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <vector>

class GpuSpectrogram {
   public:
    GpuSpectrogram(const Spectrogram& spectrogram, int samplerate);
    ~GpuSpectrogram();
    void Draw(int channel);

   private:
    GLuint vao = 0;
    GLuint vbo = 0;
    std::vector<GLuint> tex;
};

#endif
