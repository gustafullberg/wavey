#include "gpu_label.hpp"

GpuLabel::GpuLabel(const unsigned char* image_data, int width, int height)
    : width(width), height(height) {
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 image_data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GpuLabel::~GpuLabel() {
    glDeleteTextures(1, &tex);
}

void GpuLabel::Draw() {}
