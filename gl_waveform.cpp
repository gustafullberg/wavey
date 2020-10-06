#include "gl_waveform.hpp"
#include <chrono>
#include <iostream>

GLWaveform::GLWaveform(const AudioBuffer& ab) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    num_channels = ab.NumChannels();
    num_vertices = ab.NumFrames();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, ab.NumChannels() * ab.NumFrames() * sizeof(float), ab.Samples(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cerr << "Waveform uploaded to GPU in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms"
              << std::endl;
}

GLWaveform::~GLWaveform() {
    if (num_channels) {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
}

void GLWaveform::Draw(int channel) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, num_channels * sizeof(float),
                          (const void*)(channel * sizeof(float)));
    glDrawArrays(GL_LINE_STRIP, 0, num_vertices);
}
