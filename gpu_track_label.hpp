#ifndef GPU_TRACK_LABEL_HPP
#define GPU_TRACK_LABEL_HPP

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>

class GpuTrackLabel {
   public:
    GpuTrackLabel(const unsigned char* image_data, int width, int height);
    ~GpuTrackLabel();
    void Draw();
    float Width() const { return width; }
    float Height() const { return height; }
    GLuint Texture() const { return tex; }

   private:
    GLuint tex;
    int width;
    int height;
};

#endif
