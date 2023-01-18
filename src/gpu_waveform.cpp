#include "gpu_waveform.hpp"
#include <cmath>
#include <iostream>

GpuWaveform::GpuWaveform(const AudioBuffer& ab, const std::vector<float>& buffer_lod) {
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

    num_vertices_lod = num_channels ? buffer_lod.size() / num_channels : 0;
    glGenBuffers(1, &vbo_lod);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_lod);
    glBufferData(GL_ARRAY_BUFFER, buffer_lod.size() * sizeof(float), buffer_lod.data(),
                 GL_STATIC_DRAW);
    glBindVertexArray(0);
}

GpuWaveform::~GpuWaveform() {
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &vbo_lod);
    glDeleteVertexArrays(1, &vao);
}

void GpuWaveform::Draw(int channel,
                       float start_time,
                       float end_time,
                       bool draw_low_res,
                       bool draw_points) {
    const float rate = draw_low_res ? samplerate / 1000.f * 2.f : samplerate;
    const int last_index = (draw_low_res ? num_vertices_lod : num_vertices) - 1;
    const int start_index = std::max(static_cast<int>(std::floor(start_time * rate)), 0);
    const int end_index = std::min(
        start_index + static_cast<int>(std::ceil((end_time - start_time) * rate)), last_index);

    if (end_index >= start_index) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, draw_low_res ? vbo_lod : vbo);
        glVertexAttribPointer(
            0, 1, GL_FLOAT, GL_FALSE, num_channels * sizeof(float),
            (const void*)((start_index * num_channels + channel) * sizeof(float)));
        glDrawArrays(draw_points ? GL_POINTS : GL_LINE_STRIP, 0, end_index - start_index + 1);
    }
}

void GpuWaveform::DrawLines(int channel, float start_time, float end_time, bool draw_low_res) {
    Draw(channel, start_time, end_time, draw_low_res, false);
}

void GpuWaveform::DrawPoints(int channel, float start_time, float end_time, bool draw_low_res) {
    Draw(channel, start_time, end_time, draw_low_res, true);
}
