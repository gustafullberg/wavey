#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <memory>
#include <optional>

#include <stdint.h>

#include "SDL2/SDL_audio.h"
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

    uint8_t NumOutputChannels();

    std::shared_ptr<AudioBuffer> playingBuffer;

    int index;
    int end_index;

   private:
    static void Callback(void* user_data, Uint8* stream, int len);

    std::optional<SDL_AudioDeviceID> stream;
    SDL_AudioSpec audio_spec_;
    int num_channels = 0;
    int samplerate = 0;
    bool loop = false;
    int start_index;
    std::unique_ptr<AudioMixer> mixer_;
};

#endif
