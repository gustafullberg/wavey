#include "gpu_spectrogram.hpp"
#include <glm/glm.hpp>

namespace {
struct vertex {
    vertex(glm::vec2 pos, glm::vec2 texCoord) : pos(pos), texCoord(texCoord) {}
    glm::vec2 pos;
    glm::vec2 texCoord;
};
}  // namespace

GpuSpectrogram::GpuSpectrogram(const Spectrogram& spectrogram, int samplerate) {
    float length = 512.f * spectrogram.NumPowerSpectrum() / samplerate;
    std::vector<vertex> vertices;
    vertices.push_back(vertex(glm::vec2(0.f, -1.f), glm::vec2(0.f, 0.f)));
    vertices.push_back(vertex(glm::vec2(length, 1.f), glm::vec2(1.f, 1.f)));
    vertices.push_back(vertex(glm::vec2(0.f, 1.f), glm::vec2(1.f, 0.f)));
    vertices.push_back(vertex(glm::vec2(0.f, -1.f), glm::vec2(0.f, 0.f)));
    vertices.push_back(vertex(glm::vec2(length, -1.f), glm::vec2(0.f, 1.f)));
    vertices.push_back(vertex(glm::vec2(length, 1.f), glm::vec2(1.f, 1.f)));

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),
                          (const void*)offsetof(vertex, texCoord));
    glBindVertexArray(0);

    int num_channels = spectrogram.NumChannels();
    tex.resize(num_channels);
    glGenTextures(tex.size(), tex.data());
    for (int c = 0; c < num_channels; c++) {
        glBindTexture(GL_TEXTURE_2D, tex[c]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 513, spectrogram.NumPowerSpectrum(), 0, GL_RED,
                     GL_FLOAT, spectrogram.Data(c));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

GpuSpectrogram::~GpuSpectrogram() {
    glDeleteTextures(tex.size(), tex.data());
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void GpuSpectrogram::Draw(int channel) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[channel]);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
