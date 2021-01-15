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

void AudioSystem::SetLooping(bool do_loop) {
    loop = do_loop;
}

void AudioSystem::TogglePlayback(std::shared_ptr<AudioBuffer> ab,
                                 float start,
                                 std::optional<float> end) {
    if (stream && Pa_IsStreamActive(stream)) {
        Pa_AbortStream(stream);
    } else {
        Play(ab, start, end);
    }
}

void AudioSystem::Play(std::shared_ptr<AudioBuffer> ab, float start, std::optional<float> end) {
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
        Pa_StartStream(stream);
    }
}

bool AudioSystem::Playing(float* time) {
    if (stream && Pa_IsStreamActive(stream)) {
        *time = static_cast<float>(index) / playingBuffer->Samplerate();
        return true;
    }
    return false;
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
        if (t->loop) {
            t->index = t->start_index;
            // Call the callback recursively.
            return Callback(input_buffer, &out[t->num_channels * frames_to_copy],
                            frames_per_buffer - frames_to_copy, time_info, status_flags, user_data);
        } else {
            for (int i = frames_to_copy * t->num_channels; i < frames * t->num_channels; i++) {
                out[i] = 0.f;
            }
            return paComplete;
        }
    }

    return paContinue;
}
