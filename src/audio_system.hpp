#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <portaudio.h>
#include <memory>
#include <optional>
#include "audio_buffer.hpp"

class AudioSystem {
   public:
    AudioSystem();
    ~AudioSystem();
    void TogglePlayback(std::shared_ptr<AudioBuffer> ab, float start, std::optional<float> end);
    void Play(std::shared_ptr<AudioBuffer> ab, float start, std::optional<float> end);
    bool Playing(float* time);
    void SetLooping(bool do_loop);
    bool Looping() const { return loop; }

    std::shared_ptr<AudioBuffer> playingBuffer;

    int index;
    int end_index;

   private:
    static int Callback(const void* input_buffer,
                        void* output_buffer,
                        unsigned long frames_per_buffer,
                        const PaStreamCallbackTimeInfo* time_info,
                        PaStreamCallbackFlags status_flags,
                        void* user_data);
    PaStream* stream = nullptr;
    int num_channels = 0;
    int samplerate = 0;
    bool loop;
    int start_index;
};

#endif
