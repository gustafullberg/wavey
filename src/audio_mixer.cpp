#include "audio_mixer.hpp"

#include <cstdio>

AudioMixer::AudioMixer(int num_input_channels, int num_output_channels)
    : gain(num_output_channels, std::vector<float>(num_input_channels)),
      num_output_channels_(num_output_channels),
      num_input_channels_(num_input_channels) {
    printf("creating mixer output channels: %d, input channels: %d\n", num_output_channels_,
           num_input_channels_);
    for (int m = 0; m < num_input_channels_; m++) {
        int output_chan = m % num_output_channels_;
        gain[output_chan][m] = 1.0f;
    }
}

void AudioMixer::Mix(const float* input_buffer, float* output_buffer, size_t num_frames) {
    float* out = output_buffer;
    float* in = const_cast<float*>(input_buffer);
    for (size_t n = 0; n < num_frames; n++) {
        for (int i = 0; i < num_output_channels_; i++) {
            *out = 0.0;
            for (int j = 0; j < num_input_channels_; j++) {
                *out += *(in + j) * gain[i][j];
            }
            ++out;
        }
        in += num_input_channels_;
    }
}
