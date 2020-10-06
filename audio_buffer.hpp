#ifndef AUDIO_BUFFER_HPP
#define AUDIO_BUFFER_HPP

#include <string>
#include <vector>

class AudioBuffer {
   public:
    bool LoadFile(std::string file_name);
    size_t Samplerate() const { return samplerate; }
    size_t NumChannels() const { return num_channels; }
    size_t NumFrames() const { return num_frames; }
    const float* Samples() const { return samples.data(); }
    operator bool() const { return samplerate != 0; }

   private:
    size_t samplerate = 0;
    size_t num_channels = 0;
    size_t num_frames = 0;
    std::vector<float> samples;
};

#endif
