#ifndef AUDIO_BUFFER_HPP
#define AUDIO_BUFFER_HPP

#include <assert.h>
#include <string>
#include <vector>

class AudioBuffer {
   public:
    AudioBuffer(std::string file_name);
    int Samplerate() const { return samplerate; }
    int NumChannels() const { return num_channels; }
    int NumFrames() const { return num_frames; }
    float Duration() const { return static_cast<float>(num_frames) / samplerate; }
    const float* Samples() const { return samples.data(); }
    const float* Samples(float start) const {
        assert(start < Duration());
        return samples.data() + num_channels * static_cast<int>(start * samplerate);
    }
    operator bool() const { return samplerate != 0; }

   private:
    int samplerate = 0;
    int num_channels = 0;
    int num_frames = 0;
    int format = 0;
    std::vector<float> samples;
};

#endif
