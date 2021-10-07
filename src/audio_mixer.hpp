#ifndef AUDIO_MIXER_HPP
#define AUDIO_MIXER_HPP

#include <vector>

class AudioMixer {
   public:
    AudioMixer(int num_input_channels, int num_output_channels);
    ~AudioMixer() = default;

    void Mix(const float* input_buffer, float* output_buffer, size_t num_frames);
    int NumOutputChannels() const { return num_output_channels_; }
    void Solo(int channel);
    void Gain(float linear_gain);

   private:
    std::vector<std::vector<float>> gain_;
    int num_output_channels_;
    int num_input_channels_;
};

#endif  // AUDIO_MIXER_HPP
