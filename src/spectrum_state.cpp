#include "spectrum_state.hpp"

#include <assert.h>
#include <string.h>

#include <iostream>
#include <memory>

#include <fftw3.h>

constexpr size_t kFftSize = 4096;
constexpr size_t kHalfFftSize = kFftSize / 2 + 1;
constexpr size_t kStepSize = 512;

namespace {
template <class T>
struct FftwDeleter {
    void operator()(T* p) const { fftwf_free(p); }
};
}  // namespace

void SpectrumState::Add(const Track& track, float begin, float end, int channel) {
    if (track.audio_buffer == nullptr)
        return;
    std::unique_ptr<Spectrum> s = std::make_unique<Spectrum>();

    std::shared_ptr<AudioBuffer> audio = track.audio_buffer;
    s->future_spectrum_ = std::async([audio, channel, begin, end]() {
        std::cerr << "Start computing fft: " << begin << " - " << end << std::endl;
        float duration = end - begin;
        int num_frames = duration * audio->Samplerate();
        std::vector<float> output(kHalfFftSize);
        std::unique_ptr<float, FftwDeleter<float>> input(fftwf_alloc_real(kFftSize));
        std::unique_ptr<fftwf_complex, FftwDeleter<fftwf_complex>> fft_output(
            fftwf_alloc_complex(kHalfFftSize));

        fftwf_plan plan =
            fftwf_plan_dft_r2c_1d(kFftSize, input.get(), fft_output.get(), FFTW_ESTIMATE);
        int count = 0;
        for (int start = 0; start + kFftSize < num_frames; start += kStepSize) {
            const float* a = audio->Samples() + start * audio->NumChannels() + channel;
            for (int k = 0; k < kFftSize; k++) {
                input.get()[k] = *a;
                a += audio->NumChannels();
            }
            fftwf_execute(plan);
            for (int k = 0; k < kHalfFftSize; ++k) {
                float r = fft_output.get()[k][0];
                float i = fft_output.get()[k][1];
                output[k] = r * r + i * i;
            }
            count += 1;
        }

        fftwf_destroy_plan(plan);
        if (count > 0) {
            for (int k = 0; k < kHalfFftSize; ++k) {
                output[k] = output[k] / count;
            }
        } else {
            // No spectrum
            return std::vector<float>();
        }

        std::cerr << "Stop computing fft" << std::endl;
        return output;
    });
    spectrums_.push_back(std::move(s));
}
