#include "audio_mixer.hpp"

#include <cassert>

AudioMixer::AudioMixer(int num_input_channels, int num_output_channels)
    : mix_matrix_(num_output_channels, std::vector<float>(num_input_channels)),
      num_output_channels_(num_output_channels),
      num_input_channels_(num_input_channels) {
    if (num_input_channels > 1) {
        for (int m = 0; m < num_input_channels_; m++) {
            int output_chan = m % num_output_channels_;
            mix_matrix_[output_chan][m] = 1.0f;
        }
    } else {
        Solo(0);
    }
}

void AudioMixer::Solo(int channel) {
    assert(channel >= 0 && channel < num_input_channels_);
    for (int m = 0; m < num_output_channels_; ++m) {
        std::fill(mix_matrix_[m].begin(), mix_matrix_[m].end(), 0.0f);
        mix_matrix_[m][channel] = 1.0f;
    }
}

void AudioMixer::Gain(float linear_gain) {
    gain_ = linear_gain;
}

void AudioMixer::Mix(const float* input_buffer, float* output_buffer, std::size_t num_frames) {
    float* out = output_buffer;
    float* in = const_cast<float*>(input_buffer);
    for (std::size_t n = 0; n < num_frames; n++) {
        for (int i = 0; i < num_output_channels_; i++) {
            *out = 0.0;
            for (int j = 0; j < num_input_channels_; j++) {
                *out += *(in + j) * mix_matrix_[i][j] * gain_;
            }
            ++out;
        }
        in += num_input_channels_;
    }
}
