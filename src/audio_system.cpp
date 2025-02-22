#include "audio_system.hpp"

#include <SDL3/SDL_audio.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>

namespace  {
  constexpr int kStreamBufferSizeFrames = 1024;
}

AudioSystem::AudioSystem(): stream_buffer_(kStreamBufferSizeFrames * NumOutputChannels()) {}

AudioSystem::~AudioSystem() {
    if (audio_stream_) {
        SDL_PauseAudioStreamDevice(audio_stream_);
        SDL_DestroyAudioStream(audio_stream_);
        audio_stream_ = nullptr;
    }
}

void AudioSystem::SetLooping(bool do_loop) {
    loop = do_loop;
}

void AudioSystem::TogglePlayback(std::shared_ptr<AudioBuffer> ab,
                                 std::unique_ptr<AudioMixer> mixer,
                                 float start,
                                 std::optional<float> end) {
    if (audio_stream_ && !SDL_AudioStreamDevicePaused(audio_stream_)) {
        SDL_PauseAudioStreamDevice(audio_stream_);
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
    if (audio_stream_ && samplerate != ab->Samplerate()) {
        SDL_PauseAudioStreamDevice(audio_stream_);
        SDL_DestroyAudioStream(audio_stream_);
        audio_stream_ = nullptr;
    } else if (audio_stream_) {
        SDL_PauseAudioStreamDevice(audio_stream_);
    }

    if (!audio_stream_) {
        num_channels = NumOutputChannels();
        samplerate = ab->Samplerate();
        audio_spec_ = {
            .format = SDL_AUDIO_F32,
            .channels = NumOutputChannels(),
            .freq = samplerate,
        };
        audio_stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec_,
                                                  &Callback, this);
        if (!audio_stream_) {
	  std::cerr << "Unable to open sound device with correct parameters." << SDL_GetError();;
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
    if (audio_stream_ && end_index != start_index) {
        int buffer_size = 0;
        SDL_AudioSpec spec;
        SDL_GetAudioDeviceFormat(SDL_GetAudioStreamDevice(audio_stream_), &spec, &buffer_size);
        SDL_ResumeAudioStreamDevice(audio_stream_);
    }
}

bool AudioSystem::Playing(float* time) {
    if (audio_stream_ && !SDL_AudioStreamDevicePaused(audio_stream_)) {
        *time = static_cast<float>(index) / playingBuffer->Samplerate();
        return true;
    }
    return false;
}

void AudioSystem::Callback(void* userdata,
                           SDL_AudioStream* stream,
                           int additional_amount,
                           int total_amount) {
    AudioSystem* t = static_cast<AudioSystem*>(userdata);
    const int frame_size_bytes = (sizeof(float) * t->audio_spec_.channels);
    const float* samples = t->playingBuffer->Samples();

    int additional_frames = additional_amount / frame_size_bytes;
    int frames_to_copy = std::min(additional_frames, t->end_index - t->index);
    
    while (frames_to_copy > 0) {
      int num_frames_to_transfer = std::min(frames_to_copy, kStreamBufferSizeFrames);
        t->mixer_->Mix(&samples[t->playingBuffer->NumChannels() * t->index],
                       t->stream_buffer_.data(), num_frames_to_transfer);
        t->index += num_frames_to_transfer;
        const int transferred_data_size = num_frames_to_transfer * frame_size_bytes;
        SDL_PutAudioStreamData(stream, t->stream_buffer_.data(), transferred_data_size);
        additional_amount -= transferred_data_size;
        total_amount -= transferred_data_size;
	frames_to_copy -= num_frames_to_transfer;
	additional_frames -= num_frames_to_transfer;
    }
    if (additional_frames > 0) {
        if (t->loop) {
            t->index = t->start_index;
            // Call the callback recursively.
            return Callback(userdata, stream, additional_amount, total_amount);
        } else {
            SDL_PauseAudioStreamDevice(t->audio_stream_);
            return;
        }
    }
}
