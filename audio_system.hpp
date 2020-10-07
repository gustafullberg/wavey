#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <portaudio.h>
#include <memory>
#include "audio_buffer.hpp"

class AudioSystem {
   public:
    AudioSystem();
    ~AudioSystem();
    void TogglePlayback(const AudioBuffer& ab, float start, float end);
    void Play(const AudioBuffer& ab, float start, float end);

    std::unique_ptr<AudioBuffer> playingBuffer;
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
};

#endif
