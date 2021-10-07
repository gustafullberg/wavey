#include "audio_mixer.hpp"

#include <cassert>
#include <cstdio>

AudioMixer::AudioMixer(int num_input_channels, int num_output_channels)
    : gain_(num_output_channels, std::vector<float>(num_input_channels)),
      num_output_channels_(num_output_channels),
      num_input_channels_(num_input_channels) {
    printf("creating mixer output channels: %d, input channels: %d\n", num_output_channels_,
           num_input_channels_);
    for (int m = 0; m < num_input_channels_; m++) {
        int output_chan = m % num_output_channels_;
        gain_[output_chan][m] = 1.0f;
    }
}

void AudioMixer::Solo(int channel) {
    assert(channel >= 0 && channel < num_input_channels_);
    for (int m = 0; m < num_output_channels_; ++m) {
        std::fill(gain_[m].begin(), gain_[m].end(), 0.0f);
        gain_[m][channel] = 1.0f;
    }
}

void AudioMixer::Gain(float linear_gain) {
    for (std::vector<float>& output_channel : gain_) {
        for (float& gain : output_channel) {
            gain *= linear_gain;
        }
    }
}

void AudioMixer::Mix(const float* input_buffer, float* output_buffer, size_t num_frames) {
    float* out = output_buffer;
    float* in = const_cast<float*>(input_buffer);
    for (size_t n = 0; n < num_frames; n++) {
        for (int i = 0; i < num_output_channels_; i++) {
            *out = 0.0;
            for (int j = 0; j < num_input_channels_; j++) {
                *out += *(in + j) * gain_[i][j];
            }
            ++out;
        }
        in += num_input_channels_;
    }
}
