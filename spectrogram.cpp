#include "spectrogram.hpp"
#include <fftw3.h>
#include <chrono>
#include <cmath>
#include <iostream>

namespace {
constexpr float kDftScaleFactor = 1.f / kInputSize;
}  // namespace

Spectrogram::Spectrogram(const float* samples, int num_channels, int num_frames) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    // Hann window.
    float window[kInputSize];
    constexpr float pi = static_cast<float>(M_PI);
    for (int n = 0; n < kInputSize; n++) {
        window[n] = 0.5f * (1.f - std::cos(2.f * pi * n / (kInputSize - 1)));
    }

    float* input_buffer = static_cast<float*>(fftwf_malloc(kInputSize * sizeof(float)));
    fftwf_complex* output_buffer =
        static_cast<fftwf_complex*>(fftwf_malloc(kOutputSize * sizeof(fftwf_complex)));

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(kInputSize, input_buffer, output_buffer, FFTW_ESTIMATE);

    power_spectra.resize(num_channels);

    for (int c = 0; c < num_channels; c++) {
        auto& power_spectra_channel = power_spectra[c];
        const int num_spectra_per_chanel =
            std::max((num_frames - kInputAdvance) / kInputAdvance, 0);
        power_spectra_channel.resize(num_spectra_per_chanel);

        for (int i = 0; i < num_spectra_per_chanel; i++) {
            // Fill input buffer and apply Hann window.
            int src_idx = i * kInputAdvance * num_channels + c;
            for (int k = 0; k < kInputSize; k++) {
                input_buffer[k] = samples[src_idx] * window[k];
                src_idx += num_channels;
            }

            // Transform.
            fftwf_execute(plan);

            // Power spectrum.
            auto& power = power_spectra_channel[i];
            for (int k = 0; k < kOutputSize; k++) {
                const float re = output_buffer[k][0] * kDftScaleFactor;
                const float im = output_buffer[k][1] * kDftScaleFactor;

                // Power in dB domain.
                power[k] = 10.f * log10(re * re + im * im);
            }
        }
    }

    fftwf_destroy_plan(plan);
    fftwf_free(input_buffer);
    fftwf_free(output_buffer);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cerr << "Power spectrum computed in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms"
              << std::endl;
}
