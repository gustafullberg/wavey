#include "gpu_waveform.hpp"
#include <chrono>
#include <cmath>
#include <iostream>

namespace {
void MakeLowResBuffer(std::vector<float>& buffer, const AudioBuffer& ab) {
    const int num_channels = ab.NumChannels();
    const int num_vertices = ab.NumFrames();
    const float* samples = ab.Samples();

    // Each channel has 2 output samples for every 1000 input samples.
    buffer.resize(2 * num_channels * ((num_vertices + 999) / 1000));

    int dest_idx = 0;
    for (int i = 0; i < num_vertices; i += 1000) {
        const int end = std::min(i + 1000, num_vertices);
        for (int c = 0; c < num_channels; c++) {
            float min = 1.f;
            float max = -1.f;
            for (int k = i; k < end; k++) {
                min = std::min(min, samples[num_channels * k + c]);
                max = std::max(max, samples[num_channels * k + c]);
            }
            buffer[dest_idx + c] = min;
            buffer[dest_idx + c + num_channels] = max;
        }
        dest_idx += 2 * num_channels;
    }
}
}  // namespace

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

    std::vector<float> buffer_lod;
    MakeLowResBuffer(buffer_lod, ab);
    num_vertices_lod = buffer_lod.size() / num_channels;

    glGenBuffers(1, &vbo_lod);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_lod);
    glBufferData(GL_ARRAY_BUFFER, buffer_lod.size() * sizeof(float), buffer_lod.data(),
                 GL_STATIC_DRAW);
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

void GpuWaveform::Draw(int channel, float start_time, float end_time, bool low_res) {
    if (low_res) {
        const int start_index =
            std::max(static_cast<int>(std::floor(start_time * samplerate / 1000.f * 2.f)), 0);
        const int end_index = std::min(
            static_cast<int>(std::ceil(end_time * samplerate / 1000.f * 2.f)), num_vertices_lod);
        // int start_index = 0;
        // int end_index = num_vertices_lod;
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_lod);
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, num_channels * sizeof(float),
                              (const void*)(channel * sizeof(float)));
        glDrawArrays(GL_LINES, start_index, end_index - start_index);
    } else {
        const int start_index = std::max(static_cast<int>(std::floor(start_time * samplerate)), 0);
        const int end_index =
            std::min(static_cast<int>(std::ceil(end_time * samplerate)), num_vertices);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, num_channels * sizeof(float),
                              (const void*)(channel * sizeof(float)));
        glDrawArrays(GL_LINE_STRIP, start_index, end_index - start_index);
    }
}
