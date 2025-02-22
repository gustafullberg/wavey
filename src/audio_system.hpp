#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <memory>
#include <optional>

#include <stdint.h>

#include "SDL3/SDL_audio.h"
#include "audio_buffer.hpp"
#include "audio_mixer.hpp"

class AudioSystem {
   public:
    AudioSystem();
    ~AudioSystem();
    void TogglePlayback(std::shared_ptr<AudioBuffer> ab,
                        std::unique_ptr<AudioMixer> mixer,
                        float start,
                        std::optional<float> end);
    void Play(std::shared_ptr<AudioBuffer> ab,
              std::unique_ptr<AudioMixer> mixer,
              float start,
              std::optional<float> end);
    bool Playing(float* time);
    void SetLooping(bool do_loop);
    bool Looping() const { return loop; }

  int NumOutputChannels() const { return 2; }

    std::shared_ptr<AudioBuffer> playingBuffer;

    int index;
    int end_index;

   private:
    static void Callback(void* userdata,
                         SDL_AudioStream* stream,
                         int additional_amount,
                         int total_amount);

    SDL_AudioStream* audio_stream_ = nullptr;
    SDL_AudioSpec audio_spec_;
    int num_channels = 0;
    int samplerate = 0;
    bool loop = false;
    int start_index;
    std::unique_ptr<AudioMixer> mixer_;
    std::vector<float> stream_buffer_;
};

#endif
