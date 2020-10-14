#include "gpu_waveform.hpp"
#include <chrono>
#include <cmath>
#include <iostream>

GpuWaveform::GpuWaveform(const AudioBuffer& ab) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    num_channels = ab.NumChannels();
    num_vertices = ab.NumFrames();
    samplerate = ab.Samplerate();

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

GpuWaveform::~GpuWaveform() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void GpuWaveform::Draw(int channel, float start_time, float end_time) {
    const int start_index = std::max(static_cast<int>(std::floor(start_time * samplerate)), 0);
    const int end_index =
        std::min(static_cast<int>(std::ceil(end_time * samplerate)), num_vertices);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, num_channels * sizeof(float),
                          (const void*)(channel * sizeof(float)));
    glDrawArrays(GL_LINE_STRIP, start_index, end_index - start_index);
}
