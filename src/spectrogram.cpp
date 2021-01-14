#include "spectrogram.hpp"
#include <fftw3.h>
#include <omp.h>
#include <chrono>
#include <cmath>
#include <iostream>

namespace {
constexpr float kDftScaleFactor = 1.f / kInputSize;
}  // namespace

std::mutex Spectrogram::mtx;

Spectrogram::Spectrogram(const float* samples, int num_channels, int num_frames) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    std::unique_lock<std::mutex> lck(mtx, std::defer_lock);

    // Hann window.
    float window[kInputSize];
    constexpr float pi = static_cast<float>(M_PI);
    for (int n = 0; n < kInputSize; n++) {
        window[n] = 0.5f * (1.f - std::cos(2.f * pi * n / (kInputSize - 1)));
    }

    // Add kInputAdvance samples at the beginning,
    const int start_index = -kInputAdvance;
    // Add between kInputAdvance and kInputSize samples at the end.
    const int end_index = kInputAdvance * ((num_frames + kInputSize - 1) / kInputAdvance);
    // Number of power spectra (DFTs) per channel.
    const int num_spectra_per_chanel = (end_index - start_index) / kInputAdvance - 1;

    power_spectra.resize(num_channels);
    for (int c = 0; c < num_channels; c++) {
        power_spectra[c].resize(num_spectra_per_chanel);
    }

    const int num_threads = omp_get_max_threads();
    std::vector<float*> input_buffers(num_threads, nullptr);
    std::vector<fftwf_complex*> output_buffers(num_threads, nullptr);
    std::vector<fftwf_plan> plans(num_threads);
    lck.lock();
    for (int t = 0; t < num_threads; t++) {
        input_buffers[t] = static_cast<float*>(fftwf_malloc(kInputSize * sizeof(float)));
        output_buffers[t] =
            static_cast<fftwf_complex*>(fftwf_malloc(kOutputSize * sizeof(fftwf_complex)));
        plans[t] =
            fftwf_plan_dft_r2c_1d(kInputSize, input_buffers[t], output_buffers[t], FFTW_ESTIMATE);
    }
    lck.unlock();

#pragma omp parallel
    {
        float* input_buffer = input_buffers[omp_get_thread_num()];
        fftwf_complex* output_buffer = output_buffers[omp_get_thread_num()];
        fftwf_plan plan = plans[omp_get_thread_num()];

        for (int c = 0; c < num_channels; c++) {
            auto& power_spectra_channel = power_spectra[c];

#pragma omp for
            for (int i = 0; i < num_spectra_per_chanel; i++) {
                // Fill input buffer and apply Hann window.
                int src_frame = start_index + i * kInputAdvance;
                for (int k = 0; k < kInputSize; k++) {
                    if (src_frame >= 0 && src_frame < num_frames) {
                        input_buffer[k] = samples[src_frame * num_channels + c] * window[k];
                    } else {
                        input_buffer[k] = 0.f;
                    }
                    src_frame++;
                }

                // Transform.
                fftwf_execute(plan);

                // Power spectrum.
                auto& power = power_spectra_channel[i];
                for (int k = 0; k < kOutputSize; k++) {
                    const float re = output_buffer[k][0] * kDftScaleFactor;
                    const float im = output_buffer[k][1] * kDftScaleFactor;

                    // Power in dB domain.
                    constexpr float min_dB = -100.f;
                    constexpr float max_dB = -20.f;
                    constexpr float scaling = 65535.f / (max_dB - min_dB);
                    float dB = std::max(min_dB, std::min(max_dB, 10.f * std::log10(re * re + im * im)));
                    power[k] = (dB - min_dB) * scaling;
                }
            }
        }
    }

    lck.lock();
    for (int t = 0; t < num_threads; t++) {
        fftwf_destroy_plan(plans[t]);
        fftwf_free(input_buffers[t]);
        fftwf_free(output_buffers[t]);
    }
    lck.unlock();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cerr << "Power spectrum computed in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms"
              << std::endl;
}
