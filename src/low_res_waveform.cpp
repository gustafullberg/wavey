#include "low_res_waveform.hpp"
#include <iostream>

namespace {
void MakeLowResBuffer(std::vector<float>& buffer,
                      const AudioBuffer& ab,
                      const int down_sampling_factor) {
    const int num_channels = ab.NumChannels();
    const int num_vertices = ab.NumFrames();
    const float* samples = ab.Samples();

    // Each channel has 2 output samples for every 1000 input samples.
    buffer.resize(2 * num_channels *
                  ((num_vertices + down_sampling_factor - 1) / down_sampling_factor));

    int dest_idx = 0;
    for (int i = 0; i < num_vertices; i += down_sampling_factor) {
        const int end = std::min(i + down_sampling_factor, num_vertices);
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

LowResWaveform::LowResWaveform(const AudioBuffer& ab) {
    MakeLowResBuffer(buffer, ab, 1000);
}
