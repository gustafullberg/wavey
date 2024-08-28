#include "spectrum_state.hpp"

#include <assert.h>
#include <complex.h>
#include <string.h>

#include <iostream>
#include <memory>

#include <fftw3.h>

#include "implot.h"

constexpr int kFftSize = 4096;
static_assert((kFftSize & (kFftSize - 1)) == 0, "kFftSize must be a power of 2");
constexpr int kFftOutputSize = kFftSize / 2 + 1;
constexpr float kWindowSizeMs = 10.0f;

namespace {
template <class T>
struct FftwAllocator {
    typedef T value_type;

    FftwAllocator() = default;

    template <class U>
    constexpr FftwAllocator(const FftwAllocator<U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_array_new_length();

        if (auto p = static_cast<T*>(fftwf_malloc(n * sizeof(T)))) {
            return p;
        }
        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept { fftwf_free(p); }
};

template <class T, class U>
bool operator==(const FftwAllocator<T>&, const FftwAllocator<U>&) {
    return true;
}

template <class T, class U>
bool operator!=(const FftwAllocator<T>&, const FftwAllocator<U>&) {
    return false;
}

template <typename... Args>
std::string string_format(const std::string& format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;  // Extra space for '\0'
    if (size_s <= 0) {
        throw std::runtime_error("Error during formatting.");
    }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1);  // We don't want the '\0' inside
}

}  // namespace

void SpectrumState::Add(const Track& track, float begin, float end, int channel) {
    if (end < begin)
        std::swap(end, begin);
    if (track.audio_buffer == nullptr)
        return;
    if ((end - begin) * 1000.0f < kWindowSizeMs)
        return;
    Spectrum s;
    s.name = string_format("%s #%d [%.3f - %.3f]", track.short_name.c_str(), channel, begin, end);
    s.frequencies.resize(kFftOutputSize);

    {
        const int csize = ImPlot::GetColormapSize();
        ImVec4 color = ImPlot::GetColormapColor(spectrums_.size() % csize);
        s.color[0] = color.x;
        s.color[1] = color.y;
        s.color[2] = color.z;
    }

    for (int n = 0; n < kFftOutputSize; ++n) {
        s.frequencies[n] = static_cast<float>(track.audio_buffer->Samplerate()) * n / kFftSize;
    }
    s.future_spectrum = std::async([audio = track.audio_buffer, channel, begin, end,
                                    &fftw_mutex = fftw_mutex_]() {
        const float duration = end - begin;
        const int num_frames =
            std::min(audio->NumFrames(), static_cast<int>(duration * audio->Samplerate()));
        std::vector<float> output(kFftOutputSize);
        std::vector<float, FftwAllocator<float>> input(kFftSize);
        std::vector<std::complex<float>, FftwAllocator<std::complex<float>>> fft_output(
            kFftOutputSize);
        fftwf_plan plan;
        {
            std::scoped_lock locked_fftw_mutex(fftw_mutex);
            plan = fftwf_plan_dft_r2c_1d(kFftSize, input.data(),
                                         reinterpret_cast<fftwf_complex*>(fft_output.data()),
                                         FFTW_ESTIMATE);
        }
        const int window_size = kWindowSizeMs * audio->Samplerate() / 1000;
        int count = 0;
        for (int start = 0; start + window_size < num_frames; start += window_size / 4, ++count) {
            auto input_it = input.begin();
            const float* a = audio->Samples() + start * audio->NumChannels() + channel;
            for (int k = 0; k < window_size; ++k, ++input_it) {
                *input_it = *a;
                a += audio->NumChannels();
            }
            std::fill(input_it, input.end(), 0.0f);
            fftwf_execute(plan);
            for (int k = 0; k < kFftOutputSize; ++k) {
                const float r = fft_output[k].real();
                const float i = fft_output[k].imag();
                output[k] += r * r + i * i;
            }
        }
        {
            std::scoped_lock locked_fftw_mutex(fftw_mutex);
            fftwf_destroy_plan(plan);
        }
        if (count > 0) {
            const float scale =
                1.0f / (static_cast<float>(window_size) * static_cast<float>(window_size) * count);
            for (int k = 0; k < kFftOutputSize; ++k) {
                output[k] = 10.0f * log10f(output[k] * scale);
            }
        } else {
            // No spectrum
            return std::vector<float>(kFftOutputSize);
        }
        return output;
    });
    spectrums_.push_back(std::move(s));
}
