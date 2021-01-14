#include "gpu_spectrogram.hpp"
#include <chrono>
#include <glm/glm.hpp>
#include <iostream>

namespace {
struct vertex {
    vertex(glm::vec2 pos, glm::vec3 texCoord) : pos(pos), texCoord(texCoord) {}
    glm::vec2 pos;
    glm::vec3 texCoord;
};

int NumTiles(int size, int max_size) {
    int num_tiles = 1;
    size -= max_size;
    while (size > 0) {
        size -= max_size - 1;
        num_tiles++;
    }
    return num_tiles;
}
}  // namespace

GpuSpectrogram::GpuSpectrogram(const Spectrogram& spectrogram, int samplerate) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    // Time duration of one spectrum.
    const float spectrum_duration = static_cast<float>(spectrogram.Advance()) / samplerate;
    const int num_spectrum = spectrogram.NumPowerSpectrumPerChannel();

    // Maximum supported texture size determines number of spectra per tile.
    int num_spectra_per_tile;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &num_spectra_per_tile);
    num_spectra_per_tile = std::min(num_spectra_per_tile, num_spectrum);

    // Number of tiles (depth) of texture array. Tiles share one spectrum with neighboring tiles in
    // order to support perfect transitions between tiles.
    const int num_tiles = NumTiles(num_spectrum, num_spectra_per_tile);

    // Allocate texture storage for all tiles.
    const int num_channels = spectrogram.NumChannels();
    const int width = spectrogram.OutputSize();
    tex.resize(num_channels);
    glGenTextures(tex.size(), tex.data());
    for (int c = 0; c < num_channels; c++) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, tex[c]);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_PACK_ALIGNMENT, 2);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R16, width, num_spectra_per_tile, num_tiles);
    }

    std::vector<vertex> vertices;
    for (int tile = 0; tile < num_tiles; tile++) {
        const int first_spectrum_of_tile = tile * (num_spectra_per_tile - 1);
        // Upload one texture tile per channel.
        const int height = std::min((num_spectrum - first_spectrum_of_tile), num_spectra_per_tile);
        for (int c = 0; c < num_channels; c++) {
            glBindTexture(GL_TEXTURE_2D_ARRAY, tex[c]);
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, tile, width, height, 1, GL_RED,
                            GL_UNSIGNED_SHORT, spectrogram.Data(c, first_spectrum_of_tile));
        }

        // Skip half a texel at the beginning and end of the tiles to achieve seamless borders.
        const float tex_start = 0.5f / num_spectra_per_tile;
        const float tex_end = (height - 0.5f) / num_spectra_per_tile;

        // Time location for this tile.
        const float time_start = first_spectrum_of_tile * spectrum_duration;
        const float time_end = (first_spectrum_of_tile + height - 1) * spectrum_duration;

        // Vertices with texture coordinates for the tile. Same vertices are shared by all channels.
        vertices.push_back({glm::vec2(time_start, -1.f), glm::vec3(0.f, tex_start, tile)});
        vertices.push_back({glm::vec2(time_end, 1.f), glm::vec3(1.f, tex_end, tile)});
        vertices.push_back({glm::vec2(time_start, 1.f), glm::vec3(1.f, tex_start, tile)});
        vertices.push_back({glm::vec2(time_start, -1.f), glm::vec3(0.f, tex_start, tile)});
        vertices.push_back({glm::vec2(time_end, -1.f), glm::vec3(0.f, tex_end, tile)});
        vertices.push_back({glm::vec2(time_end, 1.f), glm::vec3(1.f, tex_end, tile)});
    }

    // Vertex array object for storing vertices with texture coordinates.
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),
                          (const void*)offsetof(vertex, texCoord));
    glBindVertexArray(0);
    num_vertices = vertices.size();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cerr << "Spectrogram uploaded to GPU in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms"
              << std::endl;
}

GpuSpectrogram::~GpuSpectrogram() {
    glDeleteTextures(tex.size(), tex.data());
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

void GpuSpectrogram::Draw(int channel) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex[channel]);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}
