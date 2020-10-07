#include "audio_system.hpp"
#include <cmath>
#include <cstring>

AudioSystem::AudioSystem() {
    Pa_Initialize();
}

AudioSystem::~AudioSystem() {
    if (stream) {
        Pa_CloseStream(stream);
    }
    Pa_Terminate();
}

void AudioSystem::TogglePlayback(std::shared_ptr<AudioBuffer> ab, float start, float end) {
    if (stream && Pa_IsStreamActive(stream)) {
        Pa_AbortStream(stream);
    } else {
        Play(ab, start, end);
    }
}

void AudioSystem::Play(std::shared_ptr<AudioBuffer> ab, float start, float end) {
    if (stream && (num_channels != ab->NumChannels() || samplerate != ab->Samplerate())) {
        Pa_CloseStream(stream);
        stream = nullptr;
    } else if (stream) {
        Pa_AbortStream(stream);
    }

    if (!stream) {
        num_channels = ab->NumChannels();
        samplerate = ab->Samplerate();
        Pa_OpenDefaultStream(&stream, 0, num_channels, paFloat32, samplerate,
                             paFramesPerBufferUnspecified, Callback, this);
    }
    playingBuffer = ab;
    if (end >= 0 && start > end) {
        std::swap(start, end);
    }
    index = std::floor(start * ab->Samplerate());
    index = std::max(index, 0);
    end_index = end > 0.f ? std::floor(end * ab->Samplerate()) : ab->NumFrames();
    end_index = std::min(end_index, static_cast<int>(ab->NumFrames()));
    Pa_StartStream(stream);
}

int AudioSystem::Callback(const void* input_buffer,
                          void* output_buffer,
                          unsigned long frames_per_buffer,
                          const PaStreamCallbackTimeInfo* time_info,
                          PaStreamCallbackFlags status_flags,
                          void* user_data) {
    float* out = static_cast<float*>(output_buffer);
    AudioSystem* t = static_cast<AudioSystem*>(user_data);
    const int frames = static_cast<int>(frames_per_buffer);
    const int frames_to_copy = std::min(frames, t->end_index - t->index);
    const float* samples = t->playingBuffer->Samples();

    memcpy(out, &samples[t->num_channels * t->index],
           sizeof(float) * t->num_channels * frames_to_copy);
    t->index += frames_to_copy;

    if (frames_to_copy < frames) {
        for (int i = frames_to_copy * t->num_channels; i < frames * t->num_channels; i++) {
            out[i] = 0.f;
        }
        return paComplete;
    }

    return paContinue;
}
