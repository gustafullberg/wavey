#include "audio_system.hpp"

#include <cmath>
#include <cstring>
#include "assert.h"

#include <iostream>
#include <memory>

#include "SDL2/SDL_audio.h"

AudioSystem::AudioSystem() {}

AudioSystem::~AudioSystem() {
    if (active_audio_device_.has_value()) {
        SDL_PauseAudioDevice(active_audio_device_.value(), /*pause_on=*/1);
        SDL_CloseAudioDevice(active_audio_device_.value());
        active_audio_device_.reset();
    }
}

uint8_t AudioSystem::NumOutputChannels() {
    return 2;
}

void AudioSystem::SetLooping(bool do_loop) {
    loop = do_loop;
}

void AudioSystem::TogglePlayback(std::shared_ptr<AudioBuffer> ab,
                                 std::unique_ptr<AudioMixer> mixer,
                                 float start,
                                 std::optional<float> end) {
    if (active_audio_device_ &&
        SDL_GetAudioDeviceStatus(active_audio_device_.value()) == SDL_AUDIO_PLAYING) {
        SDL_PauseAudioDevice(active_audio_device_.value(), /*pauseon=*/1);
    } else {
        Play(ab, std::move(mixer), start, end);
    }
}

void AudioSystem::Play(std::shared_ptr<AudioBuffer> ab,
                       std::unique_ptr<AudioMixer> mixer,
                       float start,
                       std::optional<float> end) {
    mixer_ = std::move(mixer);
    assert(mixer_->NumOutputChannels() == NumOutputChannels());
    if (active_audio_device_ && samplerate != ab->Samplerate()) {
        SDL_PauseAudioDevice(active_audio_device_.value(), /*pauseon=*/1);
        SDL_CloseAudioDevice(active_audio_device_.value());
        active_audio_device_.reset();
    } else if (active_audio_device_) {
        SDL_PauseAudioDevice(active_audio_device_.value(), /*pauseon=*/1);
    }

    constexpr int kBufferSizeInFrames = 1024;

    if (!active_audio_device_) {
        num_channels = NumOutputChannels();
        samplerate = ab->Samplerate();
        const SDL_AudioSpec desired = {
            .freq = samplerate,
            .format = AUDIO_F32,
            .channels = NumOutputChannels(),
            .samples = kBufferSizeInFrames,
            .callback = &Callback,
            .userdata = this,
        };
        active_audio_device_ = SDL_OpenAudioDevice(NULL, /*iscapture=*/0, &desired, &audio_spec_,
                                                   SDL_AUDIO_ALLOW_FORMAT_CHANGE);
        if (desired.format != audio_spec_.format || desired.channels != audio_spec_.channels ||
            desired.freq != samplerate) {
            std::cerr << "Unable to open sound device with correct parameters.";
            SDL_CloseAudioDevice(active_audio_device_.value());
            active_audio_device_.reset();
        }
    }
    playingBuffer = ab;
    if (end && start > *end) {
        std::swap(start, *end);
    }
    start_index = std::floor(start * ab->Samplerate());
    start_index = std::max(start_index, 0);
    start_index = std::min(start_index, ab->NumFrames());
    end_index = end ? std::floor(*end * ab->Samplerate()) : ab->NumFrames();
    end_index = std::min(end_index, ab->NumFrames());
    index = start_index;
    if (end_index != start_index) {
        SDL_PauseAudioDevice(active_audio_device_.value(), /*pause_on=*/0);
    }
}

bool AudioSystem::Playing(float* time) {
    if (active_audio_device_ &&
        SDL_GetAudioDeviceStatus(active_audio_device_.value()) == SDL_AUDIO_PLAYING) {
        *time = (static_cast<float>(index) + audio_spec_.samples) / playingBuffer->Samplerate();
        return true;
    }
    return false;
}

void AudioSystem::Callback(void* user_data, Uint8* output_buffer, int len) {
    float* out = reinterpret_cast<float*>(output_buffer);
    AudioSystem* t = static_cast<AudioSystem*>(user_data);
    const int frames = len / (sizeof(float) * t->audio_spec_.channels);
    const int frames_to_copy = std::min(frames, t->end_index - t->index);
    const float* samples = t->playingBuffer->Samples();

    t->mixer_->Mix(&samples[t->playingBuffer->NumChannels() * t->index], out, frames_to_copy);
    t->index += frames_to_copy;
    if (frames_to_copy < frames) {
        if (t->loop) {
            t->index = t->start_index;
            // Call the callback recursively.
            return Callback(
                user_data, reinterpret_cast<Uint8*>(&out[t->audio_spec_.channels * frames_to_copy]),
                frames - frames_to_copy);
        } else {
            for (int i = frames_to_copy * t->num_channels; i < frames * t->audio_spec_.channels;
                 i++) {
                out[i] = 0.f;
            }
            SDL_PauseAudioDevice(t->active_audio_device_.value(), /*pause_on=*/1);
            return;
        }
    }
}
