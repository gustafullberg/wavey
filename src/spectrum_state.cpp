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
constexpr int kHalfFftSize = kFftSize / 2 + 1;
constexpr int kStepSize = 1024;

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
    if (track.audio_buffer == nullptr)
        return;
    Spectrum s;
    s.name = string_format("%s #%d [%.3f - %.3f]", track.short_name.c_str(), channel, begin, end);
    s.frequencies.resize(kHalfFftSize);

    {
        const int csize = ImPlot::GetColormapSize();
        ImVec4 color = ImPlot::GetColormapColor(spectrums_.size() % csize);
        s.color[0] = color.x;
        s.color[1] = color.y;
        s.color[2] = color.z;
    }

    for (int n = 0; n < kHalfFftSize; ++n) {
        s.frequencies[n] = static_cast<float>(track.audio_buffer->Samplerate()) * n / kFftSize;
    }
    std::shared_ptr<AudioBuffer> audio = track.audio_buffer;
    s.future_spectrum = std::async([audio, channel, begin, end]() {
        const float duration = end - begin;
        const int num_frames = duration * audio->Samplerate();
        std::vector<float> output(kHalfFftSize);
        std::vector<float, FftwAllocator<float>> input(kFftSize);
        std::vector<std::complex<float>, FftwAllocator<std::complex<float>>> fft_output(
            kHalfFftSize);

        fftwf_plan plan = fftwf_plan_dft_r2c_1d(kFftSize, input.data(),
                                                reinterpret_cast<fftwf_complex*>(fft_output.data()),
                                                FFTW_ESTIMATE);
        int count = 0;
        for (int start = 0; start + kFftSize < num_frames; start += kStepSize, ++count) {
            const float* a = audio->Samples() + start * audio->NumChannels() + channel;
            for (int k = 0; k < kFftSize; k++) {
                input[k] = *a;
                a += audio->NumChannels();
            }
            fftwf_execute(plan);
            for (int k = 0; k < kHalfFftSize; ++k) {
                const float r = fft_output[k].real();
                const float i = fft_output[k].imag();
                output[k] += r * r + i * i;
            }
        }

        fftwf_destroy_plan(plan);
        if (count > 0) {
            const float scale =
                (static_cast<float>(kStepSize) / static_cast<float>(kFftSize)) / (kFftSize * count);
            for (int k = 0; k < kHalfFftSize; ++k) {
                output[k] = 20.0f * log10f(output[k] * scale);
            }
        } else {
            // No spectrum
            return std::vector<float>(kHalfFftSize);
        }
        return output;
    });
    spectrums_.push_back(std::move(s));
}
